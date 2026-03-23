#pragma once
#include "common.h"

namespace FoxSQL
{
    constexpr size_t B_PLUS_TREE_ORDER = 4;
    constexpr size_t B_PLUS_TREE_MAX_KEYS = B_PLUS_TREE_ORDER - 1;
    constexpr size_t B_PLUS_TREE_MIN_KEYS = (B_PLUS_TREE_ORDER / 2) - 1;

    using Key = int64_t;
}