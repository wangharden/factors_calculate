# Python 因子转 C++ 施工稿

## 1. 本轮对齐结论

本项目不建议在 C++ 中复刻一个通用 `DataFrame`。

建议路线是：

- 保留 `order` / `trade` 的行情 API 原始字段结构
- 单独设计 `cross_section` 的强类型序列结构
- 把 Python 中常见的 `DataFrame` 操作，沉淀成面向因子场景的公共函数
- 因子函数只负责调用这些公共函数，不直接拼接复杂数据流程

这样更适合高频因子场景，原因是：

- 输入字段固定，不需要通用表计算框架
- 顺序扫描和按订单号回查是主要模式，专用结构更容易做性能优化
- 可以逐步从“先可用”演进到“少拷贝、少分配、少重复扫描”

## 2. 数据结构需求汇总

### 2.1 `cross_section` 最小必需字段与主表示

根据当前 notebook 中 `factor001-042` 的引用，`cross_section` 只用到了两个字段：

- `time: int`
  - 交易所时间，格式与现有 `n_time` 一致，按 `HHMMSSmmm` 存储
- `bid_amount: double`
  - 对应 Python 中的 `委买总额`

建议结构：

```cpp
struct FactorCrossSectionInfo {
    int time;
    double bid_amount;
};
```

阶段 A 的主表示固定为 AoS：

```cpp
std::vector<FactorCrossSectionInfo> cross_section;
```

说明：

- 当前 `FactorInput` 的标准输入继续使用 AoS
- 原因是现阶段因子以顺序扫描、按时间窗口过滤、按单条记录同时读取 `time` 与 `bid_amount` 为主
- 这与当前实现一致，改动面最小
- 目前已确认的 `cross_section` 列只有 `time` 和 `委买总额`
- 后续如果别的因子确实需要更多盘口聚合字段，再扩充结构

### 2.2 `CrossSectionSoA` 设计定位

为后续 SIMD / 批量计算预留 SoA 设计，但阶段 A 不作为 `FactorInput` 的主输入。

建议结构：

```cpp
struct CrossSectionSoA {
    std::vector<int> time;
    std::vector<double> bid_amount;
};
```

定位说明：

- SoA 面向后续批量时间过滤、单列统计和 SIMD 优化
- 阶段 A 只定义类型，不引入 AoS / SoA 双存储同步规则
- 等 `cross_section` 成为确认的热点路径后，再决定是否切换主表示或增加转换层

### 2.3 `order` 已确认会用到的字段

当前 notebook 中 `order` 用到的字段只有：

- `time`
- `orderPrice`
- `orderVolume`
- `orderOriNo`
- `orderKind`
- `functionCode`
- `bizIndex`

这些字段已经能覆盖当前 `factor001-050` 的主要订单侧需求。

### 2.4 `trade` 已确认会用到的字段

当前 notebook 中 `trade` 用到的字段只有：

- `time`
- `tradePrice`
- `tradeVolume`
- `bsFlag`
- `bidOrder`
- `askOrder`

这些字段已经能覆盖当前 `factor004`、`factor009`、`factor017-027`、`factor043-046`、`factor053-061` 的成交侧需求。

## 3. 60 个因子的公共操作清单

下面这些不是“某一个因子”的逻辑，而是整个迁移工程反复会用到的公共方法。

### 3.1 时间工具

- `convert_to_ms(int hhmmssmmm) -> int`
- `time_sub(int a, int b) -> int`
  - 需要处理午休跨段
- `time_add(int a, int delta_ms) -> int`

### 3.2 `cross_section` 公共操作

- 按精确时刻筛选
  - `time == lock_time`
  - `time == predict_t`
- 按时间窗口筛选
  - `[lock_time, predict_t]`
  - `[lock_time, lock_time + 100ms]`
- 对 `bid_amount` 做统计
  - `first`
  - `last`
  - `max`
  - `count`
  - `mean`
  - `skew`
  - `quantile`
- 对 `bid_amount` 做相邻收益率变换
  - `x / shift(1) - 1`
- 找峰值及峰值对应时间

对应因子：

- `factor001-003`
- `factor032-042`

### 3.3 `order` 公共操作

- 按时间窗口筛选
- 按 `orderPrice == limit_up / != limit_up` 筛选
- 按 `orderKind == A / D` 筛选
- 按 `functionCode == B` 筛选
- 按 `bizIndex == lock_id` 精确查找封板单
- 按 `orderOriNo` 回查整笔订单生命周期
- 从订单生命周期中取
  - 首条时间
  - 末条时间
  - 首笔挂单量
- 对 `orderVolume` / `orderPrice * orderVolume` 做
  - `sum`
  - `max`
  - `count`
  - `quantile`
- 提取“首次满足条件的撤单订单”

对应因子：

- `factor004-008`
- `factor010`
- `factor012-016`
- `factor021-022`
- `factor028-031`
- `factor041`
- `factor047-050`

### 3.4 `trade` 公共操作

- 按时间窗口筛选
- 按 `tradePrice == limit_up / != limit_up` 筛选
- 按 `bsFlag == S` 筛选
- 按 `bidOrder` / `askOrder` 去重计数
- 按 `bidOrder` 回查某笔订单成交轨迹
- 对 `tradeVolume` / `tradePrice * tradeVolume` 做
  - `sum`
  - `mean`
  - `max`
  - `count`
  - `quantile`
- 计算累计 VWAP 序列
  - `cumsum(price * volume) / cumsum(volume)`
- 对 VWAP 序列做
  - `mean`
  - `std`
  - `skew`
  - `kurt`
  - `median`
  - `quantile`
  - `min`
  - `max`

对应因子：

- `factor004`
- `factor009`
- `factor011`
- `factor017-027`
- `factor043-046`
- `factor053-061`

### 3.5 连接 / 索引类操作

这部分是 Python `DataFrame` 迁移到 C++ 最容易卡住的地方，需要提前设计。

- `orderOriNo -> 订单生命周期`
- `bidOrder -> 成交集合`
- `askOrder -> 成交集合`
- `bizIndex -> 封板订单`
- 在订单窗口内按 `orderOriNo` 做排序或定位
- 支持 `searchsorted` 风格的位置计算
  - 主要用于 `factor012`

## 4. `cross_section` 对应的成员和函数需求

### 4.1 成员需求

当前先收敛为：

```cpp
struct FactorCrossSectionInfo {
    int time;
    double bid_amount;
};
```

### 4.2 建议先实现的公共函数

- `filter_by_exact_time(cross_section, time)`
- `scan_by_window(cross_section, begin_time, end_time)`
- `count_in_window(...)`
- `max_bid_amount_in_window(...)`
- `max_bid_amount_with_time(...)`
- `first_bid_amount_at_time(...)`
- `last_bid_amount_at_time(...)`
- `diff_returns_in_window(...)`
- `quantile(...)`
- `skew(...)`

说明：

- 如果以性能优先，优先写“扫描版”函数，而不是先切片再统计
- `factor036-040` 这类时序因子尽量避免生成中间窗口副本
- `quantile / median` 的数值语义与 notebook 中的 pandas 默认行为对齐，采用线性插值

## 5. 建议的施工顺序

### 阶段 A：`cross_section` 基础层

- 统一时间工具：`convert_to_ms`、`time_sub`、`time_add`
- 统一统计工具：`mean`、`std`、`skew`、`kurt`、`quantile`、`median`
- 统一 `cross_section` 扫描工具：
  - 精确时刻提取首值 / 末值 / 正值摘要
  - 窗口计数
  - 窗口峰值及峰值时间
  - 窗口末值
  - 窗口差分收益率
- 将 `factor032-040` 改为全部复用公共层
- 本阶段不接入 `order` / `trade` 全量事件序列

### 阶段 B：先做纯扫描型因子

优先做不依赖复杂连接和生命周期回查的因子：

- `factor001-003`
- `factor005-011`
- `factor013-020`
- `factor024-027`
- `factor032-046`
- `factor053-061`

### 阶段 C：再做需要订单生命周期和索引的因子

这批因子更依赖 `orderOriNo` 级别的回查：

- `factor004`
- `factor012`
- `factor021-023`
- `factor028-031`
- `factor047-050`

### 阶段 D：针对热点做性能优化

- 消除窗口切片临时 `vector`
- 尽量复用统计 buffer
- 对 `orderOriNo`、`bidOrder`、`askOrder` 建索引
- 仅在确认热点后再考虑固定容量数组替代 `vector`

## 6. 当前实现状态与后续建议

当前代码里已经落地：

- `cross_section` 最小结构
- `CrossSectionSoA` 设计型类型
- `factor032-040` 的第一版 C++ 实现

当前仍建议继续补的工程件：

- 面向 `order` / `trade` 的统一窗口扫描函数
- 按 `orderOriNo` 的生命周期索引
- 按 `bidOrder` / `askOrder` 的去重计数工具

## 7. 备注

- notebook 当前可见因子范围包含 `factor001-050`、`factor053-061`
- `factor051`、`factor052` 在当前草稿中未看到定义，需要后续补确认
- 涉及 `order` / `trade` 全量事件序列的因子本轮先不做
- 如果本项目本轮目标是“先把工程骨架搭起来”，那么下一步最合适的是把 `order` / `trade` 公共工具层补齐，而不是继续在单个因子里重复写筛选和统计逻辑

## 8. 对接清单：依赖全量 `order / trade` 序列的因子

### 8.1 依赖全量 `order`，但不依赖订单生命周期回溯

- `factor006`
- `factor007`
- `factor008`
- `factor010`
- `factor013`
- `factor014`
- `factor015`
- `factor016`
- `factor021`
- `factor022`
- `factor041`

### 8.2 依赖全量 `trade`，但不依赖订单生命周期回溯

- `factor009`
- `factor011`
- `factor017`
- `factor018`
- `factor019`
- `factor020`
- `factor024`
- `factor025`
- `factor026`
- `factor027`
- `factor043`
- `factor044`
- `factor045`
- `factor046`
- `factor053`
- `factor054`
- `factor055`
- `factor056`
- `factor057`
- `factor058`
- `factor059`
- `factor060`
- `factor061`

### 8.3 同时依赖全量 `order` 与 `trade`，但不依赖订单生命周期回溯

- 当前草稿中无明确仅靠窗口过滤即可完成、且同时依赖两者的因子

### 8.4 依赖订单生命周期回溯或订单级关联，后续再做

- `factor004`
- `factor012`
- `factor023`
- `factor028`
- `factor029`
- `factor030`
- `factor031`
- `factor047`
- `factor048`
- `factor049`
- `factor050`
