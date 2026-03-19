//
// Created by even on 2026/3/17.
//

#include "factor.h"
#include "factor_collection.h"

static int convert_to_ms(int time_value) {
    int hours = time_value / 10000000;
    int minutes = (time_value / 100000) % 100;
    int seconds = (time_value / 1000) % 100;
    int milliseconds = time_value % 1000;
    return (hours * 3600 + minutes * 60 + seconds) * 1000 + milliseconds;
}

static int time_sub(int a, int b) {
    if (a >= 130000000 && b <= 113000000) {
        return convert_to_ms(113000000) - convert_to_ms(b) +
               convert_to_ms(a) - convert_to_ms(130000000);
    }
    return convert_to_ms(a) - convert_to_ms(b);
}

/// 封板时刻的封单额
void call_factor001(const FactorInput& input, double& result) {
    result = input.fbfd;
    return;
}

/// predict_t时刻的封单额
void call_factor002(const FactorInput& input, double& result) {
    result = input.ptfd;
    return;
}

/// predict_t时刻之前的最大封单额
void call_factor003(const FactorInput& input, double& result) {
    result = input.maxfd;
    return;
}

/// predict_t时刻订单簿的成交情况 (total_amount - traded_amount) / total_amount
void call_factor004(const FactorInput& input, double& result) {
    double total_amount = input.zt_total_amount;
    double traded_amount = input.zt_traded_amount;
    if (total_amount > 0.0) {
        result = (total_amount - traded_amount) / total_amount;
    } else {
        result = 0.0;
    }
    return;
}

/// predict_t距离封板时刻的时间(毫秒)
void call_factor005(const FactorInput& input, int& result) {
    result = time_sub(input.basic_info.predict_t, input.basic_info.lock_time);
    return;
}

/// 封板后到predict_t时刻之间的涨停价的挂单数
void call_factor006(const FactorInput& input, int& result) {
    result = input.zt_add_cnt;
    return;
}

/// 封板后到predict_t时刻之间的涨停价的撤单数
void call_factor007(const FactorInput& input, int& result) {
    result = input.zt_cancel_cnt;
    return;
}

/// 封板后到predict_t时刻之间的涨停价单笔最大撤单量
void call_factor008(const FactorInput& input, int& result) {
    result = input.zt_max_cancel_vol;
    return;
}

/// 封板后到predict_t时刻之间的涨停价单笔最大成交量
void call_factor009(const FactorInput& input, int& result) {
    result = input.zt_max_trade_vol;
    return;
}

/// 封板后到predict_t时刻之间逐笔委托的更新频率（时间间隔/委托数量)
void call_factor010(const FactorInput& input, double& result) {
    if (input.order_cnt > 0) {
        result = static_cast<double>(
            time_sub(input.last_order_time, input.first_order_time)) /
            static_cast<double>(input.order_cnt);
    } else {
        result = 0.0;
    }
    return;
}

/// 封板后到predict_t时刻之间逐笔成交的更新频率(时间间隔/委托数量)
void call_factor011(const FactorInput& input, double& result) {
    if (input.trade_cnt > 0) {
        result = static_cast<double>(
            time_sub(input.last_trade_time, input.first_trade_time)) /
            static_cast<double>(input.trade_cnt);
    } else {
        result = 0.0;
    }
    return;
}

// factor012: 分位数相关，不需要做

/// 封板后到predict_t时刻之间非涨停价的撤单金额
void call_factor013(const FactorInput& input, double& result) {
    result = input.nzt_cancel_amount;
    return;
}

/// 封板后到predict_t时刻之间非涨停价的撤单数
void call_factor014(const FactorInput& input, int& result) {
    result = input.nzt_cancel_cnt;
    return;
}

/// 封板后到predict_t时刻之间非涨停价的挂单金额
void call_factor015(const FactorInput& input, double& result) {
    result = input.nzt_add_amount;
    return;
}

/// 封板后到predict_t时刻之间非涨停价的挂单数
void call_factor016(const FactorInput& input, int& result) {
    result = input.nzt_add_cnt;
    return;
}

/// 封板后到predict_t时刻之间涨停价订单平均每笔成交量
void call_factor017(const FactorInput& input, double& result) {
    if (input.zt_trade_cnt > 0) {
        result = input.zt_trade_vol_sum /
            static_cast<double>(input.zt_trade_cnt);
    } else {
        result = 0.0;
    }
    return;
}

/// 封板后到predict_t时刻之间涨停价订单平均每笔成交金额
void call_factor018(const FactorInput& input, double& result) {
    if (input.zt_trade_cnt > 0) {
        result = input.zt_trade_amount_sum /
            static_cast<double>(input.zt_trade_cnt) / 10000.0;
    } else {
        result = 0.0;
    }
    return;
}

/// 封板后到predict_t时刻之间涨停价订单的成交笔数
void call_factor019(const FactorInput& input, int& result) {
    result = input.zt_trade_cnt;
    return;
}

/// 封板后到predict_t时刻之间非涨停价订单的成交笔数
void call_factor020(const FactorInput& input, int& result) {
    result = input.nzt_trade_cnt;
    return;
}

/// 封板订单的委托金额
void call_factor021(const FactorInput& input, double& result) {
    result = input.lock_order_amount;
    return;
}

//
