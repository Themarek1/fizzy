// Fizzy: A fast WebAssembly interpreter
// Copyright 2019-2020 The Fizzy Authors.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#if __cplusplus > 201703L

#include <bit>

namespace fizzy
{
using popcount = std::popcount;
}

#else

#include <cstdint>

namespace fizzy
{
constexpr int popcount(uint32_t x) noexcept
{
    return __builtin_popcount(x);
}

constexpr int popcount(uint64_t x) noexcept
{
    return __builtin_popcountll(x);
}
}  // namespace fizzy

#endif /* __cplusplus > 201703L */
