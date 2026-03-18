//
// Created by even on 2026/3/17.
//

#ifndef LIMIT_UP_FACTOR_DEMO_FACTOR_H
#define LIMIT_UP_FACTOR_DEMO_FACTOR_H

#include <cstdint>
#include <vector>

typedef enum {
    FACTOR_BSFLAG_BUY = 1,  // 买单
    FACTOR_BSFLAG_SELL = 2, // 卖单
} FACTOR_BSFLAG;

typedef enum {
    FACTOR_ORDER_TYPE_NORMAL = 1,   // 常规委托单
    FACTOR_ORDER_TYPE_WITHDRAW = 2, // 撤单
} FACTOR_ORDER_TYPE;

typedef enum {
    FACTOR_FUNCTION_CODE_UNKNOWN = 0,
    FACTOR_FUNCTION_CODE_BUY = 1,
    FACTOR_FUNCTION_CODE_SELL = 2,
} FACTOR_FUNCTION_CODE;

typedef struct {
    int                 predict_t;
    int                 t;  //自己单子的交易所时间戳
    int                 lock_time;
    int                 lock_id;
    int64_t             limit_up;   //10000*涨停价
} FactorBasicInfo;

typedef struct {

    int                 n_time;
    int64_t             n_price;
    int                 n_volume;
    int                 n_order_id;
    int                 n_index;

    FACTOR_BSFLAG       q_bs_flag;
    FACTOR_ORDER_TYPE   q_order_type;
    int                 q_delta_time;
    int                 q_last_time;

    int                 q_max_order_volume;
    int                 q_max_withdraw_volume;
    int64_t             q_order_cnt;

} FactorOrderInfo;

typedef struct {
    int             n_time;
    int64_t         n_price;
    int             n_volume;
    int             n_bid_order_id;
    int             n_ask_order_id;
    int             n_index;

    FACTOR_BSFLAG   q_bs_flag;
    int             q_delta_time;
    int             q_last_time;

    int             q_max_trade_volume;
    int64_t         q_trade_cnt;

} FactorTransInfo;

typedef struct {
    int             time;
    double          bid_amount; //委买总额
} FactorCrossSectionInfo;

typedef struct {
    int                 time;
    int64_t             price;
    int                 volume;
    int                 order_id;
    int                 biz_index;
    FACTOR_ORDER_TYPE   order_type;
    FACTOR_FUNCTION_CODE function_code;
} FactorOrderEvent;

typedef struct {
    int             time;
    int64_t         price;
    int             volume;
    int             bid_order_id;
    int             ask_order_id;
    FACTOR_BSFLAG   bs_flag;
} FactorTradeEvent;

typedef struct {
    std::vector<int>        time;
    std::vector<double>     bid_amount;
} CrossSectionSoA;

typedef struct {

    FactorOrderInfo last_order;
    FactorTransInfo last_transition;
    FactorBasicInfo basic_info;
    std::vector<FactorCrossSectionInfo> cross_section;
    std::vector<FactorOrderEvent> order_events;
    std::vector<FactorTradeEvent> trade_events;

} FactorInput;

#endif //LIMIT_UP_FACTOR_DEMO_FACTOR_H
