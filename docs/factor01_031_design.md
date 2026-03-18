# factor001-031 设计稿

## 1. 范围

本稿只覆盖 `factor001` 到 `factor031`。

目标是回答一个工程问题：

- 这 31 个因子能否在 C++ 中按流式状态机落地
- 在什么边界下不需要维护 `order / trade` 的全量原始事件序列
- 分别需要哪些运行时状态

结论先行：

- `factor001-031` 可以按流式状态机落地
- 不需要维护 `vector<OrderEvent>` / `vector<TradeEvent>` 这类全量原始事件序列
- 但仍需要维护三类状态：
  - 窗口标量聚合状态
  - 按 `orderOriNo` 的订单生命周期状态
  - 精确分位数样本或等价频数结构

## 2. 适用前提

本设计成立的前提是：

- `order` 与 `trade` 输入可按时间顺序重放
- 计算目标是单个 `predict_t`，或多个单调递增的 `predict_t`
- 允许维护订单级状态和统计工作区，但不要求保留全部原始事件

如果后续要支持：

- 大量任意回看 `predict_t`
- 随机访问历史窗口
- 多次反复重算不同时间段

则应另行评估是否保留前缀索引或全量序列。

## 3. 总体结论

### 3.1 可纯流式常数状态完成

这类因子只需要 `count / sum / max / first / last` 一类状态：

- `factor005`
- `factor006`
- `factor007`
- `factor008`
- `factor009`
- `factor010`
- `factor011`
- `factor013`
- `factor014`
- `factor015`
- `factor016`
- `factor017`
- `factor018`
- `factor019`
- `factor020`
- `factor021`
- `factor022`

### 3.2 需要按订单号维护状态，但不需要全量事件回放

- `factor004`
- `factor023`
- `factor028`
- `factor029`
- `factor030`
- `factor031`

这类因子都可以通过：

- `unordered_map<orderOriNo, OrderState>`

完成，不需要把每条原始委托或成交永久保留下来。

### 3.3 需要精确分位数状态

- `factor012`
- `factor024`
- `factor025`
- `factor026`
- `factor027`

说明：

- 它们不要求全量原始事件序列
- 但如果要精确复现 Python 结果，必须保留样本分布信息
- 当前建议使用“追加样本 + 查询时工作区选择”

### 3.4 依赖在线盘口状态或截面快照

- `factor001`
- `factor002`
- `factor003`

这三个因子本质上依赖“某个时刻的委买总额”或“截至 `predict_t` 的最大委买总额”。

可选实现方式：

- 方式 A：沿用 `cross_section`
- 方式 B：在订单流上实时维护涨停价买盘队列金额，并在关键时刻采样

如果整体架构改为纯事件驱动，建议优先采用方式 B。

## 4. 运行时状态设计

建议拆成四块状态。

### 4.1 BookState

服务：

- `factor001`
- `factor002`
- `factor003`

建议字段：

```cpp
struct LimitUpBookState {
    double current_bid_amount;
    double lock_time_bid_amount;
    double predict_t_bid_amount;
    double max_bid_amount_until_predict_t;

    bool   has_lock_snapshot;
    bool   has_predict_snapshot;
    bool   has_max_value;
};
```

说明：

- 如果已有稳定的 `cross_section` 生成模块，本状态可以不单独实现
- 若直接基于 `order / trade` 流维护盘口，则该状态负责实时更新涨停价委买总额

### 4.2 WindowAggState

服务：

- `factor005-011`
- `factor013-022`

建议字段：

```cpp
struct WindowAggState {
    int first_order_time;
    int last_order_time;
    int64_t order_count;

    int first_trade_time;
    int last_trade_time;
    int64_t trade_count;

    int64_t limit_up_add_count;
    int64_t limit_up_cancel_count;
    int     max_limit_up_cancel_volume;
    int     max_limit_up_trade_volume;

    double  non_limit_cancel_amount;
    int64_t non_limit_cancel_count;
    double  non_limit_add_amount;
    int64_t non_limit_add_count;

    int64_t limit_up_trade_count;
    int64_t non_limit_trade_count;
    double  limit_up_trade_volume_sum;
    double  limit_up_trade_amount_sum;

    int64_t lock_order_volume;
    int64_t lock_order_amount;
};
```

说明：

- 除 `factor010`、`factor011` 的首末时间外，其余都可单笔增量更新
- `factor021`、`factor022` 也可以在识别到 `bizIndex == lock_id` 时直接锁定

### 4.3 OrderStateMap

服务：

- `factor004`
- `factor023`
- `factor028-031`
- `factor012` 的订单侧部分

建议字段：

```cpp
struct OrderLifecycleState {
    bool    seen_add;
    bool    seen_cancel;

    int     first_time;
    int     last_time;

    int64_t first_price;
    int     first_volume;

    int64_t add_amount_sum;
    int64_t cancel_amount_sum;
    int64_t traded_amount_sum;

    bool    is_limit_up_buy;
    bool    is_function_b;
};
```

核心用途：

- `factor004`
  - 对封板后新增的涨停价买单维护净委托额与累计成交额
- `factor023`
  - 只维护封板单 `orderOriNo` 的首笔成交时间和末笔成交时间
- `factor028-031`
  - 维护每个撤单订单的首挂时间、撤单时间、首挂量

### 4.4 QuantileState

服务：

- `factor012`
- `factor024-027`

建议字段：

```cpp
struct QuantileState {
    std::vector<double> limit_up_trade_volumes;
    std::vector<double> limit_up_add_volumes;
};
```

说明：

- `limit_up_trade_volumes` 用于 `factor024-027`
- `limit_up_add_volumes` 用于 `factor012` 的 `0.8` 分位阈值
- 当前数值语义需与 pandas 默认行为对齐，采用线性插值

如果后续存在大量在线查询，可再升级为：

- 值域压缩 + Fenwick 树
- 订单统计树

本稿不要求先做这一步。

## 5. 事件驱动执行流程

建议统一把 `order` 与 `trade` 包装成时间有序事件流：

```cpp
enum class StreamEventKind {
    Order,
    Trade
};
```

然后单遍推进状态：

1. 跳过 `time < lock_time` 的窗口外事件
2. 对 `time > predict_t` 的事件停止推进
3. `Order` 事件更新：
   - `BookState`
   - `WindowAggState`
   - `OrderStateMap`
   - `QuantileState.limit_up_add_volumes`
4. `Trade` 事件更新：
   - `BookState`
   - `WindowAggState`
   - `OrderStateMap`
   - `QuantileState.limit_up_trade_volumes`
5. 查询阶段统一导出 `factor001-031`

这个过程只保留状态，不保留原始事件历史。

## 6. 因子分组与实现建议

### 6.1 factor001-003

- `factor001`：封板时刻的封单额
- `factor002`：`predict_t` 时刻的封单额
- `factor003`：截至 `predict_t` 的最大封单额

实现建议：

- 若已有 `cross_section`，直接顺序扫描或在线摘要
- 若改成纯事件驱动，则在涨停价盘口状态上实时维护 `current_bid_amount`

### 6.2 factor004

Python 逻辑见：

- 选出 `[lock_time, predict_t]` 内涨停价、`orderKind == A`、`functionCode == B` 的 `orderOriNo`
- 汇总这些订单到 `predict_t` 为止的净委托额
- 再减去这些订单已成交额

建议状态：

- 对候选订单维护：
  - 首挂金额
  - 累计撤单金额
  - 累计成交金额

最终读取：

```text
remain_ratio = (net_order_amount - traded_amount) / net_order_amount
```

其中：

```text
net_order_amount = add_amount_sum - cancel_amount_sum
```

### 6.3 factor005-011

这组都是简单窗口统计：

- `005`：`predict_t - lock_time`
- `006`：涨停价挂单数
- `007`：涨停价撤单数
- `008`：涨停价最大撤单量
- `009`：涨停价最大成交量
- `010`：逐笔委托更新频率
- `011`：逐笔成交更新频率

全部可单遍流式更新，无需保留历史。

### 6.4 factor012

这是 `factor001-031` 中最特殊的一项。

它依赖：

- `[lock_time, predict_t]` 内涨停价新增挂单量的 `0.8` 分位数
- 当前自己单子的 `orderOriNo`
- 窗口内最后一笔涨停价卖出成交对应的 `bidOrder`

因此至少需要：

- 精确 `0.8` 分位数样本：`limit_up_add_volumes`
- `t_id`
- `last_limit_up_sell_bid_order`
- 一个“满足阈值的订单集合”的顺序定位能力

当前 notebook 存在一个语义风险：

- 代码使用 `np.searchsorted(temp_order['orderOriNo'], ...)`
- 但 `temp_order` 并未按 `orderOriNo` 显式排序

因此 C++ 落地前必须先固定语义。推荐解释为：

- 对满足条件的订单按 `orderOriNo` 升序排列
- 如果自己的订单未过 `0.8` 分位阈值，也强制纳入集合
- 结果为 `t_id` 与 `last_id` 在该有序集合中的相对位置差

如果不先澄清，跨语言结果可能不稳定。

### 6.5 factor013-022

这组都是简单窗口聚合：

- `013`：非涨停价撤单金额
- `014`：非涨停价撤单数
- `015`：非涨停价挂单金额
- `016`：非涨停价挂单数
- `017`：涨停价平均每笔成交量
- `018`：涨停价平均每笔成交金额
- `019`：涨停价成交笔数
- `020`：非涨停价成交笔数
- `021`：封板订单委托金额
- `022`：封板订单委托量

全部可流式完成。

### 6.6 factor023

定义：

- 封板订单 `orderOriNo` 对应成交轨迹的首末成交时间差

建议状态：

```cpp
struct LockOrderTradeState {
    bool has_trade;
    int  first_trade_time;
    int  last_trade_time;
};
```

只要逐笔成交流里能识别：

- `trade.bidOrder == lock_order_id`

就不需要保留封板单的全部成交历史。

### 6.7 factor024-027

定义：

- `[lock_time, predict_t]` 内涨停价成交量的 `q10 / q50 / q90 / q95`
- 最终再乘 `limit_up / 10000`

这组因子要求精确分位数。

当前建议：

- 每来一笔满足条件的成交，把 `tradeVolume` 追加到 `limit_up_trade_volumes`
- 查询时在工作区上调用线性插值 `quantile`

说明：

- 不需要保留全量 `trade` 事件
- 但需要保留分位数样本或等价频数结构

### 6.8 factor028-031

这组因子都围绕“涨停价订单从发单到撤单”的持续时间。

建议对每个 `orderOriNo` 维护：

- 首挂时间
- 最后事件时间
- 首挂量
- 是否已撤

然后在看到撤单事件时直接更新聚合结果。

建议读取结果：

- `028`：满足阈值条件的总撤单量
- `029`：满足阈值条件的总撤单金额
- `030`：满足阈值条件的总撤单数
- `031`：按首挂量加权的平均撤单时长

## 7. 与 notebook 注释不一致的口径风险

### 7.1 factor028-030

确认口径：

- 交易数据最小时间粒度是 `10ms`
- 因此 notebook 里使用 `time_diff == 0`，等价于“发单到撤单时间小于 `10ms`”

第一版 C++ 实现按这个口径对齐，不额外改成 `< 10`。

### 7.2 factor031

注释写的是：

- “涨停价订单加权平均撤单时间”

实际代码是：

- 对所有满足窗口条件的撤单订单计算持续时间
- 按首挂量加权平均
- 没有加 `< 10ms` 过滤

因此 `031` 不应默认继承 `028-030` 的时间阈值。

确认口径：

- `factor031` 按 notebook 实际代码对齐
- 即对窗口内全部涨停价撤单订单按首挂量做持续时间加权平均
- 不额外增加 `< 10ms` 过滤

## 8. 数值语义

### 8.1 空样本

建议约定：

- 无法定义的量返回 `NaN`
- 纯计数量可返回 `0`

### 8.2 平均值

对 `factor017`、`factor018`、`factor031`：

- 若分母为 `0`，按当前 Python 逻辑返回 `0` 或 `NaN` 需要逐项固定
- 第一版建议优先对齐 notebook 实际行为

### 8.3 分位数

当前工程约定已经调整为：

- 与 pandas `Series.quantile(..., interpolation="linear")` 对齐
- 使用线性插值

因此：

- `factor012` 的 `0.8` 阈值
- `factor024-027` 的 `0.1 / 0.5 / 0.9 / 0.95`

都应按线性插值实现。

## 9. 接口建议

建议把 `factor001-031` 拆成单独工具层，而不是直接在 `factor_collection.h` 里堆实现。

建议文件：

- `factor01_031_state.h`
- `factor01_031_stream.h`
- `factor01_031_extract.h`

职责建议：

- `factor01_031_state.h`
  - 定义 `BookState`
  - 定义 `WindowAggState`
  - 定义 `OrderLifecycleState`
  - 定义 `QuantileState`
- `factor01_031_stream.h`
  - 提供 `on_order(...)`
  - 提供 `on_trade(...)`
  - 提供 `reset(...)`
- `factor01_031_extract.h`
  - 提供 `get_factor001(...)` 到 `get_factor031(...)`
  - 或者一次性导出结果结构

## 10. 当前建议的落地顺序

1. 先实现 `WindowAggState`
2. 再实现 `OrderStateMap`
3. 再接入 `factor024-027` 的分位数工作区
4. 最后单独确认 `factor012` 的有序定位语义

原因：

- `005-011`、`013-022` 最直接，能先把流式框架跑通
- `004`、`023`、`028-031` 只增加订单状态，不涉及复杂数值问题
- `024-027` 只增加精确分位数
- `012` 同时依赖分位数和顺序定位，最容易出现口径偏差
