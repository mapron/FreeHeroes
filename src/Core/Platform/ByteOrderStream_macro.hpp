/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

/// File contains som compiler-defines guess code for byteorder.

#define ORDER_LE 0
//The least significant byte (LSB) value is at the lowest address. The other bytes follow in increasing order of significance.

#define ORDER_BE 1
// The most significant byte (MSB) value is stored at the memory location with the lowest address, the next byte value in significance is stored at the following memory location and so on

#if defined(__BYTE_ORDER__) // GCC
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define HOST_BYTE_ORDER_INT ORDER_BE
#define HOST_WORD_ORDER_INT ORDER_BE
#define HOST_DWORD_ORDER_INT ORDER_BE
#define HOST_BYTE_ORDER_FLOAT ORDER_BE
#define HOST_WORD_ORDER_FLOAT ORDER_BE
#endif
#if __BYTE_ORDER__ == __ORDER_PDP_ENDIAN__
#define HOST_BYTE_ORDER_INT ORDER_LE
#define HOST_WORD_ORDER_INT ORDER_BE
#endif
#if __FLOAT_WORD_ORDER__ == __ORDER_BIG_ENDIAN__
#define HOST_WORD_ORDER_FLOAT ORDER_BE
#endif
#endif
#if defined(__arm__) || defined(__ARM__)
#define HOST_DWORD_ORDER_DOUBLE ORDER_BE
#endif

// If we have no euristic check or compile-time defines, set default values (little-endian):
// Byte orders for integers
#ifndef HOST_BYTE_ORDER_INT
#define HOST_BYTE_ORDER_INT ORDER_LE
#endif
#ifndef HOST_WORD_ORDER_INT
#define HOST_WORD_ORDER_INT ORDER_LE
#endif
#ifndef HOST_DWORD_ORDER_INT64
#define HOST_DWORD_ORDER_INT64 ORDER_LE
#endif
// byte orders for float and double.
#ifndef HOST_BYTE_ORDER_FLOAT
#define HOST_BYTE_ORDER_FLOAT ORDER_LE
#endif
#ifndef HOST_WORD_ORDER_FLOAT
#define HOST_WORD_ORDER_FLOAT ORDER_LE
#endif
#ifndef HOST_DWORD_ORDER_DOUBLE
#define HOST_DWORD_ORDER_DOUBLE ORDER_LE
#endif
