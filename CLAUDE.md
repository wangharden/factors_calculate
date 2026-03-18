# 涨停因子 C++ 高性能计算项目

## 项目概述

本项目将 Python notebook (`factor_calculator_draft.ipynb`) 中定义的涨停板因子计算逻辑，迁移为 C++ 高性能实现。

业务场景：A 股涨停板封单策略，在封板后实时计算多维因子，用于判断是否撤单。

## 因子总览

共 50 个因子（编号 001-050, 053-061），按计算依赖分为四组：

| 组别 | 因子范围 | 依赖数据 | 实现状态 | 设计文档 |
|------|----------|----------|----------|----------|
| 委托/成交事件流 | 001-031 | order + trade + cross_section | ✅ 已实现 | `docs/factor01_031_design.md` |
| 截面时序统计 | 032-040 | cross_section | ✅ 已实现 | `docs/factor032_040_design.md` |
| 订单/成交混合 | 041-050 | order + trade | ❌ 未实现 | `docs/factor041_050_design.md` |
| VWAP 比率统计 | 053-061 | trade | ✅ 已实现 | `docs/factor053_061_design.md` |

注：factor051/052 不存在于 notebook 定义中。

## 文件结构

### 核心头文件

| 文件 | 职责 |
|------|------|
| `factor.h` | 数据结构定义：`FactorInput`、`FactorOrderEvent`、`FactorTradeEvent`、`FactorCrossSectionInfo` 等 |
| `cross_section_utils.h` | 公共工具层：时间函数、统计函数（mean/std/skew/kurt/quantile）、截面扫描函数 |
| `factor01_031_state.h` | factor001-031 的运行时状态结构定义 |
| `factor01_031_stream.h` | factor001-031 的流式状态构建（扫描 order/trade/cross_section） |
| `factor01_031_extract.h` | factor001-031 的因子提取函数 |
| `trade_vwap_utils.h` | factor053-061 的 VWAP 比率流式统计 |
| `factor_collection.h` | 统一入口：所有因子的 `call_factorXXX()` 和批量 `collect_factorXXX_YYY()` |

### 其他文件

| 文件 | 说明 |
|------|------|
| `main.cpp` | 示例 main |
| `factor001.cpp` | 最小编译单元（占位） |
| `CMakeLists.txt` | CMake 构建（C++11） |
| `docs/优化思路.md` | 代码落地六大原则 |
| `docs/cpp_factor_migration_draft.md` | Python → C++ 迁移施工稿（数据结构、公共操作、施工顺序） |
| `docs/性能对比_01031_vs_032040.md` | 两种设计风格的性能对比分析 |

## 核心数据结构

```
FactorInput
├── basic_info: FactorBasicInfo
│   ├── predict_t        — 预测撤单时刻
│   ├── t                — 自己单子的交易所时间戳
│   ├── lock_time        — 封板时刻
│   ├── lock_id          — 封板单 bizIndex
│   └── limit_up         — 涨停价 × 10000
├── order_events: vector<FactorOrderEvent>  — 全量逐笔委托
├── trade_events: vector<FactorTradeEvent>  — 全量逐笔成交
├── cross_section: vector<FactorCrossSectionInfo> — 截面快照序列
├── last_order: FactorOrderInfo              — 最新一条委托（实时字段）
└── last_transition: FactorTransInfo         — 最新一条成交（实时字段）
```

### 价格与金额约定

- `limit_up` / `n_price`：原始价格 × 10000（int64_t），避免浮点
- 金额单位：原始价格 × 数量 / 10000 = 万元
- 所有 `scaled_amount()` 操作统一除以 10000.0

### 时间格式

- 整数 `HHMMSSmmm`，如 `93000000` = 09:30:00.000
- 午休：11:30:00 ~ 13:00:00 需做跨段处理
- 工具函数：`convert_to_ms()` / `convert_to_time()` / `time_sub()` / `time_add()`

## 架构模式

### 1. 流式状态机（factor001-031, factor053-061）

```
输入事件序列 → 单遍扫描 → 状态累积 → 因子提取
```

- 不保留全量原始事件，只维护聚合状态
- 状态分层：`BookSnapshotState` / `WindowAggState` / `OrderLifecycleState` / `QuantileState`
- 订单生命周期按 `order_id` 索引（`unordered_map`）

### 2. 截面顺序扫描（factor032-040）

```
cross_section 序列 → 窗口过滤 → 统计计算
```

- 不构造窗口子序列，直接在原始 AoS 上顺序扫描
- 差分收益率、峰值扫描、VaR 计算均内联完成

### 3. 统一入口（factor_collection.h）

- 单因子调用：`call_factorXXX(input, result)`
- 批量调用：`collect_factor001_031(input)` → `Factor01_031Result`

## 代码落地原则（摘要）

完整版见 `docs/优化思路.md`。

1. **AoS 主表示**：`cross_section` 固定为 `vector<FactorCrossSectionInfo>`，不引入 SoA 双存储
2. **原始数据只读**：不允许对输入做排序/重排/覆盖写入
3. **一遍扫描**：能在一次遍历中完成的窗口操作不先构造子序列
4. **分位数对齐 pandas**：使用线性插值，`quantile_in_place` 只作用于工作区
5. **最小拷贝**：新增临时 vector 需有理由，优先复用工作区
6. **header-only inline**：性能关键路径全部内联，避免跨编译单元开销

## 构建方式

### CMake（推荐）

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### 直接编译

```bash
g++ -std=c++14 -g main.cpp factor001.cpp -o factor_demo
```

## 当前进度

### 已完成

- [x] 核心数据结构（`factor.h`）
- [x] 时间工具、统计工具、截面扫描工具（`cross_section_utils.h`）
- [x] factor001-031 流式状态机（state / stream / extract 三层）
- [x] factor032-040 截面时序因子
- [x] factor053-061 VWAP 比率流式统计
- [x] 统一入口 `factor_collection.h`

### 待完成

- [ ] factor041-050 订单/成交混合因子（设计稿已就绪：`docs/factor041_050_design.md`）
- [ ] 与行情系统的集成测试
- [ ] 与 Python notebook 的数值对齐验证

## 数值语义约定

| 场景 | 约定 |
|------|------|
| 无数据 / 不可计算 | 返回 `NaN`（`std::numeric_limits<double>::quiet_NaN()`） |
| 纯计数量为零 | 返回 `0.0` |
| 分位数 | 线性插值，与 pandas `interpolation="linear"` 对齐 |
| 偏度 / 峰度 | 样本统计（n-1 / n-2 / n-3 校正） |
| 时间差 | 毫秒（int），午休跨段自动处理 |
| factor028-030 的 "< 10ms" | 按 `time_diff == 0` 对齐（数据最小粒度 10ms） |

## 给 AI 编码助手的指引

### 新增因子时

1. 先在 notebook 中确认 Python 原始逻辑
2. 确定因子属于哪个分组，对照对应设计文档
3. 如果依赖新的状态字段，在 `*_state.h` 中增加
4. 在 `*_stream.h` 的扫描函数中更新状态
5. 在 `*_extract.h` 中实现提取函数
6. 在 `factor_collection.h` 中增加 `call_factorXXX` 薄封装
7. 遵守 `docs/优化思路.md` 中的六大原则

### 常见陷阱

- notebook 中 `orderOriNo` 对应 C++ 的 `order_id`，不是 `biz_index`
- notebook 中 `bizIndex` 对应 C++ 的 `biz_index`，仅封板单定位使用
- `orderKind == 'A'` 对应 `FACTOR_ORDER_TYPE_NORMAL`
- `orderKind == 'D'` 对应 `FACTOR_ORDER_TYPE_WITHDRAW`
- `functionCode == 'B'` 对应 `FACTOR_FUNCTION_CODE_BUY`
- `bsFlag == 'S'` 对应 `FACTOR_BSFLAG_SELL`
- Python 中 `orderPrice` 直接是价格，C++ 中 `n_price` / `limit_up` 已乘 10000
