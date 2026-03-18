//
// Created by Codex on 2026/3/18.
//

#ifndef LIMIT_UP_FACTOR_DEMO_FACTOR01_031_STREAM_H
#define LIMIT_UP_FACTOR_DEMO_FACTOR01_031_STREAM_H

#include <cstddef>

#include "cross_section_utils.h"
#include "factor01_031_state.h"

namespace factor01_031_detail {

inline double raw_amount(const int64_t price, const int volume) {
    return static_cast<double>(price) * static_cast<double>(volume);
}

inline bool is_in_window(const int time, const FactorBasicInfo& basic_info) {
    return time >= basic_info.lock_time && time <= basic_info.predict_t;
}

inline void reset_factor01_031_state(Factor01_031State& state) {
    state.book.has_lock_bid_amount = false;
    state.book.has_predict_bid_amount = false;
    state.book.has_max_bid_amount = false;
    state.book.lock_bid_amount = 0.0;
    state.book.predict_bid_amount = 0.0;
    state.book.max_bid_amount_until_predict_t = 0.0;

    state.window.has_first_order_time = false;
    state.window.has_first_trade_time = false;
    state.window.has_lock_order = false;
    state.window.has_early70_window = false;
    state.window.has_limit_up_cancel_sample = false;
    state.window.has_limit_up_trade_sample = false;
    state.window.first_order_time = 0;
    state.window.last_order_time = 0;
    state.window.order_count = 0;
    state.window.first_trade_time = 0;
    state.window.last_trade_time = 0;
    state.window.trade_count = 0;
    state.window.limit_up_add_count = 0;
    state.window.limit_up_cancel_count = 0;
    state.window.max_limit_up_cancel_volume = 0;
    state.window.max_limit_up_trade_volume = 0;
    state.window.early70_limit_up_buy_volume_sum = 0;
    state.window.early70_limit_up_buy_count = 0;
    state.window.non_limit_cancel_amount = 0.0;
    state.window.non_limit_cancel_count = 0;
    state.window.non_limit_add_amount = 0.0;
    state.window.non_limit_add_count = 0;
    state.window.limit_up_trade_count = 0;
    state.window.non_limit_trade_count = 0;
    state.window.limit_up_trade_volume_sum = 0.0;
    state.window.limit_up_trade_amount_sum = 0.0;
    state.window.limit_up_sell_volume_sum = 0;
    state.window.limit_up_sell_bid_orders.clear();
    state.window.limit_up_sell_ask_orders.clear();
    state.window.lock_order_volume = 0;
    state.window.lock_order_amount = 0.0;

    state.order_lifecycle.clear();
    state.quantiles.limit_up_add_samples.clear();
    state.quantiles.limit_up_trade_volumes.clear();
    state.lock_trade.has_trade = false;
    state.lock_trade.first_trade_time = 0;
    state.lock_trade.last_trade_time = 0;

    state.has_lock_order_id = false;
    state.has_self_order_id = false;
    state.has_last_limit_up_sell_bid_order = false;
    state.has_first_limit_cancel_order = false;
    state.lock_order_id = 0;
    state.self_order_id = 0;
    state.last_limit_up_sell_bid_order = 0;
    state.first_limit_cancel_order_id = 0;
    state.limit_up_cancel_order_ids_in_window.clear();
}

inline OrderLifecycleState& get_order_lifecycle(
    Factor01_031State& state,
    const int order_id) {
    OrderLifecycleState& lifecycle = state.order_lifecycle[order_id];
    if (!lifecycle.seen_any) {
        lifecycle.factor004_candidate = false;
        lifecycle.has_limit_up_cancel_in_window = false;
        lifecycle.first_time = 0;
        lifecycle.last_time = 0;
        lifecycle.first_price = 0;
        lifecycle.first_volume = 0;
        lifecycle.add_amount_until_predict_t = 0.0;
        lifecycle.cancel_amount_until_predict_t = 0.0;
        lifecycle.traded_amount_until_predict_t = 0.0;
    }
    return lifecycle;
}

inline void scan_cross_section(
    const FactorInput& input,
    Factor01_031State& state) {
    for (std::vector<FactorCrossSectionInfo>::const_iterator it = input.cross_section.begin();
         it != input.cross_section.end(); ++it) {
        if (it->time == input.basic_info.lock_time) {
            state.book.has_lock_bid_amount = true;
            state.book.lock_bid_amount = it->bid_amount;
        }
        if (it->time == input.basic_info.predict_t) {
            state.book.has_predict_bid_amount = true;
            state.book.predict_bid_amount = it->bid_amount;
        }
        if (it->time <= input.basic_info.predict_t &&
            (!state.book.has_max_bid_amount ||
             it->bid_amount > state.book.max_bid_amount_until_predict_t)) {
            state.book.has_max_bid_amount = true;
            state.book.max_bid_amount_until_predict_t = it->bid_amount;
        }
    }
}

inline void scan_orders(
    const FactorInput& input,
    Factor01_031State& state) {
    state.order_lifecycle.reserve(input.order_events.size());
    state.quantiles.limit_up_add_samples.reserve(input.order_events.size() / 4);
    state.limit_up_cancel_order_ids_in_window.reserve(input.order_events.size() / 8);

    state.window.has_early70_window =
        factor_detail::time_sub(input.basic_info.predict_t, input.basic_info.lock_time) >= 70;
    const int early70_end_time = factor_detail::time_add(input.basic_info.lock_time, 70);

    for (std::vector<FactorOrderEvent>::const_iterator it = input.order_events.begin();
         it != input.order_events.end(); ++it) {
        if (!state.has_self_order_id && it->time > input.basic_info.t) {
            state.has_self_order_id = true;
            state.self_order_id = it->order_id;
        }

        if (!state.has_lock_order_id && it->biz_index == input.basic_info.lock_id) {
            state.has_lock_order_id = true;
            state.lock_order_id = it->order_id;
            state.window.has_lock_order = true;
            state.window.lock_order_volume = it->volume;
            state.window.lock_order_amount = raw_amount(it->price, it->volume);
        }

        OrderLifecycleState& lifecycle = get_order_lifecycle(state, it->order_id);
        if (!lifecycle.seen_any) {
            lifecycle.seen_any = true;
            lifecycle.first_time = it->time;
            lifecycle.first_price = it->price;
            lifecycle.first_volume = it->volume;
        }
        lifecycle.last_time = it->time;

        if (it->time <= input.basic_info.predict_t) {
            if (it->order_type == FACTOR_ORDER_TYPE_NORMAL) {
                lifecycle.add_amount_until_predict_t += raw_amount(it->price, it->volume);
            } else if (it->order_type == FACTOR_ORDER_TYPE_WITHDRAW) {
                lifecycle.cancel_amount_until_predict_t += raw_amount(it->price, it->volume);
            }
        }

        if (!is_in_window(it->time, input.basic_info)) {
            continue;
        }

        if (!state.window.has_first_order_time) {
            state.window.has_first_order_time = true;
            state.window.first_order_time = it->time;
        }
        state.window.last_order_time = it->time;
        ++state.window.order_count;

        const bool is_limit_up_price = it->price == input.basic_info.limit_up;
        if (it->order_type == FACTOR_ORDER_TYPE_NORMAL) {
            if (is_limit_up_price) {
                ++state.window.limit_up_add_count;
                state.quantiles.limit_up_add_samples.push_back(
                    Factor12AddSample{it->order_id, it->volume});
                if (state.window.has_early70_window &&
                    it->time <= early70_end_time &&
                    it->function_code == FACTOR_FUNCTION_CODE_BUY) {
                    state.window.early70_limit_up_buy_volume_sum +=
                        static_cast<int64_t>(it->volume);
                    ++state.window.early70_limit_up_buy_count;
                }
                if (it->function_code == FACTOR_FUNCTION_CODE_BUY) {
                    lifecycle.factor004_candidate = true;
                }
            } else {
                state.window.non_limit_add_amount += raw_amount(it->price, it->volume);
                ++state.window.non_limit_add_count;
            }
        } else if (it->order_type == FACTOR_ORDER_TYPE_WITHDRAW) {
            if (is_limit_up_price) {
                ++state.window.limit_up_cancel_count;
                if (!lifecycle.has_limit_up_cancel_in_window) {
                    lifecycle.has_limit_up_cancel_in_window = true;
                    state.limit_up_cancel_order_ids_in_window.push_back(it->order_id);
                    if (!state.has_first_limit_cancel_order) {
                        state.has_first_limit_cancel_order = true;
                        state.first_limit_cancel_order_id = it->order_id;
                    }
                }
                if (!state.window.has_limit_up_cancel_sample ||
                    it->volume > state.window.max_limit_up_cancel_volume) {
                    state.window.has_limit_up_cancel_sample = true;
                    state.window.max_limit_up_cancel_volume = it->volume;
                }
            } else {
                state.window.non_limit_cancel_amount += raw_amount(it->price, it->volume);
                ++state.window.non_limit_cancel_count;
            }
        }
    }
}

inline void scan_trades(
    const FactorInput& input,
    Factor01_031State& state) {
    state.quantiles.limit_up_trade_volumes.reserve(input.trade_events.size() / 4);
    state.window.limit_up_sell_bid_orders.reserve(input.trade_events.size() / 4);
    state.window.limit_up_sell_ask_orders.reserve(input.trade_events.size() / 4);

    for (std::vector<FactorTradeEvent>::const_iterator it = input.trade_events.begin();
         it != input.trade_events.end(); ++it) {
        if (state.has_lock_order_id && it->bid_order_id == state.lock_order_id) {
            if (!state.lock_trade.has_trade) {
                state.lock_trade.has_trade = true;
                state.lock_trade.first_trade_time = it->time;
            }
            state.lock_trade.last_trade_time = it->time;
        }

        if (it->time <= input.basic_info.predict_t &&
            it->bs_flag == FACTOR_BSFLAG_SELL) {
            std::unordered_map<int, OrderLifecycleState>::iterator order_it =
                state.order_lifecycle.find(it->bid_order_id);
            if (order_it != state.order_lifecycle.end()) {
                order_it->second.traded_amount_until_predict_t +=
                    raw_amount(it->price, it->volume);
            }
        }

        if (!is_in_window(it->time, input.basic_info)) {
            continue;
        }

        if (!state.window.has_first_trade_time) {
            state.window.has_first_trade_time = true;
            state.window.first_trade_time = it->time;
        }
        state.window.last_trade_time = it->time;
        ++state.window.trade_count;

        const bool is_limit_up_price = it->price == input.basic_info.limit_up;
        if (is_limit_up_price) {
            ++state.window.limit_up_trade_count;
            state.window.limit_up_trade_volume_sum += static_cast<double>(it->volume);
            state.window.limit_up_trade_amount_sum += raw_amount(it->price, it->volume);
            state.quantiles.limit_up_trade_volumes.push_back(static_cast<double>(it->volume));

            if (it->bs_flag == FACTOR_BSFLAG_SELL) {
                state.has_last_limit_up_sell_bid_order = true;
                state.last_limit_up_sell_bid_order = it->bid_order_id;
                state.window.limit_up_sell_volume_sum += static_cast<int64_t>(it->volume);
                state.window.limit_up_sell_bid_orders.insert(it->bid_order_id);
                state.window.limit_up_sell_ask_orders.insert(it->ask_order_id);
                if (!state.window.has_limit_up_trade_sample ||
                    it->volume > state.window.max_limit_up_trade_volume) {
                    state.window.has_limit_up_trade_sample = true;
                    state.window.max_limit_up_trade_volume = it->volume;
                }
            }
        } else {
            ++state.window.non_limit_trade_count;
        }
    }
}

inline void build_factor01_031_state(
    const FactorInput& input,
    Factor01_031State& state) {
    reset_factor01_031_state(state);
    scan_cross_section(input, state);
    scan_orders(input, state);
    scan_trades(input, state);
}

inline Factor01_031State build_factor01_031_state(const FactorInput& input) {
    Factor01_031State state;
    build_factor01_031_state(input, state);
    return state;
}

} // namespace factor01_031_detail

#endif //LIMIT_UP_FACTOR_DEMO_FACTOR01_031_STREAM_H
