//
// Created by Codex on 2026/3/18.
//

#ifndef LIMIT_UP_FACTOR_DEMO_FACTOR01_031_EXTRACT_H
#define LIMIT_UP_FACTOR_DEMO_FACTOR01_031_EXTRACT_H

#include <algorithm>
#include <cmath>

#include "factor01_031_stream.h"

namespace factor01_031_detail {

inline double scaled_amount(const double raw_amount_value) {
    return raw_amount_value / 10000.0;
}

inline double sorted_linear_quantile(
    const std::vector<double>& sorted_values,
    const double p) {
    if (sorted_values.empty()) {
        return factor_detail::nan_value();
    }

    const double position = factor_detail::quantile_position(sorted_values.size(), p);
    const std::size_t lower_index = static_cast<std::size_t>(std::floor(position));
    const std::size_t upper_index = static_cast<std::size_t>(std::ceil(position));
    const double lower_value = sorted_values[lower_index];
    if (lower_index == upper_index) {
        return lower_value;
    }

    const double weight = position - static_cast<double>(lower_index);
    const double upper_value = sorted_values[upper_index];
    return lower_value + (upper_value - lower_value) * weight;
}

inline void compute_limit_up_trade_quantiles(
    const Factor01_031State& state,
    Factor01_031Workspace& workspace,
    double& q10,
    double& q50,
    double& q90,
    double& q95) {
    if (state.quantiles.limit_up_trade_volumes.empty()) {
        q10 = factor_detail::nan_value();
        q50 = factor_detail::nan_value();
        q90 = factor_detail::nan_value();
        q95 = factor_detail::nan_value();
        return;
    }

    workspace.trade_quantile_work.assign(
        state.quantiles.limit_up_trade_volumes.begin(),
        state.quantiles.limit_up_trade_volumes.end());
    std::sort(workspace.trade_quantile_work.begin(), workspace.trade_quantile_work.end());
    q10 = sorted_linear_quantile(workspace.trade_quantile_work, 0.10);
    q50 = sorted_linear_quantile(workspace.trade_quantile_work, 0.50);
    q90 = sorted_linear_quantile(workspace.trade_quantile_work, 0.90);
    q95 = sorted_linear_quantile(workspace.trade_quantile_work, 0.95);
}

inline double get_factor001(const Factor01_031State& state) {
    return state.book.has_lock_bid_amount ? state.book.lock_bid_amount : factor_detail::nan_value();
}

inline double get_factor002(const Factor01_031State& state) {
    return state.book.has_predict_bid_amount ?
        state.book.predict_bid_amount :
        factor_detail::nan_value();
}

inline double get_factor003(const Factor01_031State& state) {
    return state.book.has_max_bid_amount ?
        state.book.max_bid_amount_until_predict_t :
        factor_detail::nan_value();
}

inline double get_factor004(const Factor01_031State& state) {
    double total_amount = 0.0;
    double traded_amount = 0.0;
    for (std::unordered_map<int, OrderLifecycleState>::const_iterator it =
             state.order_lifecycle.begin();
         it != state.order_lifecycle.end(); ++it) {
        if (!it->second.factor004_candidate) {
            continue;
        }

        const double net_amount =
            it->second.add_amount_until_predict_t - it->second.cancel_amount_until_predict_t;
        total_amount += net_amount;
        traded_amount += it->second.traded_amount_until_predict_t;
    }

    if (total_amount <= 0.0) {
        return 0.0;
    }
    return (total_amount - traded_amount) / total_amount;
}

inline double get_factor005(const FactorInput& input) {
    return static_cast<double>(
        factor_detail::time_sub(input.basic_info.predict_t, input.basic_info.lock_time));
}

inline double get_factor006(const Factor01_031State& state) {
    return static_cast<double>(state.window.limit_up_add_count);
}

inline double get_factor007(const Factor01_031State& state) {
    return static_cast<double>(state.window.limit_up_cancel_count);
}

inline double get_factor008(const Factor01_031State& state) {
    return state.window.has_limit_up_cancel_sample ?
        static_cast<double>(state.window.max_limit_up_cancel_volume) :
        factor_detail::nan_value();
}

inline double get_factor009(const Factor01_031State& state) {
    return state.window.has_limit_up_trade_sample ?
        static_cast<double>(state.window.max_limit_up_trade_volume) :
        factor_detail::nan_value();
}

inline double get_factor010(const Factor01_031State& state) {
    if (!state.window.has_first_order_time || state.window.order_count == 0) {
        return factor_detail::nan_value();
    }
    return static_cast<double>(factor_detail::time_sub(
               state.window.last_order_time,
               state.window.first_order_time)) /
           static_cast<double>(state.window.order_count);
}

inline double get_factor011(const Factor01_031State& state) {
    if (!state.window.has_first_trade_time || state.window.trade_count == 0) {
        return factor_detail::nan_value();
    }
    return static_cast<double>(factor_detail::time_sub(
               state.window.last_trade_time,
               state.window.first_trade_time)) /
           static_cast<double>(state.window.trade_count);
}

inline double get_factor012(
    const Factor01_031State& state,
    Factor01_031Workspace& workspace) {
    if (!state.has_self_order_id ||
        !state.has_last_limit_up_sell_bid_order ||
        state.quantiles.limit_up_add_samples.empty()) {
        return factor_detail::nan_value();
    }

    workspace.add_quantile_work.clear();
    workspace.add_quantile_work.reserve(state.quantiles.limit_up_add_samples.size());
    for (std::vector<Factor12AddSample>::const_iterator it =
             state.quantiles.limit_up_add_samples.begin();
         it != state.quantiles.limit_up_add_samples.end(); ++it) {
        workspace.add_quantile_work.push_back(static_cast<double>(it->volume));
    }
    const double threshold = factor_detail::quantile_in_place(workspace.add_quantile_work, 0.80);

    workspace.factor012_order_ids.clear();
    workspace.factor012_order_ids.reserve(state.quantiles.limit_up_add_samples.size() + 1);
    bool self_order_present = false;
    for (std::vector<Factor12AddSample>::const_iterator it =
             state.quantiles.limit_up_add_samples.begin();
         it != state.quantiles.limit_up_add_samples.end(); ++it) {
        if (static_cast<double>(it->volume) > threshold) {
            workspace.factor012_order_ids.push_back(it->order_id);
            if (it->order_id == state.self_order_id) {
                self_order_present = true;
            }
        }
    }
    if (!self_order_present) {
        workspace.factor012_order_ids.push_back(state.self_order_id);
    }

    std::sort(workspace.factor012_order_ids.begin(), workspace.factor012_order_ids.end());
    const std::size_t self_pos = static_cast<std::size_t>(
        std::upper_bound(
            workspace.factor012_order_ids.begin(),
            workspace.factor012_order_ids.end(),
            state.self_order_id) - workspace.factor012_order_ids.begin());
    const std::size_t last_pos = static_cast<std::size_t>(
        std::upper_bound(
            workspace.factor012_order_ids.begin(),
            workspace.factor012_order_ids.end(),
            state.last_limit_up_sell_bid_order) - workspace.factor012_order_ids.begin());
    return static_cast<double>(self_pos) - static_cast<double>(last_pos);
}

inline double get_factor013(const Factor01_031State& state) {
    return scaled_amount(state.window.non_limit_cancel_amount);
}

inline double get_factor014(const Factor01_031State& state) {
    return static_cast<double>(state.window.non_limit_cancel_count);
}

inline double get_factor015(const Factor01_031State& state) {
    return scaled_amount(state.window.non_limit_add_amount);
}

inline double get_factor016(const Factor01_031State& state) {
    return static_cast<double>(state.window.non_limit_add_count);
}

inline double get_factor017(const Factor01_031State& state) {
    if (state.window.limit_up_trade_count == 0) {
        return factor_detail::nan_value();
    }
    return state.window.limit_up_trade_volume_sum /
           static_cast<double>(state.window.limit_up_trade_count);
}

inline double get_factor018(const Factor01_031State& state) {
    if (state.window.limit_up_trade_count == 0) {
        return factor_detail::nan_value();
    }
    return scaled_amount(
        state.window.limit_up_trade_amount_sum /
        static_cast<double>(state.window.limit_up_trade_count));
}

inline double get_factor019(const Factor01_031State& state) {
    return static_cast<double>(state.window.limit_up_trade_count);
}

inline double get_factor020(const Factor01_031State& state) {
    return static_cast<double>(state.window.non_limit_trade_count);
}

inline double get_factor021(const Factor01_031State& state) {
    return state.window.has_lock_order ?
        scaled_amount(state.window.lock_order_amount) :
        factor_detail::nan_value();
}

inline double get_factor022(const Factor01_031State& state) {
    return state.window.has_lock_order ?
        static_cast<double>(state.window.lock_order_volume) :
        factor_detail::nan_value();
}

inline double get_factor023(const Factor01_031State& state) {
    return state.lock_trade.has_trade ?
        static_cast<double>(factor_detail::time_sub(
            state.lock_trade.last_trade_time,
            state.lock_trade.first_trade_time)) :
        factor_detail::nan_value();
}

inline double get_factor024(
    const Factor01_031State& state,
    Factor01_031Workspace& workspace,
    const FactorInput& input) {
    double q10 = 0.0;
    double q50 = 0.0;
    double q90 = 0.0;
    double q95 = 0.0;
    compute_limit_up_trade_quantiles(state, workspace, q10, q50, q90, q95);
    return std::isnan(q10) ? q10 : scaled_amount(q10 * static_cast<double>(input.basic_info.limit_up));
}

inline double get_factor025(
    const Factor01_031State& state,
    Factor01_031Workspace& workspace,
    const FactorInput& input) {
    double q10 = 0.0;
    double q50 = 0.0;
    double q90 = 0.0;
    double q95 = 0.0;
    compute_limit_up_trade_quantiles(state, workspace, q10, q50, q90, q95);
    return std::isnan(q50) ? q50 : scaled_amount(q50 * static_cast<double>(input.basic_info.limit_up));
}

inline double get_factor026(
    const Factor01_031State& state,
    Factor01_031Workspace& workspace,
    const FactorInput& input) {
    double q10 = 0.0;
    double q50 = 0.0;
    double q90 = 0.0;
    double q95 = 0.0;
    compute_limit_up_trade_quantiles(state, workspace, q10, q50, q90, q95);
    return std::isnan(q90) ? q90 : scaled_amount(q90 * static_cast<double>(input.basic_info.limit_up));
}

inline double get_factor027(
    const Factor01_031State& state,
    Factor01_031Workspace& workspace,
    const FactorInput& input) {
    double q10 = 0.0;
    double q50 = 0.0;
    double q90 = 0.0;
    double q95 = 0.0;
    compute_limit_up_trade_quantiles(state, workspace, q10, q50, q90, q95);
    return std::isnan(q95) ? q95 : scaled_amount(q95 * static_cast<double>(input.basic_info.limit_up));
}

inline double get_factor028(const Factor01_031State& state) {
    double volume_sum = 0.0;
    for (std::unordered_map<int, OrderLifecycleState>::const_iterator it =
             state.order_lifecycle.begin();
         it != state.order_lifecycle.end(); ++it) {
        if (!it->second.has_limit_up_cancel_in_window) {
            continue;
        }
        if (factor_detail::time_sub(it->second.last_time, it->second.first_time) == 0) {
            volume_sum += static_cast<double>(it->second.first_volume);
        }
    }
    return volume_sum;
}

inline double get_factor029(const Factor01_031State& state, const FactorInput& input) {
    return scaled_amount(get_factor028(state) * static_cast<double>(input.basic_info.limit_up));
}

inline double get_factor030(const Factor01_031State& state) {
    double count = 0.0;
    for (std::unordered_map<int, OrderLifecycleState>::const_iterator it =
             state.order_lifecycle.begin();
         it != state.order_lifecycle.end(); ++it) {
        if (!it->second.has_limit_up_cancel_in_window) {
            continue;
        }
        if (factor_detail::time_sub(it->second.last_time, it->second.first_time) == 0) {
            count += 1.0;
        }
    }
    return count;
}

inline double get_factor031(const Factor01_031State& state) {
    double total_weighted_time = 0.0;
    double total_volume = 0.0;
    for (std::unordered_map<int, OrderLifecycleState>::const_iterator it =
             state.order_lifecycle.begin();
         it != state.order_lifecycle.end(); ++it) {
        if (!it->second.has_limit_up_cancel_in_window) {
            continue;
        }

        const int duration = factor_detail::time_sub(it->second.last_time, it->second.first_time);
        total_weighted_time +=
            static_cast<double>(duration) * static_cast<double>(it->second.first_volume);
        total_volume += static_cast<double>(it->second.first_volume);
    }
    return total_volume > 0.0 ? total_weighted_time / total_volume : 0.0;
}

inline bool find_first_large_cancel_order_id(
    const Factor01_031State& state,
    Factor01_031Workspace& workspace,
    int& order_id) {
    if (state.quantiles.limit_up_add_samples.empty() ||
        state.limit_up_cancel_order_ids_in_window.empty()) {
        return false;
    }

    workspace.add_quantile_work.clear();
    workspace.add_quantile_work.reserve(state.quantiles.limit_up_add_samples.size());
    for (std::vector<Factor12AddSample>::const_iterator it =
             state.quantiles.limit_up_add_samples.begin();
         it != state.quantiles.limit_up_add_samples.end(); ++it) {
        workspace.add_quantile_work.push_back(static_cast<double>(it->volume));
    }

    const double threshold = factor_detail::quantile_in_place(workspace.add_quantile_work, 0.80);
    if (std::isnan(threshold)) {
        return false;
    }

    for (std::vector<int>::const_iterator it = state.limit_up_cancel_order_ids_in_window.begin();
         it != state.limit_up_cancel_order_ids_in_window.end(); ++it) {
        const std::unordered_map<int, OrderLifecycleState>::const_iterator lifecycle_it =
            state.order_lifecycle.find(*it);
        if (lifecycle_it == state.order_lifecycle.end()) {
            continue;
        }
        if (static_cast<double>(lifecycle_it->second.first_volume) >= threshold) {
            order_id = *it;
            return true;
        }
    }
    return false;
}

inline double get_factor041(const Factor01_031State& state) {
    if (!state.window.has_early70_window || state.window.early70_limit_up_buy_count == 0) {
        return factor_detail::nan_value();
    }
    return static_cast<double>(state.window.early70_limit_up_buy_volume_sum) /
           static_cast<double>(state.window.early70_limit_up_buy_count);
}

inline double get_factor042(const FactorInput& input) {
    if (factor_detail::time_sub(input.basic_info.predict_t, input.basic_info.lock_time) < 100) {
        return factor_detail::nan_value();
    }

    const int end_time = factor_detail::time_add(input.basic_info.lock_time, 100);
    bool has_value = false;
    int first_time = 0;
    int last_time = 0;
    double first_bid_amount = 0.0;
    double peak_bid_amount = 0.0;

    for (std::vector<FactorCrossSectionInfo>::const_iterator it = input.cross_section.begin();
         it != input.cross_section.end(); ++it) {
        if (it->time < input.basic_info.lock_time || it->time > end_time) {
            continue;
        }

        if (!has_value) {
            has_value = true;
            first_time = it->time;
            first_bid_amount = it->bid_amount;
            peak_bid_amount = it->bid_amount;
        } else if (it->bid_amount > peak_bid_amount) {
            peak_bid_amount = it->bid_amount;
        }
        last_time = it->time;
    }

    if (!has_value) {
        return factor_detail::nan_value();
    }

    const double numerator = peak_bid_amount - first_bid_amount;
    const int denominator = factor_detail::time_sub(last_time, first_time);
    return denominator != 0 ? numerator / static_cast<double>(denominator) : numerator;
}

inline double get_factor043(const Factor01_031State& state, const FactorInput& input) {
    return scaled_amount(
        static_cast<double>(state.window.limit_up_sell_volume_sum) *
        static_cast<double>(input.basic_info.limit_up));
}

inline double get_factor044(const Factor01_031State& state) {
    return static_cast<double>(state.window.limit_up_sell_bid_orders.size());
}

inline double get_factor045(const Factor01_031State& state) {
    return static_cast<double>(state.window.limit_up_sell_ask_orders.size());
}

inline double get_factor046(const Factor01_031State& state) {
    const std::size_t ask_count = state.window.limit_up_sell_ask_orders.size();
    if (ask_count == 0) {
        return factor_detail::nan_value();
    }
    return static_cast<double>(state.window.limit_up_sell_bid_orders.size()) /
           static_cast<double>(ask_count);
}

inline double get_factor047(const Factor01_031State& state) {
    if (!state.has_first_limit_cancel_order) {
        return factor_detail::nan_value();
    }

    const std::unordered_map<int, OrderLifecycleState>::const_iterator lifecycle_it =
        state.order_lifecycle.find(state.first_limit_cancel_order_id);
    if (lifecycle_it == state.order_lifecycle.end()) {
        return factor_detail::nan_value();
    }
    return static_cast<double>(factor_detail::time_sub(
        lifecycle_it->second.last_time,
        lifecycle_it->second.first_time));
}

inline double get_factor048(const Factor01_031State& state, const FactorInput& input) {
    if (!state.has_first_limit_cancel_order) {
        return factor_detail::nan_value();
    }

    const std::unordered_map<int, OrderLifecycleState>::const_iterator lifecycle_it =
        state.order_lifecycle.find(state.first_limit_cancel_order_id);
    if (lifecycle_it == state.order_lifecycle.end()) {
        return factor_detail::nan_value();
    }
    return static_cast<double>(factor_detail::time_sub(
        lifecycle_it->second.last_time,
        input.basic_info.lock_time));
}

inline double get_factor049(
    const Factor01_031State& state,
    Factor01_031Workspace& workspace) {
    int order_id = 0;
    if (!find_first_large_cancel_order_id(state, workspace, order_id)) {
        return factor_detail::nan_value();
    }

    const std::unordered_map<int, OrderLifecycleState>::const_iterator lifecycle_it =
        state.order_lifecycle.find(order_id);
    if (lifecycle_it == state.order_lifecycle.end()) {
        return factor_detail::nan_value();
    }
    return static_cast<double>(factor_detail::time_sub(
        lifecycle_it->second.last_time,
        lifecycle_it->second.first_time));
}

inline double get_factor050(
    const Factor01_031State& state,
    Factor01_031Workspace& workspace,
    const FactorInput& input) {
    int order_id = 0;
    if (!find_first_large_cancel_order_id(state, workspace, order_id)) {
        return factor_detail::nan_value();
    }

    const std::unordered_map<int, OrderLifecycleState>::const_iterator lifecycle_it =
        state.order_lifecycle.find(order_id);
    if (lifecycle_it == state.order_lifecycle.end()) {
        return factor_detail::nan_value();
    }
    return static_cast<double>(factor_detail::time_sub(
        lifecycle_it->second.last_time,
        input.basic_info.lock_time));
}

inline Factor01_031Result collect_factor001_031(
    const FactorInput& input,
    const Factor01_031State& state,
    Factor01_031Workspace& workspace) {
    Factor01_031Result result;
    result.factor001 = get_factor001(state);
    result.factor002 = get_factor002(state);
    result.factor003 = get_factor003(state);
    result.factor004 = get_factor004(state);
    result.factor005 = get_factor005(input);
    result.factor006 = get_factor006(state);
    result.factor007 = get_factor007(state);
    result.factor008 = get_factor008(state);
    result.factor009 = get_factor009(state);
    result.factor010 = get_factor010(state);
    result.factor011 = get_factor011(state);
    result.factor012 = get_factor012(state, workspace);
    result.factor013 = get_factor013(state);
    result.factor014 = get_factor014(state);
    result.factor015 = get_factor015(state);
    result.factor016 = get_factor016(state);
    result.factor017 = get_factor017(state);
    result.factor018 = get_factor018(state);
    result.factor019 = get_factor019(state);
    result.factor020 = get_factor020(state);
    result.factor021 = get_factor021(state);
    result.factor022 = get_factor022(state);
    result.factor023 = get_factor023(state);

    double q10 = 0.0;
    double q50 = 0.0;
    double q90 = 0.0;
    double q95 = 0.0;
    compute_limit_up_trade_quantiles(state, workspace, q10, q50, q90, q95);
    result.factor024 = std::isnan(q10) ? q10 : scaled_amount(q10 * static_cast<double>(input.basic_info.limit_up));
    result.factor025 = std::isnan(q50) ? q50 : scaled_amount(q50 * static_cast<double>(input.basic_info.limit_up));
    result.factor026 = std::isnan(q90) ? q90 : scaled_amount(q90 * static_cast<double>(input.basic_info.limit_up));
    result.factor027 = std::isnan(q95) ? q95 : scaled_amount(q95 * static_cast<double>(input.basic_info.limit_up));

    result.factor028 = get_factor028(state);
    result.factor029 = get_factor029(state, input);
    result.factor030 = get_factor030(state);
    result.factor031 = get_factor031(state);
    return result;
}

inline Factor01_031Result collect_factor001_031(const FactorInput& input) {
    Factor01_031State state;
    build_factor01_031_state(input, state);
    Factor01_031Workspace workspace;
    return collect_factor001_031(input, state, workspace);
}

inline Factor041_050Result collect_factor041_050(
    const FactorInput& input,
    const Factor01_031State& state,
    Factor01_031Workspace& workspace) {
    Factor041_050Result result;
    result.factor041 = get_factor041(state);
    result.factor042 = get_factor042(input);
    result.factor043 = get_factor043(state, input);
    result.factor044 = get_factor044(state);
    result.factor045 = get_factor045(state);
    result.factor046 = get_factor046(state);
    result.factor047 = get_factor047(state);
    result.factor048 = get_factor048(state, input);
    result.factor049 = get_factor049(state, workspace);
    result.factor050 = get_factor050(state, workspace, input);
    return result;
}

inline Factor041_050Result collect_factor041_050(const FactorInput& input) {
    Factor01_031State state;
    build_factor01_031_state(input, state);
    Factor01_031Workspace workspace;
    return collect_factor041_050(input, state, workspace);
}

} // namespace factor01_031_detail

#endif //LIMIT_UP_FACTOR_DEMO_FACTOR01_031_EXTRACT_H
