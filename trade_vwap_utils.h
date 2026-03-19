//
// Created by even on 2026/3/18.
//

#ifndef LIMIT_UP_FACTOR_DEMO_TRADE_VWAP_UTILS_H
#define LIMIT_UP_FACTOR_DEMO_TRADE_VWAP_UTILS_H

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "cross_section_utils.h"

namespace factor_vwap_detail {


struct VwapRatioStatsState {
    double              cum_amount;   // 累计成交金额(单位：元)
    double              cum_volume;   // 累计成交量(单位：股)

    std::size_t         count;        // 样本数量

    double              mean;         // 样本的均值
    double              m2;           // 样本的二阶中心矩
    double              m3;           // 样本的三阶中心矩
    double              m4;           // 样本的四阶中心矩

    double              min_value;    // 样本中的最小值
    double              max_value;    // 样本中的最大值

    std::vector<double> samples;      // 样本数据的集合（存储所有计算出的样本值）
};

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


//初始化
inline void reset_vwap_ratio_stats(VwapRatioStatsState& state) {
    state.cum_amount = 0.0;
    state.cum_volume = 0.0;
    state.count = 0;
    state.mean = 0.0;
    state.m2 = 0.0;
    state.m3 = 0.0;
    state.m4 = 0.0;
    state.min_value = factor_detail::nan_value();
    state.max_value = factor_detail::nan_value();
    state.samples.clear();
}


// 流式更新逻辑
inline void update_moments(VwapRatioStatsState& state, const double sample) {
    const std::size_t previous_count = state.count;
    state.count += 1;

    const double n = static_cast<double>(state.count);
    const double n1 = static_cast<double>(previous_count);
    const double delta = sample - state.mean;
    const double delta_n = delta / n;
    const double delta_n2 = delta_n * delta_n;
    const double term1 = delta * delta_n * n1;

    state.mean += delta_n;
    state.m4 += term1 * delta_n2 * (n * n - 3.0 * n + 3.0) +
                6.0 * delta_n2 * state.m2 -
                4.0 * delta_n * state.m3;
    state.m3 += term1 * delta_n * (n - 2.0) - 3.0 * delta_n * state.m2;
    state.m2 += term1;

    if (previous_count == 0) {
        state.min_value = sample;
        state.max_value = sample;
        return;
    }

    if (sample < state.min_value) {
        state.min_value = sample;
    }
    if (sample > state.max_value) {
        state.max_value = sample;
    }
}


//更新均值
inline void update_vwap_ratio_stats(
    VwapRatioStatsState& state,
    const int time,
    const int lock_time, //涨停时间
    const int64_t trade_price,
    const int trade_volume,
    const int64_t limit_up) {
    if (time < 93000000 || time > lock_time) {
        return;
    }
    if (trade_volume <= 0 || limit_up <= 0) {
        return;
    }

    state.cum_amount += static_cast<double>(trade_price) * static_cast<double>(trade_volume);
    state.cum_volume += static_cast<double>(trade_volume);
    if (state.cum_volume <= 0.0) {
        return;
    }

    const double sample = state.cum_amount / state.cum_volume / static_cast<double>(limit_up);
    state.samples.push_back(sample);
    update_moments(state, sample);
}


//
inline void build_vwap_ratio_stats(
    const FactorInput& input,
    VwapRatioStatsState& state) {
    reset_vwap_ratio_stats(state);
    state.samples.reserve(input.trade_events.size());

    for (std::vector<FactorTradeEvent>::const_iterator it = input.trade_events.begin();
         it != input.trade_events.end(); ++it) {
        update_vwap_ratio_stats(
            state,
            it->time,
            input.basic_info.lock_time,
            it->price,
            it->volume,
            input.basic_info.limit_up);
    }
}

inline VwapRatioStatsState build_vwap_ratio_stats(const FactorInput& input) {
    VwapRatioStatsState state;
    build_vwap_ratio_stats(input, state);
    return state;
}

inline double get_factor053(const VwapRatioStatsState& state) {
    return state.count == 0 ? factor_detail::nan_value() : state.mean;
}

inline double get_factor054(const VwapRatioStatsState& state) {
    if (state.count < 2) {
        return factor_detail::nan_value();
    }
    return std::sqrt(state.m2 / static_cast<double>(state.count - 1));
}

inline double get_factor055(const VwapRatioStatsState& state) {
    if (state.count < 3 || state.m2 == 0.0) {
        return factor_detail::nan_value();
    }

    const double sample_std = std::sqrt(state.m2 / static_cast<double>(state.count - 1));
    if (sample_std == 0.0) {
        return 0.0;
    }

    const double n = static_cast<double>(state.count);
    return (n * state.m3) /
           ((n - 1.0) * (n - 2.0) * sample_std * sample_std * sample_std);
}

inline double get_factor056(const VwapRatioStatsState& state) {
    if (state.count < 4 || state.m2 == 0.0) {
        return factor_detail::nan_value();
    }

    const double n = static_cast<double>(state.count);
    const double variance = state.m2 / static_cast<double>(state.count - 1);
    if (variance == 0.0) {
        return 0.0;
    }

    const double numerator = n * (n + 1.0) * state.m4;
    const double denominator = (n - 1.0) * (n - 2.0) * (n - 3.0) * variance * variance;
    const double adjustment = 3.0 * (n - 1.0) * (n - 1.0) / ((n - 2.0) * (n - 3.0));
    return numerator / denominator - adjustment;
}

inline double get_quantile_factor(const VwapRatioStatsState& state, const double p) {
    if (state.samples.empty()) {
        return factor_detail::nan_value();
    }

    std::vector<double> work(state.samples);
    return factor_detail::quantile_in_place(work, p);
}

inline double get_factor057(const VwapRatioStatsState& state) {
    return get_quantile_factor(state, 0.25);
}

inline double get_factor058(const VwapRatioStatsState& state) {
    return get_quantile_factor(state, 0.50);
}

inline double get_factor059(const VwapRatioStatsState& state) {
    return get_quantile_factor(state, 0.75);
}

inline double get_factor060(const VwapRatioStatsState& state) {
    return state.count == 0 ? factor_detail::nan_value() : state.min_value;
}

inline double get_factor061(const VwapRatioStatsState& state) {
    return state.count == 0 ? factor_detail::nan_value() : state.max_value;
}

inline VwapRatioFactorResult collect_factor053_061(const VwapRatioStatsState& state) {
    VwapRatioFactorResult result;
    result.factor053 = get_factor053(state);
    result.factor054 = get_factor054(state);
    result.factor055 = get_factor055(state);
    result.factor056 = get_factor056(state);
    result.factor057 = get_factor057(state);
    result.factor058 = get_factor058(state);
    result.factor059 = get_factor059(state);
    result.factor060 = get_factor060(state);
    result.factor061 = get_factor061(state);
    return result;
}

inline VwapRatioFactorResult collect_factor053_061(const FactorInput& input) {
    const VwapRatioStatsState state = build_vwap_ratio_stats(input);
    return collect_factor053_061(state);
}

} // namespace factor_vwap_detail

#endif //LIMIT_UP_FACTOR_DEMO_TRADE_VWAP_UTILS_H
