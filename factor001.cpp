//
// Created by even on 2026/3/17.
//

#include "factor.h"
#include "factor_collection.h"

/// 依照 call_factorxxx 进行编名
/// [in]    input   输入的上层标量
/// [out]   result  因子计算结果，类型可以自定
void call_factor001(const FactorInput& input, double& result) {
    // 进行因子计算主体逻辑

    result = static_cast<double>(input.last_order.n_volume) *
        static_cast<double>(input.last_order.n_price);
}


void call_factor006(const FactorInput& input, double& result) {
    if (!result >0){
        result = 0;
    }

}
