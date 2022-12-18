/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#define INTERNAL_EXPAND(x) x
#define CONCATENATE(arg1, arg2) CONCATENATE1(arg1, arg2)
#define CONCATENATE1(arg1, arg2) CONCATENATE2(arg1, arg2)
#define CONCATENATE2(arg1, arg2) arg1##arg2

#define FOR_EACH_BY_PAIR_2(what, a, x, y) what(a, x, y)
#define FOR_EACH_BY_PAIR_4(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_2(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_6(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_4(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_8(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_6(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_10(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_8(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_12(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_10(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_14(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_12(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_16(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_14(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_18(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_16(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_20(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_18(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_22(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_20(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_24(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_22(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_26(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_24(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_28(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_26(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_30(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_28(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_32(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_30(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_34(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_32(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_36(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_34(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_38(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_36(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_40(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_38(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_42(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_40(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_44(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_42(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_46(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_44(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_48(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_46(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_50(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_48(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_52(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_50(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_54(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_52(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_56(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_54(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_58(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_56(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_60(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_58(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_62(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_60(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_64(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_62(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_66(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_64(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_68(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_66(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR_70(what, a, x, y, ...) what(a, x, y) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_68(what, a, __VA_ARGS__))

#define FOR_EACH_BY_PAIR_IMPL_(N, what, a, ...) INTERNAL_EXPAND(CONCATENATE(FOR_EACH_BY_PAIR_, N)(what, a, __VA_ARGS__))
#define FOR_EACH_BY_PAIR(what, a, ...) INTERNAL_EXPAND(FOR_EACH_BY_PAIR_IMPL_(GET_ARG_COUNT(__VA_ARGS__), what, a, __VA_ARGS__))

#define FOR_EACH_BY_SINGLE_1(what, a, x, ...) what(a, x)
#define FOR_EACH_BY_SINGLE_2(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_1(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_3(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_2(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_4(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_3(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_5(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_4(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_6(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_5(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_7(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_6(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_8(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_7(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_9(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_8(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_10(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_9(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_11(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_10(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_12(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_11(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_13(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_12(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_14(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_13(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_15(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_14(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_16(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_15(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_17(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_16(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_18(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_17(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_19(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_18(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_20(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_19(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_21(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_20(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_22(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_21(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_23(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_22(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_24(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_23(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_25(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_24(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_26(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_25(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_27(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_26(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_28(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_27(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_29(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_28(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_30(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_29(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_31(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_30(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_32(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_31(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_33(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_32(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_34(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_33(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_35(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_34(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_36(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_35(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_37(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_36(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_38(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_37(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_39(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_38(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_40(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_39(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_41(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_40(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_42(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_41(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_43(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_42(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_44(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_43(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_45(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_44(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_46(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_45(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_47(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_46(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_48(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_47(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_49(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_48(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_50(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_49(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_51(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_50(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_52(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_51(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_53(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_52(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_54(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_53(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_55(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_54(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_56(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_55(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_57(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_56(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_58(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_57(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_59(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_58(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_60(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_59(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_61(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_60(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_62(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_61(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_63(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_62(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_64(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_63(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_65(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_64(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_66(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_65(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_67(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_66(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_68(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_67(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_69(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_68(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE_70(what, a, x, ...) what(a, x) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_69(what, a, __VA_ARGS__))

#define FOR_EACH_BY_SINGLE_IMPL_(N, what, a, ...) INTERNAL_EXPAND(CONCATENATE(FOR_EACH_BY_SINGLE_, N)(what, a, __VA_ARGS__))
#define FOR_EACH_BY_SINGLE(what, a, ...) INTERNAL_EXPAND(FOR_EACH_BY_SINGLE_IMPL_(GET_ARG_COUNT(__VA_ARGS__), what, a, __VA_ARGS__))

#ifdef _MSC_VER // Microsoft compilers

#define GET_ARG_COUNT(...) INTERNAL_EXPAND_ARGS_PRIVATE(INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))
#define INTERNAL_ARGS_AUGMENTER(...) unused, __VA_ARGS__
#define INTERNAL_EXPAND_ARGS_PRIVATE(...) INTERNAL_EXPAND(INTERNAL_GET_ARG_COUNT_PRIVATE(__VA_ARGS__, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#define INTERNAL_GET_ARG_COUNT_PRIVATE(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count

#else // Non-Microsoft compilers

#define GET_ARG_COUNT(...) INTERNAL_GET_ARG_COUNT_PRIVATE(0, ##__VA_ARGS__, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define INTERNAL_GET_ARG_COUNT_PRIVATE(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count

#endif

#ifndef NDEBUG
static_assert(GET_ARG_COUNT() == 0, "GET_ARG_COUNT() failed for 0 arguments");
static_assert(GET_ARG_COUNT(1) == 1, "GET_ARG_COUNT() failed for 1 argument");
static_assert(GET_ARG_COUNT(1, 2) == 2, "GET_ARG_COUNT() failed for 2 arguments");
static_assert(GET_ARG_COUNT(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70) == 70, "GET_ARG_COUNT() failed for 70 arguments");
#endif
