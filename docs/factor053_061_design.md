# factor053-061 设计稿

## 1. 范围

本稿只覆盖 `factor053` 到 `factor061`。

对应 Python 定义：

- `factor053`：`9:30` 到 `lock_time` 之间 `trade` 的 `vwap_ratio` 的 `mean`
- `factor054`：`std`
- `factor055`：`skew`
- `factor056`：`kurt`
- `factor057`：`q25`
- `factor058`：`median`
- `factor059`：`q75`
- `factor060`：`min`
- `factor061`：`max`

其中：

```text
vwap_ratio = cumulative_vwap / limit_up
           = (cumsum(trade_price * trade_volume) / cumsum(trade_volume)) / limit_up
```

## 2. 输入依赖

这组因子不依赖 `order`，只依赖：

- 全量逐笔 `trade`
- `lock_time`
- `limit_up`

最小成交字段需求：

- `time`
- `tradePrice`
- `tradeVolume`

可选但本组因子不需要的字段：

- `bsFlag`
- `bidOrder`
- `askOrder`

## 3. 计算模式

这组因子适合按逐笔成交流式更新。

每来一笔满足：

- `93000000 <= time <= lock_time`

的成交，就更新：

```text
cum_amount += trade_price * trade_volume
cum_volume += trade_volume
vwap_ratio = cum_amount / cum_volume / limit_up
```

然后把当前这一个 `vwap_ratio` 样本送入统计状态。

说明：

- `vwap_ratio` 是“到当前成交为止的累计 VWAP 比率”
- 样本序列长度等于窗口内成交笔数
- 这与 Python 里对整段 `trade` 做 `cumsum` 后再统计的口径一致

## 4. 状态设计

建议设计单独的运行时状态：

```cpp
struct VwapRatioStatsState {
    double      cum_amount;
    double      cum_volume;

    std::size_t count;

    double      mean;
    double      m2;
    double      m3;
    double      m4;

    double      min_value;
    double      max_value;

    std::vector<double> samples;
};
```

字段含义：

- `cum_amount`：累计成交额
- `cum_volume`：累计成交量
- `count / mean / m2 / m3 / m4`：在线维护一到四阶中心矩
- `min_value / max_value`：当前样本最值
- `samples`：仅用于精确分位数

## 5. 因子分组

### 5.1 可纯流式常数状态更新

这几项不需要保存全部样本：

- `factor053`：`mean`
- `factor054`：`std`
- `factor055`：`skew`
- `factor056`：`kurt`
- `factor060`：`min`
- `factor061`：`max`

### 5.2 可流式接收，但若要求精确值仍需保留样本

- `factor057`：`q25`
- `factor058`：`median`
- `factor059`：`q75`

实现方式：

- 每生成一个新的 `vwap_ratio`，追加到 `samples`
- 最终计算时对 `samples` 的工作区做离散分位数选择

说明：

- 当前要求是精确分位数，不做近似
- 采用离散分位数定义
- 不做线性插值
- 不允许直接修改原始输入，只能对工作区重排

## 6. 更新接口建议

建议提供三个层次的接口。

### 6.1 状态初始化

```cpp
void reset_vwap_ratio_stats(VwapRatioStatsState& state);
```

### 6.2 单笔成交推进

```cpp
void update_vwap_ratio_stats(
    VwapRatioStatsState& state,
    int time,
    int lock_time,
    int64_t trade_price,
    int trade_volume,
    int64_t limit_up);
```

行为要求：

- 若 `time < 93000000` 或 `time > lock_time`，直接忽略
- 若 `trade_volume <= 0` 或 `limit_up <= 0`，直接忽略
- 否则更新累计成交额、累计成交量和当前 `vwap_ratio`

### 6.3 因子读取

```cpp
double get_factor053(const VwapRatioStatsState& state);
double get_factor054(const VwapRatioStatsState& state);
double get_factor055(const VwapRatioStatsState& state);
double get_factor056(const VwapRatioStatsState& state);
double get_factor057(const VwapRatioStatsState& state);
double get_factor058(const VwapRatioStatsState& state);
double get_factor059(const VwapRatioStatsState& state);
double get_factor060(const VwapRatioStatsState& state);
double get_factor061(const VwapRatioStatsState& state);
```

### 6.4 批量结果结构

如果一次性输出 `factor053-061`，建议显式定义结果结构：

```cpp
struct VwapRatioFactorResult {
    double factor053;
    double factor054;
    double factor055;
    double factor056;
    double factor057;
    double factor058;
    double factor059;
    double factor060;
    double factor061;
};
```

## 7. 数值语义

### 7.1 空样本

若窗口内没有任何成交样本：

- 所有因子返回 `NaN`

### 7.2 单样本

若只有一个 `vwap_ratio` 样本：

- `mean/min/max/median/q25/q75` 返回该样本
- `std/skew/kurt` 返回 `NaN`

### 7.3 分位数

采用离散分位数定义：

- 使用 `index = round((n - 1) * p)`
- `p = 0.25 / 0.5 / 0.75`
- 不做线性插值

### 7.4 标准差 / 偏度 / 峰度

采用与当前工具层一致的样本统计定义：

- `std`：样本标准差
- `skew`：样本偏度
- `kurt`：样本超额峰度

## 8. 与全量 `trade` 序列的关系

这组因子虽然适合流式更新，但并不要求实现模块必须自己维护全量 `trade` 历史。

可以有两种落地方式：

### 方式 A：全量 `trade` 序列后处理

- 另一个模块提供全量 `trade`
- 因子模块顺序扫描一遍生成 `vwap_ratio` 样本并统计

### 方式 B：在线流式状态

- 逐笔成交到来时实时更新 `VwapRatioStatsState`
- 查询时直接输出结果

当前建议：

- 设计上优先支持方式 B
- 如果另一个模块先给的是全量 `trade` 序列，也允许顺序重放到同一套状态更新接口

## 9. 与现有工程的衔接建议

本组因子不建议直接堆到 `factor_collection.h` 内部实现。

建议拆成单独工具层：

- `trade_vwap_utils.h`

职责：

- 管理 `VwapRatioStatsState`
- 提供更新接口
- 提供 `factor053-061` 的读取接口
- 提供批量结果结构与批量导出函数

而 `factor_collection.h` 后续只保留薄封装。

## 10. 本稿结论

`factor053-061` 是当前整批因子里最适合走流式状态机的一组之一。

结论如下：

- 只依赖 `trade`，不依赖 `order`
- 不依赖订单生命周期
- `53-56, 60-61` 可纯流式常数状态更新
- `57-59` 可流式接收，但若要求精确分位数，需要保留 `vwap_ratio` 样本
- 建议引入 `VwapRatioFactorResult`，用于一次性导出 `53-061`
- 最合适的工程落点是单独的 `trade_vwap_utils` 工具层，而不是把统计逻辑直接塞进单个因子函数
