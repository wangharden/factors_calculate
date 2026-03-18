//
// Created by Codex on 2026/3/18.
//

#ifndef LIMIT_UP_FACTOR_DEMO_FACTOR032_040_EXTRACT_H
#define LIMIT_UP_FACTOR_DEMO_FACTOR032_040_EXTRACT_H

#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

#include "cross_section_utils.h"

namespace factor032_040_detail {

struct RunningMomentState {
    std::size_t count;
    double mean;
    double m2; //二阶中心矩累积
    double m3; //三阶中心矩累积
};

struct Factor032_040State {
    factor_detail::ExactTimePositiveSummary lock_summary;  // 封板时刻的统计信息
    factor_detail::ExactTimePositiveSummary predict_summary;  // 预测时刻的统计信息
    factor_detail::WindowPeakInfo peak;  // 时间窗口内的最大值信息

    bool has_window_last;  // 是否存在最后一条窗口数据
    bool has_window_prev;  // 是否存在上一条窗口数据
    bool has_bid_amount_moments;  // 是否计算了委托金额的统计量

    std::size_t window_count;  // 时间窗口内的截面数量
    double last_bid_amount;  // 最后一条截面的委托金额
    double prev_bid_amount;  // 上一条截面的委托金额

    RunningMomentState bid_amount_moments;  // 委托金额的统计量（均值、方差、偏度）
    std::vector<double> diff_returns;  // 差分收益率序列
};

struct Factor032_040Workspace {
    std::vector<double> diff_returns_work;
};

struct Factor032_040Result {
    double factor032;
    double factor033;
    double factor034;
    double factor035;
    double factor036;
    double factor037;
    double factor038;
    double factor039;
    double factor040;
};

inline void reset_running_moments(RunningMomentState& state) {
    state.count = 0;
    state.mean = 0.0;
    state.m2 = 0.0;
    state.m3 = 0.0;
}

inline void update_running_moments(RunningMomentState& state, const double sample) {
    const std::size_t previous_count = state.count;
    state.count += 1;

    const double n = static_cast<double>(state.count);
    const double n1 = static_cast<double>(previous_count);
    const double delta = sample - state.mean;
    const double delta_n = delta / n;
    const double term1 = delta * delta_n * n1;

    state.mean += delta_n;
    state.m3 += term1 * delta_n * (n - 2.0) - 3.0 * delta_n * state.m2;
    state.m2 += term1;
}

inline double finalize_sample_skew(const RunningMomentState& state) {
    if (state.count < 3) {
        return factor_detail::nan_value();
    }
    if (state.m2 == 0.0) {
        return 0.0;
    }

    const double sample_variance = state.m2 / static_cast<double>(state.count - 1);
    const double sample_std = std::sqrt(sample_variance);
    if (sample_std == 0.0) {
        return 0.0;
    }

    const double n = static_cast<double>(state.count);
    return (n * state.m3) /
           ((n - 1.0) * (n - 2.0) * sample_std * sample_std * sample_std);
}

inline void reset_factor032_040_state(Factor032_040State& state) {
    state.lock_summary.has_value = false;
    state.lock_summary.first_value = 0.0;
    state.lock_summary.last_value = 0.0;
    state.lock_summary.count = 0;

    state.predict_summary.has_value = false;
    state.predict_summary.first_value = 0.0;
    state.predict_summary.last_value = 0.0;
    state.predict_summary.count = 0;

    state.peak.has_value = false;
    state.peak.time = 0;
    state.peak.bid_amount = 0.0;

    state.has_window_last = false;
    state.has_window_prev = false;
    state.has_bid_amount_moments = false;
    state.window_count = 0;
    state.last_bid_amount = 0.0;
    state.prev_bid_amount = 0.0;
    reset_running_moments(state.bid_amount_moments);
    state.diff_returns.clear();
}

inline void update_exact_time_summary(
    factor_detail::ExactTimePositiveSummary& summary,
    const double bid_amount) {
    if (bid_amount <= 0.0) {
        return;
    }
    if (!summary.has_value) {
        summary.has_value = true;
        summary.first_value = bid_amount;
    }
    summary.last_value = bid_amount;
    ++summary.count;
}

// 逐条截面推送接口：每收到一条截面调用一次
inline void on_cross_section_update(
    Factor032_040State& state,
    const FactorBasicInfo& basic_info,
    const FactorCrossSectionInfo& cs) {
    if (cs.time == basic_info.lock_time) {
        update_exact_time_summary(state.lock_summary, cs.bid_amount);
    }
    if (cs.time == basic_info.predict_t) {
        update_exact_time_summary(state.predict_summary, cs.bid_amount);
    }

    if (cs.time < basic_info.lock_time || cs.time > basic_info.predict_t) {
        return;
    }

    ++state.window_count;
    state.last_bid_amount = cs.bid_amount;
    state.has_window_last = true;

    //更新峰值数据
    if (!state.peak.has_value || cs.bid_amount > state.peak.bid_amount) {
        state.peak.has_value = true;
        state.peak.time = cs.time;
        state.peak.bid_amount = cs.bid_amount;
    }

    //更新累计委托金额
    update_running_moments(state.bid_amount_moments, cs.bid_amount);
    state.has_bid_amount_moments = true;

    //初始化差分收益率
    if (!state.has_window_prev) {
        state.diff_returns.push_back(0.0);
        state.prev_bid_amount = cs.bid_amount;
        state.has_window_prev = true;
        return;
    }

    //计算查分收益率
    if (state.prev_bid_amount == 0.0) {
        state.diff_returns.push_back(
            cs.bid_amount == 0.0 ? 0.0 : std::numeric_limits<double>::infinity());
    } else {
        state.diff_returns.push_back(cs.bid_amount / state.prev_bid_amount - 1.0);
    }
    state.prev_bid_amount = cs.bid_amount;
}

// 批量构建：遍历全量 cross_section，逐条调用 on_cross_section_update
inline void build_factor032_040_state(
    const FactorInput& input,
    Factor032_040State& state) {
    reset_factor032_040_state(state);
    state.diff_returns.reserve(input.cross_section.size());

    for (std::vector<FactorCrossSectionInfo>::const_iterator it = input.cross_section.begin();
         it != input.cross_section.end(); ++it) {
        on_cross_section_update(state, input.basic_info, *it);
    }
}


//初始化中间参数计算对象
inline Factor032_040State build_factor032_040_state(const FactorInput& input) {
    Factor032_040State state;
    build_factor032_040_state(input, state);
    return state;
}


//计算对数增长率
inline double compute_alpha(const factor_detail::ExactTimePositiveSummary& summary) {
    if (!summary.has_value || summary.first_value <= 0.0 || summary.last_value <= 0.0) {
        return factor_detail::nan_value();
    }
    return std::log(summary.last_value / summary.first_value) /
           static_cast<double>(summary.count);
}

inline double get_factor032(const Factor032_040State& state) {
    return compute_alpha(state.lock_summary);
}

inline double get_factor033(const Factor032_040State& state) {
    return compute_alpha(state.predict_summary);
}

inline double get_factor034(const Factor032_040State& state) {
    return static_cast<double>(state.window_count);
}

inline double get_factor035(const FactorInput& input, const Factor032_040State& state) {
    const int duration = factor_detail::time_sub(
        input.basic_info.predict_t, input.basic_info.lock_time);
    if (duration != 0) {
        return static_cast<double>(state.window_count) / static_cast<double>(duration);
    }
    return static_cast<double>(input.cross_section.size());
}

inline double compute_var95(
    const Factor032_040State& state,
    Factor032_040Workspace& workspace) {
    if (state.diff_returns.empty()) {
        return factor_detail::nan_value();
    }

    workspace.diff_returns_work.assign(
        state.diff_returns.begin(),
        state.diff_returns.end());
    return factor_detail::quantile_in_place(workspace.diff_returns_work, 0.05);
}

inline double get_factor036(
    const Factor032_040State& state,
    Factor032_040Workspace& workspace) {
    return compute_var95(state, workspace);
}

inline double get_factor037(
    const Factor032_040State& state,
    Factor032_040Workspace& workspace) {
    const double var_95 = compute_var95(state, workspace);
    if (state.diff_returns.empty() || std::isnan(var_95)) {
        return factor_detail::nan_value();
    }

    double sum = 0.0;
    std::size_t count = 0;
    for (std::vector<double>::const_iterator it = state.diff_returns.begin();
         it != state.diff_returns.end(); ++it) {
        if (*it < var_95) {
            sum += *it;
            ++count;
        }
    }
    return count == 0 ? factor_detail::nan_value() : sum / static_cast<double>(count);
}

inline double get_factor038(const FactorInput& input, const Factor032_040State& state) {
    if (!state.peak.has_value) {
        return factor_detail::nan_value();
    }
    const int duration = factor_detail::time_sub(state.peak.time, input.basic_info.lock_time);
    return duration != 0 ? state.peak.bid_amount / static_cast<double>(duration) : state.peak.bid_amount;
}

inline double get_factor039(const FactorInput& input, const Factor032_040State& state) {
    if (!state.peak.has_value || !state.has_window_last) {
        return factor_detail::nan_value();
    }
    const double delta = state.last_bid_amount - state.peak.bid_amount;
    const int duration = factor_detail::time_sub(input.basic_info.predict_t, state.peak.time);
    return duration != 0 ? delta / static_cast<double>(duration) : delta;
}

inline double get_factor040(const Factor032_040State& state) {
    return state.has_bid_amount_moments ?
        finalize_sample_skew(state.bid_amount_moments) :
        factor_detail::nan_value();
}


//输出因子
inline Factor032_040Result collect_factor032_040(
    const FactorInput& input,
    const Factor032_040State& state,
    Factor032_040Workspace& workspace) {
    Factor032_040Result result;
    result.factor032 = get_factor032(state);
    result.factor033 = get_factor033(state);
    result.factor034 = get_factor034(state);
    result.factor035 = get_factor035(input, state);
    result.factor036 = get_factor036(state, workspace);
    result.factor037 = get_factor037(state, workspace);
    result.factor038 = get_factor038(input, state);
    result.factor039 = get_factor039(input, state);
    result.factor040 = get_factor040(state);
    return result;
}

//用于批量数据一次性构建并输出因子
inline Factor032_040Result collect_factor032_040(const FactorInput& input) {
    Factor032_040State state;
    build_factor032_040_state(input, state);
    Factor032_040Workspace workspace;
    return collect_factor032_040(input, state, workspace);
}

} // namespace factor032_040_detail

#endif //LIMIT_UP_FACTOR_DEMO_FACTOR032_040_EXTRACT_H
