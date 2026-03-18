# factor041-050 设计稿

## 1. 范围

本稿覆盖 `factor041` 到 `factor050`。

这组因子是当前项目中**唯一尚未实现 C++ 版本**的一批。

## 2. 因子定义与 Python 原始逻辑

### factor041：封板后 70ms 内涨停价平均新增挂单量

```python
def factor041(self):
    if time_sub(self.predict_t, self.lock_time) < 70:
        return np.nan
    temp_order = self.order[
        (time >= lock_time) &
        (time <= time_add(lock_time, 70)) &
        (orderKind == 'A') &
        (orderPrice == limit_up) &
        (functionCode == 'B')]
    return temp_order['orderVolume'].sum() / len(temp_order)
```

- 数据依赖：`order`
- 时间窗口：`[lock_time, lock_time + 70ms]`
- 过滤条件：涨停价、新增单、买入
- 前置检查：若 `predict_t - lock_time < 70ms`，返回 NaN
- 计算：总挂单量 / 挂单笔数

### factor042：封板后 100ms 的上升梯度

```python
def factor042(self):
    if time_sub(self.predict_t, self.lock_time) < 100:
        return np.nan
    temp_cs = cross_section[
        (time >= lock_time) &
        (time <= time_add(lock_time, 100))]
    denominator = time_sub(temp_cs['time'].iloc[-1], temp_cs['time'].iloc[0])
    peak = temp_cs['委买总额'].max()
    numerator = peak - temp_cs['委买总额'].iloc[0]
    return numerator / denominator if denominator != 0 else numerator
```

- 数据依赖：`cross_section`
- 时间窗口：`[lock_time, lock_time + 100ms]`
- 前置检查：若 `predict_t - lock_time < 100ms`，返回 NaN
- 计算：(峰值 - 首值) / (末时刻 - 首时刻)

### factor043：封板后到 predict_t 的涨停价主卖金额

```python
def factor043(self):
    temp_trade = self.trade[
        (time >= lock_time) &
        (time <= predict_t) &
        (bsFlag == 'S') &
        (tradePrice == limit_up)]
    return temp_trade['tradeVolume'].sum() * limit_up / 10000
```

- 数据依赖：`trade`
- 时间窗口：`[lock_time, predict_t]`
- 过滤条件：涨停价、主卖
- 计算：总成交量 × 涨停价 / 10000

### factor044：涨停价主卖成交中不同 bidOrder 的数量

```python
def factor044(self):
    temp_trade = self.trade[
        (time >= lock_time) & (time <= predict_t) &
        (tradePrice == limit_up) & (bsFlag == 'S')]
    return len(temp_trade['bidOrder'].unique())
```

- 计算：窗口内涨停价主卖成交涉及的不同买方订单数

### factor045：涨停价主卖成交中不同 askOrder 的数量

```python
def factor045(self):
    # 同 factor044 的过滤条件
    return len(temp_trade['askOrder'].unique())
```

- 计算：窗口内涨停价主卖成交涉及的不同卖方订单数

### factor046：bidOrder 去重数 / askOrder 去重数

```python
def factor046(self):
    bid_count = len(temp_trade['bidOrder'].unique())
    ask_count = len(temp_trade['askOrder'].unique())
    return bid_count / ask_count if ask_count != 0 else np.nan
```

### factor047：第一次涨停价撤单订单的持续时间

```python
def factor047(self):
    temp_order = order[窗口 & 涨停价]
    order_id = temp_order[orderKind == 'D']['orderOriNo'].iloc[0]
    temp_order = order[orderOriNo == order_id]
    return time_sub(temp_order['time'].iloc[-1], temp_order['time'].iloc[0])
```

- 数据依赖：`order`
- 找到窗口内第一个涨停价撤单，回查该订单的完整生命周期
- 返回末次事件时间 - 首次事件时间

### factor048：第一次涨停价撤单距离封板的时间差

```python
def factor048(self):
    # 同 factor047 找到第一个撤单
    return time_sub(temp_order['time'].iloc[-1], self.lock_time)
```

- 计算：该撤单订单末次事件时间 - lock_time

### factor049：第一次大额涨停价撤单订单的持续时间

```python
def factor049(self):
    temp_order = order[窗口 & 涨停价]
    threshold = temp_order[orderKind == 'A']['orderVolume'].quantile(0.8)
    order_id = temp_order[(orderVolume >= threshold) & (orderKind == 'D')]['orderOriNo'].iloc[0]
    temp_order = order[orderOriNo == order_id]
    return time_sub(temp_order['time'].iloc[-1], temp_order['time'].iloc[0])
```

- "大额"定义：窗口内涨停价新增挂单量的 0.8 分位数
- 其余同 factor047

### factor050：第一次大额涨停价撤单距离封板的时间差

同 factor049 找到大额撤单，返回 `time_sub(last_time, lock_time)`。

## 3. 分组分析

### 3.1 可复用现有 factor01_031 状态的因子

**factor043-046** 的核心计算——窗口内涨停价主卖成交统计——与 factor01_031 的 `WindowAggState` / `scan_trades()` 高度重叠。

现有 `WindowAggState` 已维护：

- `limit_up_trade_count`：涨停价成交笔数
- `limit_up_trade_volume_sum`：涨停价成交量合计

**需新增的状态**：

```cpp
// 在 WindowAggState 或单独结构中新增
double      limit_up_sell_volume_sum;    // factor043: 涨停价主卖总成交量
std::unordered_set<int> limit_up_sell_bid_orders;  // factor044: 去重 bidOrder
std::unordered_set<int> limit_up_sell_ask_orders;  // factor045: 去重 askOrder
```

### 3.2 需要新增扫描逻辑的因子

**factor041**：需要在 `scan_orders()` 中额外判断 `time <= time_add(lock_time, 70)` 的子窗口。

建议新增状态：

```cpp
struct EarlyWindowState {
    bool    has_early_window;      // predict_t - lock_time >= 70
    int64_t early70_add_volume_sum;
    int64_t early70_add_count;
};
```

**factor042**：需要在截面扫描中使用 `time_add(lock_time, 100)` 作为窗口上界。

可直接复用 `cross_section_utils.h` 现有的 `max_bid_amount_with_time_in_window()` 和窗口首值提取。

### 3.3 需要订单生命周期和分位数的因子

**factor047-050** 需要：

1. 在 `scan_orders()` 中追踪窗口内**第一个涨停价撤单订单** (`order_id`)
2. 对该订单回查生命周期的首末时间
3. factor049/050 还需要计算窗口内涨停价新增挂单量的 0.8 分位数

现有 `OrderLifecycleState` 已维护 `first_time` / `last_time`，可直接复用。

需新增的状态：

```cpp
struct FirstCancelState {
    bool    has_first_limit_cancel;
    int     first_limit_cancel_order_id;

    bool    has_first_large_cancel;
    int     first_large_cancel_order_id;
    double  large_cancel_threshold;  // 0.8 分位数
};
```

## 4. 建议的实现方案

### 方案 A：扩展现有 factor01_031 框架

将 factor041-050 的状态融入 `Factor01_031State`，扩展 `scan_orders()` 和 `scan_trades()`。

**优点**：

- 不增加新的扫描遍历
- 共享订单生命周期状态
- factor043-046 可几乎零成本加入

**缺点**：

- `Factor01_031State` 变得更大
- 命名不再准确（不止 001-031）

### 方案 B：建立独立的 factor041_050 工具层

新建 `factor041_050_state.h` / `factor041_050_stream.h` / `factor041_050_extract.h`。

**优点**：

- 隔离清晰
- 不影响已验证的 factor001-031 代码

**缺点**：

- 需要重复扫描 order/trade
- factor043-046 需要重复构建涨停价成交统计

### 推荐

**推荐方案 A**。理由：

1. factor043-046 与现有 `scan_trades()` 的窗口和过滤条件完全一致
2. factor047-050 共享 `OrderLifecycleState`
3. factor041 只需在 `scan_orders()` 中增加一个子窗口累加器
4. factor042 可直接复用 `cross_section_utils.h` 的公共函数

文件命名建议将 `factor01_031_*` 统一重命名为 `factor_event_*`，或在现有文件中追加并更新注释。

## 5. 新增状态设计

在 `Factor01_031State` 中追加：

```cpp
// factor041: 封板后 70ms 子窗口
bool    has_early70_window;        // predict_t - lock_time >= 70
int64_t early70_limit_up_buy_volume_sum;
int64_t early70_limit_up_buy_count;

// factor043-046: 涨停价主卖统计
double                      limit_up_sell_volume_sum;
std::unordered_set<int>     limit_up_sell_bid_orders;
std::unordered_set<int>     limit_up_sell_ask_orders;

// factor047-048: 第一次涨停价撤单
bool    has_first_limit_cancel_order;
int     first_limit_cancel_order_id;

// factor049-050: 第一次大额涨停价撤单
bool    has_first_large_cancel_order;
int     first_large_cancel_order_id;
```

## 6. 扫描逻辑变更

### scan_orders() 新增逻辑

```
对每条 order_event:
    if 已在窗口内 && price == limit_up && type == NORMAL && function == BUY:
        if time <= time_add(lock_time, 70):
            early70_volume_sum += volume
            early70_count += 1

    if 已在窗口内 && price == limit_up && type == WITHDRAW:
        if !has_first_limit_cancel:
            has_first_limit_cancel = true
            first_limit_cancel_order_id = order_id
```

注意：factor049/050 的大额撤单阈值需要窗口内所有涨停价新增挂单量的 0.8 分位数，这与 `QuantileState.limit_up_add_samples` 已收集的数据一致。因此大额撤单判定需要**延迟到扫描完成后**，在提取阶段遍历已撤订单找出第一个满足阈值条件的。

### scan_trades() 新增逻辑

```
对每条 trade_event:
    if 已在窗口内 && price == limit_up && bs_flag == SELL:
        limit_up_sell_volume_sum += volume
        limit_up_sell_bid_orders.insert(bid_order_id)
        limit_up_sell_ask_orders.insert(ask_order_id)
```

## 7. 因子提取函数

```cpp
inline double get_factor041(const Factor01_031State& state, const FactorInput& input);
inline double get_factor042(const FactorInput& input);
inline double get_factor043(const Factor01_031State& state, const FactorInput& input);
inline double get_factor044(const Factor01_031State& state);
inline double get_factor045(const Factor01_031State& state);
inline double get_factor046(const Factor01_031State& state);
inline double get_factor047(const Factor01_031State& state);
inline double get_factor048(const Factor01_031State& state, const FactorInput& input);
inline double get_factor049(const Factor01_031State& state, Factor01_031Workspace& workspace);
inline double get_factor050(const Factor01_031State& state, Factor01_031Workspace& workspace, const FactorInput& input);
```

### factor042 特殊处理

factor042 依赖 `cross_section` 而非 order/trade 事件流，可直接复用 `cross_section_utils.h`：

```cpp
inline double get_factor042(const FactorInput& input) {
    int duration = time_sub(input.basic_info.predict_t, input.basic_info.lock_time);
    if (duration < 100) return nan_value();

    int end_time = time_add(input.basic_info.lock_time, 100);
    // 复用现有 max_bid_amount_with_time_in_window + 首值提取
}
```

### factor049/050 延迟判定

由于大额撤单的阈值（0.8 分位数）在扫描完成前无法确定，提取函数需要：

1. 从 `QuantileState.limit_up_add_samples` 计算 0.8 分位阈值
2. 遍历 `order_lifecycle`，找到**最早的**满足 `has_limit_up_cancel_in_window && first_volume >= threshold` 的订单
3. "最早"按 `first_time` 排序

## 8. 数值语义

| 因子 | 空样本处理 |
|------|-----------|
| factor041 | 窗口不足 70ms → NaN；窗口内无满足条件的挂单 → NaN |
| factor042 | 窗口不足 100ms → NaN；窗口内无截面数据 → NaN |
| factor043 | 无涨停价主卖成交 → 0.0 |
| factor044 | 无涨停价主卖成交 → 0 |
| factor045 | 无涨停价主卖成交 → 0 |
| factor046 | askOrder 数为 0 → NaN |
| factor047 | 无涨停价撤单 → NaN |
| factor048 | 无涨停价撤单 → NaN |
| factor049 | 无大额撤单 → NaN |
| factor050 | 无大额撤单 → NaN |

## 9. 建议的落地顺序

1. **factor043-046**：最简单，只需在 `scan_trades()` 加三行累加 + 两个 set
2. **factor041**：在 `scan_orders()` 加子窗口累加器
3. **factor042**：复用 `cross_section_utils.h`，独立于事件流
4. **factor047-048**：在 `scan_orders()` 追踪第一个撤单
5. **factor049-050**：依赖分位数阈值，在提取阶段延迟判定
