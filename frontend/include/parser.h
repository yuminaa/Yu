// This file is part of the Yu programming language and is licensed under MIT License;
// See LICENSE.txt for details

#ifndef YU_PARSER_HPP
#define YU_PARSER_HPP

#include <string_view>
#include <vector>
#include "../../common/arch.hpp"
#include "../../common/bt.hpp"
#include "../../yu/include/tokens.h"

namespace yu::frontend
{
    struct ir_node;

    struct parse_context
    {
        const char* src;
        const lang::TokenList *tokens{};
        ir_node *current{};

        struct
        {
            uint32_t pos: 24;
            uint32_t in_error: 1;
            uint32_t depth: 7;
        } state{};

        std::vector<ir_node *> scope_stack;
    };

    enum class ir_t : uint8_t
    {
        NODE_CLASS,
        NODE_METHOD,
        NODE_FIELD,
        NODE_VARIABLE,
        NODE_EXPRESSION,
        NODE_TYPE,
        NODE_BLOCK,
        NODE_RETURN,
        NODE_IF,
        NODE_LOOP,
        NODE_BINARY_OP,
        NODE_UNARY_OP,
        NODE_LITERAL,
        NODE_IDENTIFIER
    };

    struct ir_node
    {
        ir_t type;
        std::vector<ir_node *> children;

        union
        {
            struct
            {
                char *text;
                size_t length;
            } str_val;

            double num_val;
            bool bool_val;
            uint8_t op_val;
        } value;
    };

    HOT_FUNCTION
    bt::status_i parse_expression(parse_context *ctx);

    HOT_FUNCTION
    bt::status_i parse_assignment(parse_context *ctx);

    HOT_FUNCTION
    bt::status_i parse_logical_or(parse_context *ctx);

    HOT_FUNCTION
    bt::status_i parse_logical_and(parse_context *ctx);

    HOT_FUNCTION
    bt::status_i parse_equality(parse_context *ctx);

    HOT_FUNCTION
    bt::status_i parse_comparison(parse_context *ctx);

    HOT_FUNCTION
    bt::status_i parse_term(parse_context *ctx);

    HOT_FUNCTION
    bt::status_i parse_factor(parse_context *ctx);

    HOT_FUNCTION
    bt::status_i parse_unary(parse_context *ctx);

    HOT_FUNCTION
    bt::status_i parse_primary(parse_context *ctx);

    // Statement parsing functions
    HOT_FUNCTION
    bt::status_i parse_statement(parse_context *ctx);

    HOT_FUNCTION
    bt::status_i parse_block(parse_context *ctx);

    HOT_FUNCTION
    bt::status_i parse_variable(parse_context *ctx);

    HOT_FUNCTION
    bt::status_i parse_if_statement(parse_context *ctx);

    HOT_FUNCTION
    bt::status_i parse_for_loop(parse_context *ctx);

    HOT_FUNCTION
    bt::status_i parse_while_loop(parse_context *ctx);

    HOT_FUNCTION
    bt::status_i parse_return_statement(parse_context *ctx);

    // Type parsing functions
    HOT_FUNCTION
    bt::status_i parse_type(parse_context *ctx);

    HOT_FUNCTION
    bt::status_i parse_generic_params(parse_context *ctx);

    // Class parsing functions
    HOT_FUNCTION
    bt::status_i parse_class(parse_context *ctx);

    HOT_FUNCTION
    bt::status_i parse_class_header(parse_context *ctx);

    HOT_FUNCTION
    bt::status_i parse_class_body(parse_context *ctx);

    HOT_FUNCTION
    bt::status_i parse_class_member(parse_context *ctx);

    // Core parsing functions
    HOT_FUNCTION
    bt::status_i match_token(parse_context *ctx, lang::token_i type);

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i match_tokens(parse_context *ctx, const lang::token_i *types, size_t count);

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i sync_error(parse_context *ctx, lang::token_i sync_token);

    // Node creation/destruction
    HOT_FUNCTION
    ir_node *create_node(ir_t type);

    HOT_FUNCTION
    void destroy_node(ir_node *node);

    std::unique_ptr<ir_node> parse(const char* src, const lang::TokenList *tokens);

    HOT_FUNCTION
    parse_context *create_parse_context(const lang::TokenList *tokens);

    HOT_FUNCTION
    void destroy_parse_context(parse_context *ctx);

    #if defined(YUMINA_ARCH_X64)
    ALWAYS_INLINE HOT_FUNCTION
        bool token_compare(lang::token_i current, const lang::token_i* types, size_t count);
    #elif defined(YUMINA_ARCH_ARM64)
        HOT_FUNCTION
        bool token_compare(lang::token_i current, const lang::token_i *types, size_t count);
    #endif
}

#endif
