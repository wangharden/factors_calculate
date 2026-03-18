//
// Created by even on 2026/3/18.
//

#ifndef LIMIT_UP_FACTOR_DEMO_CROSS_SECTION_UTILS_H
#define LIMIT_UP_FACTOR_DEMO_CROSS_SECTION_UTILS_H

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


//平均数
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


//标准差
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


//分位数
inline double quantile_position(const std::size_t count, const double p) {
    if (count == 0) {
        return 0.0;
    }

    const double clamped_p = p < 0.0 ? 0.0 : (p > 1.0 ? 1.0 : p);
    return (static_cast<double>(count) - 1.0) * clamped_p;
}

inline double quantile_in_place(std::vector<double>& values, const double p) {
    if (values.empty()) {
        return nan_value();
    }

    const double position = quantile_position(values.size(), p);
    const std::size_t lower_index = static_cast<std::size_t>(std::floor(position));
    const std::size_t upper_index = static_cast<std::size_t>(std::ceil(position));

    std::nth_element(values.begin(), values.begin() + lower_index, values.end());
    const double lower_value = values[lower_index];
    if (lower_index == upper_index) {
        return lower_value;
    }

    std::nth_element(
        values.begin() + lower_index + 1,
        values.begin() + upper_index,
        values.end());
    const double upper_value = values[upper_index];
    const double weight = position - static_cast<double>(lower_index);
    return lower_value + (upper_value - lower_value) * weight;
}

inline double quantile(const std::vector<double>& values, const double p) {
    std::vector<double> copy(values);
    return quantile_in_place(copy, p);
}


//中位数
inline double median(const std::vector<double>& values) {
    return quantile(values, 0.5);
}


//偏度
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


//峰度
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


//汇总指定时间点的买单金额的汇总信息（是否有买单金额，第一个和最后一个的金额，金额总数）
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


//统计指定时间窗口内的截面数据数量
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


//
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

#endif //LIMIT_UP_FACTOR_DEMO_CROSS_SECTION_UTILS_H
