// This file is part of the Yu programming language and is licensed under MIT License;
// See LICENSE.txt for details

#include "../include/parser.h"

namespace yu::frontend
{
    #if defined(YUMINA_ARCH_X64)
        bool token_compare(lang::token_i current, const lang::token_i* types, size_t count)
        {
            const __m256i current_vec = _mm256_set1_epi8(static_cast<uint8_t>(current));
            for (size_t i = 0; i < count; i += 32)
            {
                const size_t remaining = std::min(size_t(32), count - i);
                __m256i compare_vec;

                if (remaining == 32)
                {
                    compare_vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(types + i));
                }
                else
                {
                    alignas(32) uint8_t temp[32] = {0};
                    memcpy(temp, types + i, remaining);
                    compare_vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(temp));
                }

                const __m256i result = _mm256_cmpeq_epi8(current_vec, compare_vec);
                const uint32_t mask = _mm256_movemask_epi8(result);

                if (mask != 0)
                    return true;
            }
            return false;
        }

    #elif defined(YUMINA_ARCH_ARM64)
        bool token_compare(lang::token_i current, const lang::token_i *types, const size_t count)
        {
            const uint8x16_t current_vec = vdupq_n_u8(static_cast<uint8_t>(current));
            for (size_t i = 0; i < count; i += 16)
            {
                const size_t remaining = std::min(static_cast<size_t>(16), count - i);
                uint8x16_t compare_vec;

                if (remaining == 16)
                {
                    compare_vec = vld1q_u8(reinterpret_cast<const uint8_t*>(types + i));
                }
                else
                {
                    alignas(16) uint8_t temp[16] = {0};
                    memcpy(temp, types + i, remaining);
                    compare_vec = vld1q_u8(temp);
                }

                const uint8x16_t result = vceqq_u8(current_vec, compare_vec);
                if (uint64x2_t paired = vpaddlq_u32(vpaddlq_u16(vpaddlq_u8(result)));
                    vgetq_lane_u64(paired, 0) != 0 || vgetq_lane_u64(paired, 1) != 0)
                {
                    return true;
                }
            }
            return false;
        }
    #endif

    bt::status_i match_token(parse_context *ctx, const lang::token_i type)
    {
        if (ctx->state.pos >= ctx->tokens->size())
        {
            return bt::status_i::FAILURE;
        }

        if (ctx->tokens->types[static_cast<std::vector<lang::token_i>::size_type>(ctx->state.pos)] == type)
        {
            ctx->state.pos++;
            return bt::status_i::SUCCESS;
        }
        return bt::status_i::FAILURE;
    }

    bt::status_i match_tokens(parse_context *ctx, const lang::token_i *types, size_t count)
    {
        if (ctx->state.pos >= ctx->tokens->size())
        {
            return bt::status_i::FAILURE;
        }
        const auto current = ctx->tokens->types[static_cast<std::vector<lang::token_i>::size_type>(ctx->state.pos)];
        #if defined(YUMINA_ARCH_X64) || defined(YUMINA_ARCH_ARM64)
            if (token_compare(current, types, count))
            {
                ctx->state.pos++;
                return bt::status_i::SUCCESS;
            }
        #else
            for (size_t i = 0; i < count; i++)
            {
                if (current == types[i])
                {
                    ctx->state.pos++;
                    return bt::status_i::SUCCESS;
                }
            }
        #endif

        return bt::status_i::FAILURE;
    }
}
