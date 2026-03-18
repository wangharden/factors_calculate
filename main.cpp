#include <iostream>

#include "factor.h"
#include "factor_collection.h"

int main() {
    std::cout << "因子计算示例" << std::endl;

    FactorInput input {

        FactorOrderInfo {
            93000000,
            10000,
            100000,
            2800,
            1700,
            FACTOR_BSFLAG_BUY,
            FACTOR_ORDER_TYPE_NORMAL,
            200,
            100,
            5000,
            2000,
            20,
        },

        FactorTransInfo {
            93000000,
            10000,
            100000,
            2800,
            0,
            1800,
            FACTOR_BSFLAG_BUY,
            200,
            300,
            100000,
            30
        },

    };

    double result = 0.0;
    call_factor001(input, result);

    std::cout << "因子计算结果: " << result << std::endl;

    return 0;
}
