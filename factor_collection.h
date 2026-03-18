//
// Created by even on 2026/3/17.
//

#ifndef LIMIT_UP_FACTOR_DEMO_FACTOR_COLLECTION_H
#define LIMIT_UP_FACTOR_DEMO_FACTOR_COLLECTION_H

#include <cmath>
#include <vector>

#include "cross_section_utils.h"
#include "factor.h"
#include "factor01_031_extract.h"
#include "factor032_040_extract.h"
#include "factor_result.h"
#include "trade_vwap_utils.h"

inline void build_factor01_031_state(
    const FactorInput& input,
    factor01_031_detail::Factor01_031State& state) {
    factor01_031_detail::build_factor01_031_state(input, state);
}

inline factor01_031_detail::Factor01_031Result collect_factor001_031(
    const FactorInput& input) {
    return factor01_031_detail::collect_factor001_031(input);
}

inline factor01_031_detail::Factor01_031Result collect_factor001_031(
    const FactorInput& input,
    const factor01_031_detail::Factor01_031State& state,
    factor01_031_detail::Factor01_031Workspace& workspace) {
    return factor01_031_detail::collect_factor001_031(input, state, workspace);
}

inline void build_factor032_040_state(
    const FactorInput& input,
    factor032_040_detail::Factor032_040State& state) {
    factor032_040_detail::build_factor032_040_state(input, state);
}

inline factor032_040_detail::Factor032_040Result collect_factor032_040(
    const FactorInput& input) {
    return factor032_040_detail::collect_factor032_040(input);
}

inline factor032_040_detail::Factor032_040Result collect_factor032_040(
    const FactorInput& input,
    const factor032_040_detail::Factor032_040State& state,
    factor032_040_detail::Factor032_040Workspace& workspace) {
    return factor032_040_detail::collect_factor032_040(input, state, workspace);
}

inline factor01_031_detail::Factor041_050Result collect_factor041_050(
    const FactorInput& input) {
    return factor01_031_detail::collect_factor041_050(input);
}

inline factor01_031_detail::Factor041_050Result collect_factor041_050(
    const FactorInput& input,
    const factor01_031_detail::Factor01_031State& state,
    factor01_031_detail::Factor01_031Workspace& workspace) {
    return factor01_031_detail::collect_factor041_050(input, state, workspace);
}

inline void build_factor053_061_state(
    const FactorInput& input,
    factor_vwap_detail::VwapRatioStatsState& state) {
    factor_vwap_detail::build_vwap_ratio_stats(input, state);
}

inline factor_vwap_detail::VwapRatioFactorResult collect_factor053_061(
    const FactorInput& input) {
    return factor_vwap_detail::collect_factor053_061(input);
}

inline factor_vwap_detail::VwapRatioFactorResult collect_factor053_061(
    const FactorInput& input,
    const factor_vwap_detail::VwapRatioStatsState& state) {
    (void)input;
    return factor_vwap_detail::collect_factor053_061(state);
}

inline AllFactorResult collect_all_factors(const FactorInput& input) {
    AllFactorResult all = {};

    factor01_031_detail::Factor01_031State state_01_031;
    factor01_031_detail::build_factor01_031_state(input, state_01_031);
    factor01_031_detail::Factor01_031Workspace workspace_01_031;
    const factor01_031_detail::Factor01_031Result r1 =
        factor01_031_detail::collect_factor001_031(input, state_01_031, workspace_01_031);
    const factor01_031_detail::Factor041_050Result r4 =
        factor01_031_detail::collect_factor041_050(input, state_01_031, workspace_01_031);

    factor032_040_detail::Factor032_040State state_032_040;
    factor032_040_detail::build_factor032_040_state(input, state_032_040);
    factor032_040_detail::Factor032_040Workspace workspace_032_040;
    const factor032_040_detail::Factor032_040Result r3 =
        factor032_040_detail::collect_factor032_040(input, state_032_040, workspace_032_040);

    factor_vwap_detail::VwapRatioStatsState state_053_061;
    factor_vwap_detail::build_vwap_ratio_stats(input, state_053_061);
    const factor_vwap_detail::VwapRatioFactorResult r5 =
        factor_vwap_detail::collect_factor053_061(state_053_061);

    all.factor001 = r1.factor001;
    all.factor002 = r1.factor002;
    all.factor003 = r1.factor003;
    all.factor004 = r1.factor004;
    all.factor005 = r1.factor005;
    all.factor006 = r1.factor006;
    all.factor007 = r1.factor007;
    all.factor008 = r1.factor008;
    all.factor009 = r1.factor009;
    all.factor010 = r1.factor010;
    all.factor011 = r1.factor011;
    all.factor012 = r1.factor012;
    all.factor013 = r1.factor013;
    all.factor014 = r1.factor014;
    all.factor015 = r1.factor015;
    all.factor016 = r1.factor016;
    all.factor017 = r1.factor017;
    all.factor018 = r1.factor018;
    all.factor019 = r1.factor019;
    all.factor020 = r1.factor020;
    all.factor021 = r1.factor021;
    all.factor022 = r1.factor022;
    all.factor023 = r1.factor023;
    all.factor024 = r1.factor024;
    all.factor025 = r1.factor025;
    all.factor026 = r1.factor026;
    all.factor027 = r1.factor027;
    all.factor028 = r1.factor028;
    all.factor029 = r1.factor029;
    all.factor030 = r1.factor030;
    all.factor031 = r1.factor031;

    all.factor032 = r3.factor032;
    all.factor033 = r3.factor033;
    all.factor034 = r3.factor034;
    all.factor035 = r3.factor035;
    all.factor036 = r3.factor036;
    all.factor037 = r3.factor037;
    all.factor038 = r3.factor038;
    all.factor039 = r3.factor039;
    all.factor040 = r3.factor040;

    all.factor041 = r4.factor041;
    all.factor042 = r4.factor042;
    all.factor043 = r4.factor043;
    all.factor044 = r4.factor044;
    all.factor045 = r4.factor045;
    all.factor046 = r4.factor046;
    all.factor047 = r4.factor047;
    all.factor048 = r4.factor048;
    all.factor049 = r4.factor049;
    all.factor050 = r4.factor050;

    all.factor053 = r5.factor053;
    all.factor054 = r5.factor054;
    all.factor055 = r5.factor055;
    all.factor056 = r5.factor056;
    all.factor057 = r5.factor057;
    all.factor058 = r5.factor058;
    all.factor059 = r5.factor059;
    all.factor060 = r5.factor060;
    all.factor061 = r5.factor061;
    return all;
}

inline void call_factor001(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor001(state);
}

inline void call_factor002(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor002(state);
}

inline void call_factor003(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor003(state);
}

inline void call_factor004(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor004(state);
}

inline void call_factor005(const FactorInput& input, double& result) {
    result = factor01_031_detail::get_factor005(input);
}

inline void call_factor006(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor006(state);
}

inline void call_factor007(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor007(state);
}

inline void call_factor008(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor008(state);
}

inline void call_factor009(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor009(state);
}

inline void call_factor010(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor010(state);
}

inline void call_factor011(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor011(state);
}

inline void call_factor012(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    factor01_031_detail::Factor01_031Workspace workspace;
    result = factor01_031_detail::get_factor012(state, workspace);
}

inline void call_factor013(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor013(state);
}

inline void call_factor014(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor014(state);
}

inline void call_factor015(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor015(state);
}

inline void call_factor016(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor016(state);
}

inline void call_factor017(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor017(state);
}

inline void call_factor018(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor018(state);
}

inline void call_factor019(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor019(state);
}

inline void call_factor020(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor020(state);
}

inline void call_factor021(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor021(state);
}

inline void call_factor022(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor022(state);
}

inline void call_factor023(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor023(state);
}

inline void call_factor024(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    factor01_031_detail::Factor01_031Workspace workspace;
    result = factor01_031_detail::get_factor024(state, workspace, input);
}

inline void call_factor025(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    factor01_031_detail::Factor01_031Workspace workspace;
    result = factor01_031_detail::get_factor025(state, workspace, input);
}

inline void call_factor026(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    factor01_031_detail::Factor01_031Workspace workspace;
    result = factor01_031_detail::get_factor026(state, workspace, input);
}

inline void call_factor027(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    factor01_031_detail::Factor01_031Workspace workspace;
    result = factor01_031_detail::get_factor027(state, workspace, input);
}

inline void call_factor028(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor028(state);
}

inline void call_factor029(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor029(state, input);
}

inline void call_factor030(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor030(state);
}

inline void call_factor031(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor031(state);
}

inline void call_factor032(const FactorInput& input, double& result) {
    const factor032_040_detail::Factor032_040State state =
        factor032_040_detail::build_factor032_040_state(input);
    result = factor032_040_detail::get_factor032(state);
}

inline void call_factor033(const FactorInput& input, double& result) {
    const factor032_040_detail::Factor032_040State state =
        factor032_040_detail::build_factor032_040_state(input);
    result = factor032_040_detail::get_factor033(state);
}

inline void call_factor034(const FactorInput& input, double& result) {
    const factor032_040_detail::Factor032_040State state =
        factor032_040_detail::build_factor032_040_state(input);
    result = factor032_040_detail::get_factor034(state);
}

inline void call_factor035(const FactorInput& input, double& result) {
    const factor032_040_detail::Factor032_040State state =
        factor032_040_detail::build_factor032_040_state(input);
    result = factor032_040_detail::get_factor035(input, state);
}

inline void call_factor036(const FactorInput& input, double& result) {
    const factor032_040_detail::Factor032_040State state =
        factor032_040_detail::build_factor032_040_state(input);
    factor032_040_detail::Factor032_040Workspace workspace;
    result = factor032_040_detail::get_factor036(state, workspace);
}

inline void call_factor037(const FactorInput& input, double& result) {
    const factor032_040_detail::Factor032_040State state =
        factor032_040_detail::build_factor032_040_state(input);
    factor032_040_detail::Factor032_040Workspace workspace;
    result = factor032_040_detail::get_factor037(state, workspace);
}

inline void call_factor038(const FactorInput& input, double& result) {
    const factor032_040_detail::Factor032_040State state =
        factor032_040_detail::build_factor032_040_state(input);
    result = factor032_040_detail::get_factor038(input, state);
}

inline void call_factor039(const FactorInput& input, double& result) {
    const factor032_040_detail::Factor032_040State state =
        factor032_040_detail::build_factor032_040_state(input);
    result = factor032_040_detail::get_factor039(input, state);
}

inline void call_factor040(const FactorInput& input, double& result) {
    const factor032_040_detail::Factor032_040State state =
        factor032_040_detail::build_factor032_040_state(input);
    result = factor032_040_detail::get_factor040(state);
}

inline void call_factor041(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor041(state);
}

inline void call_factor042(const FactorInput& input, double& result) {
    result = factor01_031_detail::get_factor042(input);
}

inline void call_factor043(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor043(state, input);
}

inline void call_factor044(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor044(state);
}

inline void call_factor045(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor045(state);
}

inline void call_factor046(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor046(state);
}

inline void call_factor047(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor047(state);
}

inline void call_factor048(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    result = factor01_031_detail::get_factor048(state, input);
}

inline void call_factor049(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    factor01_031_detail::Factor01_031Workspace workspace;
    result = factor01_031_detail::get_factor049(state, workspace);
}

inline void call_factor050(const FactorInput& input, double& result) {
    const factor01_031_detail::Factor01_031State state =
        factor01_031_detail::build_factor01_031_state(input);
    factor01_031_detail::Factor01_031Workspace workspace;
    result = factor01_031_detail::get_factor050(state, workspace, input);
}

#endif //LIMIT_UP_FACTOR_DEMO_FACTOR_COLLECTION_H
