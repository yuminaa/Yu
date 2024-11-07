// This file is part of the Yu programming language and is licensed under MIT License;
// See LICENSE.txt for details

#pragma once

#ifndef BT_H
#define BT_H

#include <algorithm>
#include <cstdint>
#include "arch.hpp"

namespace yu::bt
{
    enum class status_i : uint8_t
    {
        SUCCESS = 0,
        FAILURE = 1,
        RUNNING = 2,
    };

    template<typename context>
    using node = status_i(*)(context*);

    template<typename context>
    ALWAYS_INLINE HOT_FUNCTION
    status_i sequence(const node<context>* nodes, const size_t count, context* ctx)
    {
        for (size_t i = 0; i < count; ++i)
        {
            if (const status_i status = nodes[i](ctx); status == status_i::FAILURE || status == status_i::RUNNING)
            {
                return status;
            }
        }
        return status_i::SUCCESS;
    }

    template<typename context>
    ALWAYS_INLINE HOT_FUNCTION
    status_i fallback(const node<context>* nodes, const size_t count, context* ctx)
    {
        for (size_t i = 0; i < count; ++i)
        {
            if (const status_i status = nodes[i](ctx); status == status_i::SUCCESS || status == status_i::RUNNING)
            {
                return status;
            }
        }
        return status_i::FAILURE;
    }
}

#endif