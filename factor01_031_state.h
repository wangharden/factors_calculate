//
// Created by Codex on 2026/3/18.
//

#ifndef LIMIT_UP_FACTOR_DEMO_FACTOR01_031_STATE_H
#define LIMIT_UP_FACTOR_DEMO_FACTOR01_031_STATE_H

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "factor.h"

namespace factor01_031_detail {

struct BookSnapshotState {
    bool    has_lock_bid_amount;
    bool    has_predict_bid_amount;
    bool    has_max_bid_amount;
    double  lock_bid_amount;
    double  predict_bid_amount;
    double  max_bid_amount_until_predict_t;
};

struct WindowAggState {
    bool        has_first_order_time;
    bool        has_first_trade_time;
    bool        has_lock_order;
    bool        has_early70_window;
    bool        has_limit_up_cancel_sample;
    bool        has_limit_up_trade_sample;

    int         first_order_time;
    int         last_order_time;
    int64_t     order_count;

    int         first_trade_time;
    int         last_trade_time;
    int64_t     trade_count;

    int64_t     limit_up_add_count;
    int64_t     limit_up_cancel_count;
    int         max_limit_up_cancel_volume;
    int         max_limit_up_trade_volume;
    int64_t     early70_limit_up_buy_volume_sum;
    int64_t     early70_limit_up_buy_count;

    double      non_limit_cancel_amount;
    int64_t     non_limit_cancel_count;
    double      non_limit_add_amount;
    int64_t     non_limit_add_count;

    int64_t     limit_up_trade_count;
    int64_t     non_limit_trade_count;
    double      limit_up_trade_volume_sum;
    double      limit_up_trade_amount_sum;
    int64_t     limit_up_sell_volume_sum;
    std::unordered_set<int> limit_up_sell_bid_orders;
    std::unordered_set<int> limit_up_sell_ask_orders;

    int         lock_order_volume;
    double      lock_order_amount;
};

struct OrderLifecycleState {
    bool        seen_any;
    bool        factor004_candidate;
    bool        has_limit_up_cancel_in_window;

    int         first_time;
    int         last_time;
    int64_t     first_price;
    int         first_volume;

    double      add_amount_until_predict_t;
    double      cancel_amount_until_predict_t;
    double      traded_amount_until_predict_t;
};

struct Factor12AddSample {
    int order_id;
    int volume;
};

struct QuantileState {
    std::vector<Factor12AddSample> limit_up_add_samples;
    std::vector<double>            limit_up_trade_volumes;
};

struct LockOrderTradeState {
    bool    has_trade;
    int     first_trade_time;
    int     last_trade_time;
};

struct Factor01_031State {
    BookSnapshotState                        book;
    WindowAggState                           window;
    std::unordered_map<int, OrderLifecycleState> order_lifecycle;
    QuantileState                            quantiles;
    LockOrderTradeState                      lock_trade;

    bool    has_lock_order_id;
    bool    has_self_order_id;
    bool    has_last_limit_up_sell_bid_order;
    bool    has_first_limit_cancel_order;

    int     lock_order_id;
    int     self_order_id;
    int     last_limit_up_sell_bid_order;
    int     first_limit_cancel_order_id;

    std::vector<int> limit_up_cancel_order_ids_in_window;
};

struct Factor01_031Workspace {
    std::vector<double>  add_quantile_work;
    std::vector<double>  trade_quantile_work;
    std::vector<int>     factor012_order_ids;
};

struct Factor01_031Result {
    double factor001;
    double factor002;
    double factor003;
    double factor004;
    double factor005;
    double factor006;
    double factor007;
    double factor008;
    double factor009;
    double factor010;
    double factor011;
    double factor012;
    double factor013;
    double factor014;
    double factor015;
    double factor016;
    double factor017;
    double factor018;
    double factor019;
    double factor020;
    double factor021;
    double factor022;
    double factor023;
    double factor024;
    double factor025;
    double factor026;
    double factor027;
    double factor028;
    double factor029;
    double factor030;
    double factor031;
};

struct Factor041_050Result {
    double factor041;
    double factor042;
    double factor043;
    double factor044;
    double factor045;
    double factor046;
    double factor047;
    double factor048;
    double factor049;
    double factor050;
};

} // namespace factor01_031_detail

#endif //LIMIT_UP_FACTOR_DEMO_FACTOR01_031_STATE_H
