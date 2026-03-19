//
// Created by even on 2026/3/17.
//

#ifndef LIMIT_UP_FACTOR_DEMO_FACTOR_H
#define LIMIT_UP_FACTOR_DEMO_FACTOR_H

#include <cstdint>

typedef enum {
    FACTOR_BSFLAG_BUY = 1,  // 买单
    FACTOR_BSFLAG_SELL = 2, // 卖单
} FACTOR_BSFLAG;

typedef enum {
    FACTOR_ORDER_TYPE_NORMAL = 1,   // 常规委托单
    FACTOR_ORDER_TYPE_WITHDRAW = 2, // 撤单
} FACTOR_ORDER_TYPE;

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
    FactorOrderInfo         last_order;
    FactorTransInfo         last_transition;
    FactorBasicInfo         basic_info;
} FactorInput;

#endif //LIMIT_UP_FACTOR_DEMO_FACTOR_H
