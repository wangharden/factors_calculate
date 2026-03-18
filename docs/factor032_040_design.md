# factor032-040 设计稿

## 1. 范围

本稿覆盖 `factor032` 到 `factor040`。

这组因子全部基于 `cross_section`（截面快照序列）计算，不依赖 `order` 或 `trade` 原始事件流。

## 2. 实现状态

**已按批量状态模式实现**。代码位于：

- 公共工具：`cross_section_utils.h`（`factor_detail` 命名空间）
- 批量状态与提取：`factor032_040_extract.h`
- 因子入口：`factor_collection.h` 中的 `call_factor032()` 到 `call_factor040()`
- 批量入口：`collect_factor032_040(...)`

## 3. 因子定义与 Python 对照

### factor032：封板时刻指数增长模型的 alpha

```python
s = cross_section[(time == lock_time) & (委买总额 > 0)]['委买总额']
alpha = log(Qt / Q0) / t
```

C++ 实现：`summarize_positive_bid_amounts_at_time(cross_section, lock_time)` → `log(last / first) / count`

### factor033：predict_t 时刻指数增长模型的 alpha

同 factor032，时间点换为 `predict_t`。

### factor034：封板后到 predict_t 之间截面数据的 count

```python
len(cross_section[(time >= lock_time) & (time <= predict_t)])
```

C++ 实现：`count_cross_section_in_window(cross_section, lock_time, predict_t)`

### factor035：封板后到 predict_t 之间截面数据的更新频率

```python
count / time_sub(predict_t, lock_time)
```

特殊情况：若 `time_sub == 0`，返回 `len(cross_section)`。

### factor036：截面差分收益率的 VaR95

```python
s = (委买总额 / shift(1) - 1).fillna(0)
var_95 = s.quantile(0.05)
```

C++ 实现：`calc_diff_returns_in_window()` → `quantile_in_place(returns, 0.05)`

### factor037：截面差分收益率的 cVaR95

```python
s[s < var_95].mean()
```

C++ 实现：先算 VaR95，再对尾部取均值。

### factor038：上升梯度

```python
max_amount / time_sub(max_amount_time, lock_time)
```

C++ 实现：`max_bid_amount_with_time_in_window()` → 峰值 / 时间差

### factor039：下降梯度

```python
(last_amount - max_amount) / time_sub(predict_t, max_amount_time)
```

C++ 实现：需同时获取峰值和末值。

### factor040：截面委买总额的偏度

```python
cross_section[窗口]['委买总额'].skew()
```

C++ 实现：`collect_bid_amounts_in_window()` → `skew()`

## 4. 批量状态设计

新增批量状态：

```cpp
struct Factor032_040State {
    ExactTimePositiveSummary lock_summary;
    ExactTimePositiveSummary predict_summary;
    WindowPeakInfo           peak;

    std::size_t              window_count;
    double                   last_bid_amount;
    std::vector<double>      diff_returns;
    RunningMomentState       bid_amount_moments;
};
```

新增工作区：

```cpp
struct Factor032_040Workspace {
    std::vector<double> diff_returns_work;
};
```

新增批量结果：

```cpp
struct Factor032_040Result {
    double factor032;
    double factor033;
    double factor034;
    double factor035;
    double factor036;
    double factor037;
    double factor038;
    double factor039;
    double factor040;
};
```

批量接口：

```cpp
void build_factor032_040_state(const FactorInput&, Factor032_040State&);
Factor032_040Result collect_factor032_040(const FactorInput&);
Factor032_040Result collect_factor032_040(
    const FactorInput&,
    const Factor032_040State&,
    Factor032_040Workspace&);
```

## 5. 复用的公共工具函数清单

以下函数定义在 `cross_section_utils.h` 的 `factor_detail` 命名空间中：

### 时间函数

| 函数 | 说明 |
|------|------|
| `convert_to_ms(int)` | HHMMSSmmm → 毫秒 |
| `convert_to_time(int)` | 毫秒 → HHMMSSmmm |
| `time_sub(a, b)` | 午休跨段时间减法（返回毫秒） |
| `time_add(a, b)` | 午休跨段时间加法 |

### 统计函数

| 函数 | 说明 |
|------|------|
| `mean(vector<double>)` | 均值 |
| `stddev(vector<double>)` | 样本标准差（n-1） |
| `skew(vector<double>)` | 样本偏度 |
| `kurt(vector<double>)` | 样本超额峰度 |
| `quantile_in_place(vector<double>&, p)` | 原地选择 + 线性插值分位数 |
| `quantile(vector<double>, p)` | 拷贝后计算分位数 |
| `median(vector<double>)` | 中位数（= quantile 0.5） |

### 截面扫描函数

| 函数 | 说明 |
|------|------|
| `summarize_positive_bid_amounts_at_time(cs, time)` | 指定时刻正值买额摘要（首值/末值/计数） |
| `count_cross_section_in_window(cs, begin, end)` | 窗口内截面条数 |
| `max_bid_amount_with_time_in_window(cs, begin, end)` | 窗口内最大买额及其时刻 |
| `last_bid_amount_in_window(cs, begin, end, &out)` | 窗口内最后一条买额 |
| `collect_bid_amounts_in_window(cs, begin, end)` | 提取窗口内买额序列 |
| `calc_diff_returns_in_window(cs, begin, end)` | 窗口内差分收益率序列 |

## 6. 设计要点

### 6.1 一次扫描构建状态

`build_factor032_040_state()` 对 `cross_section` 只做一次线性扫描，同时得到：

- `factor032/033` 的时点摘要
- `factor034/035` 的窗口计数
- `factor036/037` 的 `diff_returns`
- `factor038/039` 的峰值与末值
- `factor040` 的在线偏度矩

不再让 `call_factor032()` 到 `call_factor040()` 分别重复扫描窗口。

### 6.2 差分收益率的边界处理

`calc_diff_returns_in_window()` 的第一个元素固定为 `0.0`（对应 Python 的 `fillna(0)`）。当前一条 `bid_amount == 0` 时返回 `inf`。

新实现沿用相同语义：

- 窗口第一条 diff return 固定为 `0.0`
- 当前一条 `bid_amount == 0` 且当前值非 `0` 时，返回 `inf`

### 6.3 分位数只保留一份样本

`factor036` 和 `factor037` 共用一份 `diff_returns` 样本。

- `factor036`：在 workspace 上做 `quantile_in_place`
- `factor037`：复用同一个 `var_95`，再线性扫描原始 `diff_returns`

避免了两次窗口扫描和两次样本构造。

### 6.4 factor040 改为在线矩统计

`factor040` 不再通过 `collect_bid_amounts_in_window()` 额外构造 `bid_amounts` 向量，而是在状态构建时直接在线维护偏度所需矩统计。

### 6.5 分位数使用选择算法

`quantile_in_place` 仍使用选择算法，不做全量排序。

## 7. 与 factor001-031 的关系

factor001-003 也依赖 `cross_section`，但它们的截面读取已集成到 `factor01_031_stream.h` 的 `scan_cross_section()` 中，不走本组的公共函数。

现在两组的关系是：

- `001-031`：`order/trade/cross_section` 混合状态机
- `032-040`：纯 `cross_section` 批量状态机

两者都支持：

- 先 `build state`
- 再 `collect result`

从接口形态上已经统一。
