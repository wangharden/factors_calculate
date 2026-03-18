//
// Created by even on 2026/3/17.
//

#ifndef LIMIT_UP_FACTOR_DEMO_FACTOR_COLLECTION_H
#define LIMIT_UP_FACTOR_DEMO_FACTOR_COLLECTION_H

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "factor.h"

namespace factor_detail {

struct ExactTimePositiveSummary {
    bool        has_value;
    double      first_value;
    double      last_value;
    std::size_t count;
};

struct WindowPeakInfo {
    bool    has_value;
    int     time;
    double  bid_amount;
};

inline double nan_value() {
    return std::numeric_limits<double>::quiet_NaN();
}

inline int convert_to_ms(const int time_value) {
    const int hours = time_value / 10000000;
    const int minutes = (time_value / 100000) % 100;
    const int seconds = (time_value / 1000) % 100;
    const int milliseconds = time_value % 1000;
    return (hours * 3600 + minutes * 60 + seconds) * 1000 + milliseconds;
}

inline int convert_to_time(const int milliseconds) {
    const int total_seconds = milliseconds / 1000;
    const int hours = total_seconds / 3600;
    const int minutes = (total_seconds % 3600) / 60;
    const int seconds = total_seconds % 60;
    const int millis = milliseconds % 1000;
    return hours * 10000000 + minutes * 100000 + seconds * 1000 + millis;
}

inline int time_sub(const int a, const int b) {
    if (a >= 130000000 && b <= 113000000) {
        return convert_to_ms(113000000) - convert_to_ms(b) +
               convert_to_ms(a) - convert_to_ms(130000000);
    }
    return convert_to_ms(a) - convert_to_ms(b);
}

inline int time_add(const int a, const int b) {
    int result = convert_to_time(convert_to_ms(a) + convert_to_ms(b));
    if (a < 113000000 && result > 113000000 && result < 130000000) {
        result = time_add(130000000, convert_to_time(time_sub(result, 113000000)));
    } else if (a < 130000000 && result > 130000000) {
        result = time_add(130000000, result);
    }

    if (result > 150000000) {
        return 150500000;
    }
    return result;
}

inline double mean(const std::vector<double>& values) {
    if (values.empty()) {
        return nan_value();
    }

    double sum = 0.0;
    for (std::vector<double>::const_iterator it = values.begin(); it != values.end(); ++it) {
        sum += *it;
    }
    return sum / static_cast<double>(values.size());
}

inline double stddev(const std::vector<double>& values) {
    if (values.size() < 2) {
        return nan_value();
    }

    const double avg = mean(values);
    double sum_sq = 0.0;
    for (std::vector<double>::const_iterator it = values.begin(); it != values.end(); ++it) {
        const double centered = *it - avg;
        sum_sq += centered * centered;
    }

    return std::sqrt(sum_sq / static_cast<double>(values.size() - 1));
}

inline double quantile_in_place(std::vector<double>& values, const double p) {
    if (values.empty()) {
        return nan_value();
    }

    std::sort(values.begin(), values.end());
    const double pos = (static_cast<double>(values.size()) - 1.0) * p;
    const std::size_t lower = static_cast<std::size_t>(std::floor(pos));
    const std::size_t upper = static_cast<std::size_t>(std::ceil(pos));
    if (lower == upper) {
        return values[lower];
    }

    const double weight = pos - static_cast<double>(lower);
    return values[lower] * (1.0 - weight) + values[upper] * weight;
}

inline double quantile(const std::vector<double>& values, const double p) {
    std::vector<double> copy(values);
    return quantile_in_place(copy, p);
}

inline double median(const std::vector<double>& values) {
    return quantile(values, 0.5);
}

inline double skew(const std::vector<double>& values) {
    const std::size_t n = values.size();
    if (n < 3) {
        return nan_value();
    }

    const double avg = mean(values);
    double sum_sq = 0.0;
    double sum_cube = 0.0;
    for (std::vector<double>::const_iterator it = values.begin(); it != values.end(); ++it) {
        const double centered = *it - avg;
        sum_sq += centered * centered;
        sum_cube += centered * centered * centered;
    }

    if (sum_sq == 0.0) {
        return 0.0;
    }

    const double sample_variance = sum_sq / static_cast<double>(n - 1);
    const double sample_std = std::sqrt(sample_variance);
    if (sample_std == 0.0) {
        return 0.0;
    }

    return (static_cast<double>(n) * sum_cube) /
           (static_cast<double>((n - 1) * (n - 2)) * sample_std * sample_std * sample_std);
}

inline double kurt(const std::vector<double>& values) {
    const std::size_t n = values.size();
    if (n < 4) {
        return nan_value();
    }

    const double avg = mean(values);
    double sum_sq = 0.0;
    double sum_quad = 0.0;
    for (std::vector<double>::const_iterator it = values.begin(); it != values.end(); ++it) {
        const double centered = *it - avg;
        const double square = centered * centered;
        sum_sq += square;
        sum_quad += square * square;
    }

    if (sum_sq == 0.0) {
        return 0.0;
    }

    const double variance = sum_sq / static_cast<double>(n - 1);
    const double numerator = static_cast<double>(n) * (n + 1) * sum_quad;
    const double denominator = static_cast<double>((n - 1) * (n - 2) * (n - 3)) *
                               variance * variance;
    const double adjustment = (3.0 * static_cast<double>(n - 1) * static_cast<double>(n - 1)) /
                              static_cast<double>((n - 2) * (n - 3));
    return numerator / denominator - adjustment;
}

inline ExactTimePositiveSummary summarize_positive_bid_amounts_at_time(
    const std::vector<FactorCrossSectionInfo>& cross_section,
    const int target_time) {
    ExactTimePositiveSummary summary = {false, 0.0, 0.0, 0};
    for (std::vector<FactorCrossSectionInfo>::const_iterator it = cross_section.begin();
         it != cross_section.end(); ++it) {
        if (it->time != target_time || it->bid_amount <= 0.0) {
            continue;
        }

        if (!summary.has_value) {
            summary.has_value = true;
            summary.first_value = it->bid_amount;
        }
        summary.last_value = it->bid_amount;
        ++summary.count;
    }
    return summary;
}

inline std::size_t count_cross_section_in_window(
    const std::vector<FactorCrossSectionInfo>& cross_section,
    const int begin_time,
    const int end_time) {
    std::size_t count = 0;
    for (std::vector<FactorCrossSectionInfo>::const_iterator it = cross_section.begin();
         it != cross_section.end(); ++it) {
        if (it->time >= begin_time && it->time <= end_time) {
            ++count;
        }
    }
    return count;
}

inline WindowPeakInfo max_bid_amount_with_time_in_window(
    const std::vector<FactorCrossSectionInfo>& cross_section,
    const int begin_time,
    const int end_time) {
    WindowPeakInfo peak = {false, 0, 0.0};
    for (std::vector<FactorCrossSectionInfo>::const_iterator it = cross_section.begin();
         it != cross_section.end(); ++it) {
        if (it->time < begin_time || it->time > end_time) {
            continue;
        }

        if (!peak.has_value || it->bid_amount > peak.bid_amount) {
            peak.has_value = true;
            peak.time = it->time;
            peak.bid_amount = it->bid_amount;
        }
    }
    return peak;
}

inline bool last_bid_amount_in_window(
    const std::vector<FactorCrossSectionInfo>& cross_section,
    const int begin_time,
    const int end_time,
    double& last_bid_amount) {
    bool found = false;
    for (std::vector<FactorCrossSectionInfo>::const_iterator it = cross_section.begin();
         it != cross_section.end(); ++it) {
        if (it->time < begin_time || it->time > end_time) {
            continue;
        }
        last_bid_amount = it->bid_amount;
        found = true;
    }
    return found;
}

inline std::vector<double> collect_bid_amounts_in_window(
    const std::vector<FactorCrossSectionInfo>& cross_section,
    const int begin_time,
    const int end_time) {
    std::vector<double> values;
    for (std::vector<FactorCrossSectionInfo>::const_iterator it = cross_section.begin();
         it != cross_section.end(); ++it) {
        if (it->time >= begin_time && it->time <= end_time) {
            values.push_back(it->bid_amount);
        }
    }
    return values;
}

inline std::vector<double> calc_diff_returns_in_window(
    const std::vector<FactorCrossSectionInfo>& cross_section,
    const int begin_time,
    const int end_time) {
    std::vector<double> returns;
    bool has_prev = false;
    double prev = 0.0;

    for (std::vector<FactorCrossSectionInfo>::const_iterator it = cross_section.begin();
         it != cross_section.end(); ++it) {
        if (it->time < begin_time || it->time > end_time) {
            continue;
        }

        if (!has_prev) {
            returns.push_back(0.0);
            prev = it->bid_amount;
            has_prev = true;
            continue;
        }

        const double curr = it->bid_amount;
        if (prev == 0.0) {
            returns.push_back(curr == 0.0 ? 0.0 : std::numeric_limits<double>::infinity());
        } else {
            returns.push_back(curr / prev - 1.0);
        }
        prev = curr;
    }

    return returns;
}

} // namespace factor_detail

void call_factor001(const FactorInput& input, double& result);

/// factor031: 封板后到 predict_t 时刻之间涨停价订单加权平均撤单时间。
/// 该因子仍依赖封板后的原始逐笔委托生命周期数据，当前保持占位。
inline void call_factor031(const FactorInput& input, double& result) {
    (void)input;
    result = 0.0;
}

inline void call_factor032(const FactorInput& input, double& result) {
    const factor_detail::ExactTimePositiveSummary summary =
        factor_detail::summarize_positive_bid_amounts_at_time(
            input.cross_section, input.basic_info.lock_time);
    if (!summary.has_value || summary.first_value <= 0.0 || summary.last_value <= 0.0) {
        result = factor_detail::nan_value();
        return;
    }
    result = std::log(summary.last_value / summary.first_value) /
             static_cast<double>(summary.count);
}

inline void call_factor033(const FactorInput& input, double& result) {
    const factor_detail::ExactTimePositiveSummary summary =
        factor_detail::summarize_positive_bid_amounts_at_time(
            input.cross_section, input.basic_info.predict_t);
    if (!summary.has_value || summary.first_value <= 0.0 || summary.last_value <= 0.0) {
        result = factor_detail::nan_value();
        return;
    }
    result = std::log(summary.last_value / summary.first_value) /
             static_cast<double>(summary.count);
}

inline void call_factor034(const FactorInput& input, double& result) {
    result = static_cast<double>(factor_detail::count_cross_section_in_window(
        input.cross_section, input.basic_info.lock_time, input.basic_info.predict_t));
}

inline void call_factor035(const FactorInput& input, double& result) {
    const std::size_t count = factor_detail::count_cross_section_in_window(
        input.cross_section, input.basic_info.lock_time, input.basic_info.predict_t);
    const int duration = factor_detail::time_sub(
        input.basic_info.predict_t, input.basic_info.lock_time);
    if (duration != 0) {
        result = static_cast<double>(count) / static_cast<double>(duration);
        return;
    }
    result = static_cast<double>(input.cross_section.size());
}

inline void call_factor036(const FactorInput& input, double& result) {
    std::vector<double> returns = factor_detail::calc_diff_returns_in_window(
        input.cross_section, input.basic_info.lock_time, input.basic_info.predict_t);
    result = factor_detail::quantile_in_place(returns, 0.05);
}

inline void call_factor037(const FactorInput& input, double& result) {
    const std::vector<double> returns = factor_detail::calc_diff_returns_in_window(
        input.cross_section, input.basic_info.lock_time, input.basic_info.predict_t);
    const double var_95 = factor_detail::quantile(returns, 0.05);
    if (returns.empty() || std::isnan(var_95)) {
        result = factor_detail::nan_value();
        return;
    }

    std::vector<double> tail_values;
    for (std::vector<double>::const_iterator it = returns.begin(); it != returns.end(); ++it) {
        if (*it < var_95) {
            tail_values.push_back(*it);
        }
    }
    result = factor_detail::mean(tail_values);
}

inline void call_factor038(const FactorInput& input, double& result) {
    const factor_detail::WindowPeakInfo peak = factor_detail::max_bid_amount_with_time_in_window(
        input.cross_section, input.basic_info.lock_time, input.basic_info.predict_t);
    if (!peak.has_value) {
        result = factor_detail::nan_value();
        return;
    }

    const int duration = factor_detail::time_sub(peak.time, input.basic_info.lock_time);
    result = duration != 0 ? peak.bid_amount / static_cast<double>(duration) : peak.bid_amount;
}

inline void call_factor039(const FactorInput& input, double& result) {
    const factor_detail::WindowPeakInfo peak = factor_detail::max_bid_amount_with_time_in_window(
        input.cross_section, input.basic_info.lock_time, input.basic_info.predict_t);
    if (!peak.has_value) {
        result = factor_detail::nan_value();
        return;
    }

    double last_bid_amount = 0.0;
    if (!factor_detail::last_bid_amount_in_window(
            input.cross_section,
            input.basic_info.lock_time,
            input.basic_info.predict_t,
            last_bid_amount)) {
        result = factor_detail::nan_value();
        return;
    }

    const double delta = last_bid_amount - peak.bid_amount;
    const int duration = factor_detail::time_sub(input.basic_info.predict_t, peak.time);
    result = duration != 0 ? delta / static_cast<double>(duration) : delta;
}

inline void call_factor040(const FactorInput& input, double& result) {
    result = factor_detail::skew(factor_detail::collect_bid_amounts_in_window(
        input.cross_section, input.basic_info.lock_time, input.basic_info.predict_t));
}

#endif //LIMIT_UP_FACTOR_DEMO_FACTOR_COLLECTION_H
