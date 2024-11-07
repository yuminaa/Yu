// This file is part of the Yu programming language and is licensed under MIT License;
// See LICENSE.txt for details

#include "../include/parser.h"
#include <stack>
#include "lexer.h"

namespace yu::frontend
{
    ALWAYS_INLINE HOT_FUNCTION
    ir_node *create_node(const ir_t type)
    {
        auto *node = new ir_node();
        node->type = type;
        return node;
    }

    ALWAYS_INLINE HOT_FUNCTION
    void destroy_node(ir_node *node)
    {
        if (!node)
            return;

        std::stack<ir_node *> nodes;
        nodes.push(node);

        while (!nodes.empty())
        {
            ir_node *current = nodes.top();
            nodes.pop();

            for (const auto& child:
                 current->children)
            {
                if (child)
                    nodes.emplace(child);
            }

            if (current->type == ir_t::NODE_IDENTIFIER ||
                current->type == ir_t::NODE_LITERAL)
            {
                delete[] current->value.str_val.text;
            }

            current->children.clear();
            delete current;
        }
    }

    ALWAYS_INLINE HOT_FUNCTION
    parse_context *create_parse_context(const lang::TokenList *tokens)
    {
        auto *ctx = new parse_context();
        ctx->src = nullptr;
        ctx->tokens = tokens;
        ctx->current = nullptr;
        ctx->state.pos = 0;
        ctx->state.in_error = 0;
        ctx->state.depth = 0;
        ctx->scope_stack.reserve(16);
        return ctx;
    }

    ALWAYS_INLINE HOT_FUNCTION
    void destroy_parse_context(parse_context *ctx)
    {
        if (!ctx)
            return;

        for (const auto& node: ctx->
             scope_stack)
        {
            if (node)
                destroy_node(node);
        }
        ctx->scope_stack.clear();

        if (ctx->current)
        {
            destroy_node(ctx->current);
            ctx->current = nullptr;
        }

        delete ctx;
    }

    std::unique_ptr<ir_node> parse(const char *src, const lang::TokenList *tokens)
    {
        if (!tokens || !src)
            return nullptr;

        auto *ctx = create_parse_context(tokens);
        ctx->src = src;
        // This will be later switched to a switch case to determine which function to call
        // as the language is not object-oriented
        const auto status = parse_class(ctx);
        std::unique_ptr<ir_node> result;

        if (status == bt::status_i::SUCCESS && !ctx->state.in_error)
        {
            result.reset(ctx->current);
            ctx->current = nullptr;
        }

        destroy_parse_context(ctx);
        return result;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i sync_error(parse_context *ctx, const lang::token_i sync_token)
    {
        if (!ctx)
            return bt::status_i::FAILURE;

        ctx->state.in_error = 1;

        while (ctx->state.pos < ctx->tokens->size())
        {
            if (match_token(ctx, sync_token) == bt::status_i::SUCCESS)
            {
                ctx->state.in_error = 0;
                return bt::status_i::SUCCESS;
            }
            ctx->state.pos++;
        }
        return bt::status_i::FAILURE;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_expression(parse_context *ctx) // NOLINT(*-no-recursion)
    {
        return parse_assignment(ctx);
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_assignment(parse_context *ctx)
    {
        const auto pos_backup = ctx->state.pos;
        auto *left = create_node(ir_t::NODE_EXPRESSION);
        ctx->current = left;

        if (parse_logical_or(ctx) == bt::status_i::FAILURE)
        {
            destroy_node(left);
            ctx->state.pos = pos_backup;
            return bt::status_i::FAILURE;
        }

        if (match_token(ctx, lang::token_i::EQUAL) == bt::status_i::SUCCESS)
        {
            auto *assign = create_node(ir_t::NODE_BINARY_OP);
            assign->value.op_val = static_cast<uint8_t>(lang::token_i::EQUAL);
            assign->children.push_back(left);

            auto *right = create_node(ir_t::NODE_EXPRESSION);
            ctx->current = right;

            if (parse_assignment(ctx) == bt::status_i::FAILURE)
            {
                destroy_node(assign);
                destroy_node(right);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }

            assign->children.push_back(right);
            ctx->current = assign;
        }

        return bt::status_i::SUCCESS;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_logical_or(parse_context *ctx)
    {
        const auto pos_backup = ctx->state.pos;

        if (parse_logical_and(ctx) == bt::status_i::FAILURE)
        {
            return bt::status_i::FAILURE;
        }

        while (match_token(ctx, lang::token_i::OR) == bt::status_i::SUCCESS)
        {
            auto *op = create_node(ir_t::NODE_BINARY_OP);
            op->value.op_val = static_cast<uint8_t>(lang::token_i::OR);
            op->children.push_back(ctx->current);

            if (parse_logical_and(ctx) == bt::status_i::FAILURE)
            {
                destroy_node(op);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }

            op->children.push_back(ctx->current);
            ctx->current = op;
        }

        return bt::status_i::SUCCESS;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_class(parse_context *ctx)
    {
        const auto pos_backup = ctx->state.pos;

        if (match_token(ctx, lang::token_i::CLASS) == bt::status_i::FAILURE)
        {
            return bt::status_i::FAILURE;
        }

        auto *class_node = create_node(ir_t::NODE_CLASS);
        ctx->current = class_node;

        // Parse class name (identifier)
        if (match_token(ctx, lang::token_i::IDENTIFIER) == bt::status_i::FAILURE)
        {
            destroy_node(class_node);
            ctx->state.pos = pos_backup;
            return bt::status_i::FAILURE;
        }

        // Store class name
        ctx->tokens->types[static_cast<std::vector<unsigned>::size_type>(ctx->state.pos - 1)];
        ctx->tokens->starts[static_cast<std::vector<unsigned>::size_type>(ctx->state.pos - 1)];
        const auto &length = ctx->tokens->lengths[static_cast<std::vector<unsigned>::size_type>(ctx->state.pos - 1)];

        auto *name_node = create_node(ir_t::NODE_IDENTIFIER);
        name_node->value.str_val.text = new char[length];
        name_node->value.str_val.length = length;
        const auto value = get_token_value(ctx->src, *ctx->tokens, ctx->state.pos - 1);
        memcpy(name_node->value.str_val.text, value.data(), value.length());
        class_node->children.push_back(name_node);

        // Parse generic parameters if present
        if (match_token(ctx, lang::token_i::LESS) == bt::status_i::SUCCESS)
        {
            if (parse_generic_params(ctx) == bt::status_i::FAILURE ||
                match_token(ctx, lang::token_i::GREATER) == bt::status_i::FAILURE)
            {
                destroy_node(class_node);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }
        }

        // Parse class body
        if (parse_class_body(ctx) == bt::status_i::FAILURE)
        {
            destroy_node(class_node);
            ctx->state.pos = pos_backup;
            return bt::status_i::FAILURE;
        }

        return bt::status_i::SUCCESS;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_class_body(parse_context *ctx)
    {
        if (match_token(ctx, lang::token_i::LEFT_BRACE) == bt::status_i::FAILURE)
        {
            return bt::status_i::FAILURE;
        }

        auto *body_node = create_node(ir_t::NODE_BLOCK);
        ctx->current->children.push_back(body_node);

        while (ctx->state.pos < ctx->tokens->size() &&
               match_token(ctx, lang::token_i::RIGHT_BRACE) == bt::status_i::FAILURE)
        {
            if (parse_class_member(ctx) == bt::status_i::FAILURE)
            {
                return sync_error(ctx, lang::token_i::RIGHT_BRACE);
            }
            body_node->children.push_back(ctx->current);
        }

        return bt::status_i::SUCCESS;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_method(parse_context *ctx, const bool has_visibility, const ir_t visibility)
    {
        const auto pos_backup = ctx->state.pos;

        auto *method_node = create_node(ir_t::NODE_METHOD);
        ctx->current = method_node;

        if (has_visibility)
        {
            auto *visibility_node = create_node(visibility);
            method_node->children.push_back(visibility_node);
        }

        // Parse method name
        if (match_token(ctx, lang::token_i::IDENTIFIER) == bt::status_i::FAILURE)
        {
            destroy_node(method_node);
            ctx->state.pos = pos_backup;
            return bt::status_i::FAILURE;
        }

        // Store method name
        ctx->tokens->types[static_cast<std::vector<unsigned>::size_type>(ctx->state.pos - 1)];
        ctx->tokens->starts[static_cast<std::vector<unsigned>::size_type>(ctx->state.pos - 1)];
        const auto &name_length = ctx->tokens->lengths[static_cast<std::vector<unsigned>::size_type>(
            ctx->state.pos - 1)];

        auto *name_node = create_node(ir_t::NODE_IDENTIFIER);
        name_node->value.str_val.text = new char[name_length];
        name_node->value.str_val.length = name_length;
        const auto value = get_token_value(ctx->src, *ctx->tokens, ctx->state.pos - 1);
        memcpy(name_node->value.str_val.text, value.data(), value.length());
        method_node->children.push_back(name_node);

        // Parse method parameters
        if (match_token(ctx, lang::token_i::LEFT_PAREN) == bt::status_i::FAILURE)
        {
            destroy_node(method_node);
            ctx->state.pos = pos_backup;
            return bt::status_i::FAILURE;
        }

        while (match_token(ctx, lang::token_i::RIGHT_PAREN) == bt::status_i::FAILURE)
        {
            if (parse_variable(ctx) == bt::status_i::FAILURE)
            {
                destroy_node(method_node);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }
            method_node->children.push_back(ctx->current);

            if (match_token(ctx, lang::token_i::COMMA) == bt::status_i::FAILURE &&
                match_token(ctx, lang::token_i::RIGHT_PAREN) == bt::status_i::FAILURE)
            {
                destroy_node(method_node);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }
        }

        // Parse return type (checking for both '-' and '>' separately)
        if (match_token(ctx, lang::token_i::MINUS) == bt::status_i::SUCCESS)
        {
            if (match_token(ctx, lang::token_i::GREATER) == bt::status_i::FAILURE)
            {
                destroy_node(method_node);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }

            if (parse_type(ctx) == bt::status_i::FAILURE)
            {
                destroy_node(method_node);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }
            method_node->children.push_back(ctx->current);
        }

        // Parse method body
        if (parse_block(ctx) == bt::status_i::FAILURE)
        {
            destroy_node(method_node);
            ctx->state.pos = pos_backup;
            return bt::status_i::FAILURE;
        }

        return bt::status_i::SUCCESS;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_field(parse_context *ctx, const bool has_visibility, const ir_t visibility)
    {
        const auto pos_backup = ctx->state.pos;

        auto *field_node = create_node(ir_t::NODE_FIELD);
        ctx->current = field_node;

        // Add visibility node if present
        if (has_visibility)
        {
            auto *visibility_node = create_node(visibility);
            field_node->children.push_back(visibility_node);
        }

        // Parse field name (identifier)
        if (match_token(ctx, lang::token_i::IDENTIFIER) == bt::status_i::FAILURE)
        {
            destroy_node(field_node);
            ctx->state.pos = pos_backup;
            return bt::status_i::FAILURE;
        }

        // Store field name
        ctx->tokens->types[static_cast<std::vector<unsigned>::size_type>(ctx->state.pos - 1)];
        ctx->tokens->starts[static_cast<std::vector<unsigned>::size_type>(ctx->state.pos - 1)];
        const auto &name_length = ctx->tokens->lengths[static_cast<std::vector<unsigned>::size_type>(
            ctx->state.pos - 1)];

        auto *name_node = create_node(ir_t::NODE_IDENTIFIER);
        name_node->value.str_val.text = new char[name_length];
        name_node->value.str_val.length = name_length;
        const auto value = get_token_value(ctx->src, *ctx->tokens, ctx->state.pos - 1);
        memcpy(name_node->value.str_val.text, value.data(), value.length());
        field_node->children.push_back(name_node);

        // Parse type annotation (: Type)
        if (match_token(ctx, lang::token_i::COLON) == bt::status_i::FAILURE)
        {
            destroy_node(field_node);
            ctx->state.pos = pos_backup;
            return bt::status_i::FAILURE;
        }

        if (parse_type(ctx) == bt::status_i::FAILURE)
        {
            destroy_node(field_node);
            ctx->state.pos = pos_backup;
            return bt::status_i::FAILURE;
        }
        field_node->children.push_back(ctx->current);

        // Parse optional initializer (= expression)
        if (match_token(ctx, lang::token_i::EQUAL) == bt::status_i::SUCCESS)
        {
            if (parse_expression(ctx) == bt::status_i::FAILURE)
            {
                destroy_node(field_node);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }
            field_node->children.push_back(ctx->current);
        }

        // Expect semicolon
        if (match_token(ctx, lang::token_i::SEMICOLON) == bt::status_i::FAILURE)
        {
            destroy_node(field_node);
            ctx->state.pos = pos_backup;
            return bt::status_i::FAILURE;
        }

        return bt::status_i::SUCCESS;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_class_member(parse_context *ctx)
    {
        // Parse member visibility
        auto has_visibility = false;
        auto visibility = ir_t::NODE_IDENTIFIER;

        if (match_token(ctx, lang::token_i::PUBLIC) == bt::status_i::SUCCESS ||
            match_token(ctx, lang::token_i::PRIVATE) == bt::status_i::SUCCESS ||
            match_token(ctx, lang::token_i::PROTECTED) == bt::status_i::SUCCESS)
        {
            has_visibility = true;
            visibility = ir_t::NODE_IDENTIFIER;
        }

        // Parse member type (method or field)
        if (match_token(ctx, lang::token_i::FUNCTION) == bt::status_i::SUCCESS)
        {
            return parse_method(ctx, has_visibility, visibility);
        }
        if (match_token(ctx, lang::token_i::VAR) == bt::status_i::SUCCESS)
        {
            return parse_field(ctx, has_visibility, visibility);
        }

        return bt::status_i::FAILURE;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_block(parse_context *ctx)
    {
        if (match_token(ctx, lang::token_i::LEFT_BRACE) == bt::status_i::FAILURE)
        {
            return bt::status_i::FAILURE;
        }

        auto *block_node = create_node(ir_t::NODE_BLOCK);
        ctx->current = block_node;

        while (ctx->state.pos < ctx->tokens->size() &&
               match_token(ctx, lang::token_i::RIGHT_BRACE) == bt::status_i::FAILURE)
        {
            if (parse_statement(ctx) == bt::status_i::FAILURE)
            {
                return sync_error(ctx, lang::token_i::RIGHT_BRACE);
            }
            block_node->children.push_back(ctx->current);
        }

        return bt::status_i::SUCCESS;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_statement(parse_context *ctx)
    {
        if (match_token(ctx, lang::token_i::IF) == bt::status_i::SUCCESS)
        {
            return parse_if_statement(ctx);
        }
        if (match_token(ctx, lang::token_i::FOR) == bt::status_i::SUCCESS)
        {
            return parse_for_loop(ctx);
        }
        if (match_token(ctx, lang::token_i::WHILE) == bt::status_i::SUCCESS)
        {
            return parse_while_loop(ctx);
        }
        if (match_token(ctx, lang::token_i::RETURN) == bt::status_i::SUCCESS)
        {
            return parse_return_statement(ctx);
        }
        if (match_token(ctx, lang::token_i::VAR) == bt::status_i::SUCCESS ||
            match_token(ctx, lang::token_i::CONST) == bt::status_i::SUCCESS)
        {
            return parse_variable(ctx);
        }

        return parse_expression(ctx);
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_if_statement(parse_context *ctx)
    {
        const auto pos_backup = ctx->state.pos;

        auto *if_node = create_node(ir_t::NODE_IF);
        ctx->current = if_node;

        // Parse condition
        if (match_token(ctx, lang::token_i::LEFT_PAREN) == bt::status_i::FAILURE ||
            parse_expression(ctx) == bt::status_i::FAILURE ||
            match_token(ctx, lang::token_i::RIGHT_PAREN) == bt::status_i::FAILURE)
        {
            destroy_node(if_node);
            ctx->state.pos = pos_backup;
            return bt::status_i::FAILURE;
        }

        if_node->children.push_back(ctx->current);

        // Parse then branch
        if (parse_statement(ctx) == bt::status_i::FAILURE)
        {
            destroy_node(if_node);
            ctx->state.pos = pos_backup;
            return bt::status_i::FAILURE;
        }

        if_node->children.push_back(ctx->current);

        if (match_token(ctx, lang::token_i::ELSE) == bt::status_i::SUCCESS)
        {
            if (parse_statement(ctx) == bt::status_i::FAILURE)
            {
                destroy_node(if_node);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }
            if_node->children.push_back(ctx->current);
        }

        return bt::status_i::SUCCESS;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_logical_and(parse_context *ctx)
    {
        const auto pos_backup = ctx->state.pos;

        if (parse_equality(ctx) == bt::status_i::FAILURE)
        {
            return bt::status_i::FAILURE;
        }

        while (match_token(ctx, lang::token_i::AND) == bt::status_i::SUCCESS)
        {
            auto *op = create_node(ir_t::NODE_BINARY_OP);
            op->value.op_val = static_cast<uint8_t>(lang::token_i::AND);
            op->children.push_back(ctx->current);

            if (parse_equality(ctx) == bt::status_i::FAILURE)
            {
                destroy_node(op);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }

            op->children.push_back(ctx->current);
            ctx->current = op;
        }

        return bt::status_i::SUCCESS;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_equality(parse_context *ctx)
    {
        const auto pos_backup = ctx->state.pos;

        if (parse_comparison(ctx) == bt::status_i::FAILURE)
        {
            return bt::status_i::FAILURE;
        }

        // Handle == and !=
        while ((match_token(ctx, lang::token_i::EQUAL) == bt::status_i::SUCCESS &&
                match_token(ctx, lang::token_i::EQUAL) == bt::status_i::SUCCESS) ||
               (match_token(ctx, lang::token_i::BANG) == bt::status_i::SUCCESS &&
                match_token(ctx, lang::token_i::EQUAL) == bt::status_i::SUCCESS))
        {
            auto *op = create_node(ir_t::NODE_BINARY_OP);
            // The first token of the pair determines if it's == or !=
            op->value.op_val = static_cast<uint8_t>(ctx->tokens->types[static_cast<std::vector<unsigned>::size_type>(
                ctx->state.pos - 2)]);
            op->children.push_back(ctx->current);

            if (parse_comparison(ctx) == bt::status_i::FAILURE)
            {
                destroy_node(op);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }

            op->children.push_back(ctx->current);
            ctx->current = op;
        }

        return bt::status_i::SUCCESS;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_comparison(parse_context *ctx)
    {
        const auto pos_backup = ctx->state.pos;

        if (parse_term(ctx) == bt::status_i::FAILURE)
        {
            return bt::status_i::FAILURE;
        }

        while (match_token(ctx, lang::token_i::LESS) == bt::status_i::SUCCESS ||
               match_token(ctx, lang::token_i::GREATER) == bt::status_i::SUCCESS)
        {
            auto *op = create_node(ir_t::NODE_BINARY_OP);
            op->value.op_val = static_cast<uint8_t>(ctx->tokens->types[static_cast<std::vector<unsigned>::size_type>(
                ctx->state.pos - 1)]);
            op->children.push_back(ctx->current);

            if (parse_term(ctx) == bt::status_i::FAILURE)
            {
                destroy_node(op);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }

            op->children.push_back(ctx->current);
            ctx->current = op;
        }

        return bt::status_i::SUCCESS;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_term(parse_context *ctx)
    {
        const auto pos_backup = ctx->state.pos;

        if (parse_factor(ctx) == bt::status_i::FAILURE)
        {
            return bt::status_i::FAILURE;
        }

        while (match_token(ctx, lang::token_i::PLUS) == bt::status_i::SUCCESS ||
               match_token(ctx, lang::token_i::MINUS) == bt::status_i::SUCCESS)
        {
            auto *op = create_node(ir_t::NODE_BINARY_OP);
            op->value.op_val = static_cast<uint8_t>(ctx->tokens->types[static_cast<std::vector<unsigned>::size_type>(
                ctx->state.pos - 1)]);
            op->children.push_back(ctx->current);

            if (parse_factor(ctx) == bt::status_i::FAILURE)
            {
                destroy_node(op);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }

            op->children.push_back(ctx->current);
            ctx->current = op;
        }

        return bt::status_i::SUCCESS;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_factor(parse_context *ctx)
    {
        const auto pos_backup = ctx->state.pos;

        if (parse_unary(ctx) == bt::status_i::FAILURE)
        {
            return bt::status_i::FAILURE;
        }

        while (match_token(ctx, lang::token_i::STAR) == bt::status_i::SUCCESS ||
               match_token(ctx, lang::token_i::SLASH) == bt::status_i::SUCCESS ||
               match_token(ctx, lang::token_i::PERCENT) == bt::status_i::SUCCESS)
        {
            auto *op = create_node(ir_t::NODE_BINARY_OP);
            op->value.op_val = static_cast<uint8_t>(ctx->tokens->types[static_cast<std::vector<unsigned>::size_type>(
                ctx->state.pos - 1)]);
            op->children.push_back(ctx->current);

            if (parse_unary(ctx) == bt::status_i::FAILURE)
            {
                destroy_node(op);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }

            op->children.push_back(ctx->current);
            ctx->current = op;
        }

        return bt::status_i::SUCCESS;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_unary(parse_context *ctx)
    {
        const auto pos_backup = ctx->state.pos;

        if (match_token(ctx, lang::token_i::BANG) == bt::status_i::SUCCESS ||
            match_token(ctx, lang::token_i::MINUS) == bt::status_i::SUCCESS)
        {
            auto *op = create_node(ir_t::NODE_UNARY_OP);
            op->value.op_val = static_cast<uint8_t>(ctx->tokens->types[static_cast<std::vector<unsigned>::size_type>(
                ctx->state.pos - 1)]);

            if (parse_unary(ctx) == bt::status_i::FAILURE)
            {
                destroy_node(op);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }

            op->children.push_back(ctx->current);
            ctx->current = op;
            return bt::status_i::SUCCESS;
        }

        return parse_primary(ctx);
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_primary(parse_context *ctx)
    {
        const auto pos_backup = ctx->state.pos;

        if (match_token(ctx, lang::token_i::TRUE) == bt::status_i::SUCCESS ||
            match_token(ctx, lang::token_i::FALSE) == bt::status_i::SUCCESS)
        {
            auto *literal = create_node(ir_t::NODE_LITERAL);
            literal->value.bool_val = ctx->tokens->types[static_cast<std::vector<unsigned>::size_type>(
                                          ctx->state.pos - 1)] == lang::token_i::TRUE;
            ctx->current = literal;
            return bt::status_i::SUCCESS;
        }

        if (match_token(ctx, lang::token_i::NUM_LITERAL) == bt::status_i::SUCCESS)
        {
            const auto &start = ctx->tokens->starts[static_cast<std::vector<unsigned>::size_type>(ctx->state.pos - 1)];
            const auto &length = ctx->tokens->lengths[static_cast<std::vector<unsigned>::size_type>(
                ctx->state.pos - 1)];

            auto *literal = create_node(ir_t::NODE_LITERAL);

            // Convert string to numeric value
            std::string numStr(ctx->tokens->starts[start], static_cast<char>(length));
            if (length > 2 && numStr[static_cast<std::string::size_type>(0)] == '0')
            {
                if (numStr[static_cast<std::string::size_type>(1)] == 'x' || numStr[static_cast<std::string::size_type>(
                        1)] == 'X')
                {
                    literal->value.num_val = std::strtod(numStr.c_str(), nullptr);
                    ctx->current = literal;
                    return bt::status_i::SUCCESS;
                }
                if (numStr[static_cast<std::string::size_type>(1)] == 'b' || numStr[static_cast<std::string::size_type>(
                        1)] == 'B')
                {
                    // Binary number
                    literal->value.num_val = std::strtod(numStr.c_str() + 2, nullptr);
                    ctx->current = literal;
                    return bt::status_i::SUCCESS;
                }
            }

            // Regular number
            literal->value.num_val = std::strtod(numStr.c_str(), nullptr);
            ctx->current = literal;
            return bt::status_i::SUCCESS;
        }

        if (match_token(ctx, lang::token_i::STR_LITERAL) == bt::status_i::SUCCESS)
        {
            ctx->tokens->starts[static_cast<std::vector<unsigned>::size_type>(ctx->state.pos - 1)];
            const auto &length = ctx->tokens->lengths[static_cast<std::vector<unsigned>::size_type>(
                ctx->state.pos - 1)];

            auto *literal = create_node(ir_t::NODE_LITERAL);
            literal->value.str_val.text = new char[length];
            literal->value.str_val.length = length;
            const auto value = get_token_value(ctx->src, *ctx->tokens, ctx->state.pos - 1);
            memcpy(literal->value.str_val.text, value.data(), value.length());
            ctx->current = literal;
            return bt::status_i::SUCCESS;
        }

        if (match_token(ctx, lang::token_i::IDENTIFIER) == bt::status_i::SUCCESS)
        {
            ctx->tokens->starts[static_cast<std::vector<unsigned>::size_type>(ctx->state.pos - 1)];
            const auto &length = ctx->tokens->lengths[static_cast<std::vector<unsigned>::size_type>(
                ctx->state.pos - 1)];

            auto *identifier = create_node(ir_t::NODE_IDENTIFIER);
            identifier->value.str_val.text = new char[length];
            identifier->value.str_val.length = length;
            const auto value = get_token_value(ctx->src, *ctx->tokens, ctx->state.pos - 1);
            memcpy(identifier->value.str_val.text, value.data(), value.length());
            ctx->current = identifier;
            return bt::status_i::SUCCESS;
        }

        if (match_token(ctx, lang::token_i::LEFT_PAREN) == bt::status_i::SUCCESS)
        {
            if (parse_expression(ctx) == bt::status_i::SUCCESS &&
                match_token(ctx, lang::token_i::RIGHT_PAREN) == bt::status_i::SUCCESS)
            {
                return bt::status_i::SUCCESS;
            }
        }

        ctx->state.pos = pos_backup;
        return bt::status_i::FAILURE;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_variable(parse_context *ctx)
    {
        const auto pos_backup = ctx->state.pos;

        auto *var_node = create_node(ir_t::NODE_VARIABLE);
        ctx->current = var_node;

        // Parse variable name
        if (match_token(ctx, lang::token_i::IDENTIFIER) == bt::status_i::FAILURE)
        {
            destroy_node(var_node);
            ctx->state.pos = pos_backup;
            return bt::status_i::FAILURE;
        }

        // Store variable name
        ctx->tokens->starts[static_cast<std::vector<unsigned>::size_type>(ctx->state.pos - 1)];
        const auto &name_length = ctx->tokens->lengths[static_cast<std::vector<unsigned>::size_type>(
            ctx->state.pos - 1)];

        auto *name_node = create_node(ir_t::NODE_IDENTIFIER);
        name_node->value.str_val.text = new char[name_length];
        name_node->value.str_val.length = name_length;
        const auto value = get_token_value(ctx->src, *ctx->tokens, ctx->state.pos - 1);
        memcpy(name_node->value.str_val.text, value.data(), value.length());
        var_node->children.push_back(name_node);

        // Parse type annotation if present
        if (match_token(ctx, lang::token_i::COLON) == bt::status_i::SUCCESS)
        {
            if (parse_type(ctx) == bt::status_i::FAILURE)
            {
                destroy_node(var_node);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }
            var_node->children.push_back(ctx->current);
        }

        // Parse initializer if present
        if (match_token(ctx, lang::token_i::EQUAL) == bt::status_i::SUCCESS)
        {
            if (parse_expression(ctx) == bt::status_i::FAILURE)
            {
                destroy_node(var_node);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }
            var_node->children.push_back(ctx->current);
        }

        if (match_token(ctx, lang::token_i::SEMICOLON) == bt::status_i::FAILURE)
        {
            destroy_node(var_node);
            ctx->state.pos = pos_backup;
            return bt::status_i::FAILURE;
        }

        return bt::status_i::SUCCESS;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_for_loop(parse_context *ctx)
    {
        const auto pos_backup = ctx->state.pos;

        auto *for_node = create_node(ir_t::NODE_LOOP);
        ctx->current = for_node;

        if (match_token(ctx, lang::token_i::LEFT_PAREN) == bt::status_i::FAILURE)
        {
            destroy_node(for_node);
            ctx->state.pos = pos_backup;
            return bt::status_i::FAILURE;
        }

        // Initializer
        if (match_token(ctx, lang::token_i::SEMICOLON) == bt::status_i::FAILURE)
        {
            if (parse_variable(ctx) == bt::status_i::FAILURE)
            {
                destroy_node(for_node);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }
            for_node->children.push_back(ctx->current);
        }

        // Condition
        if (match_token(ctx, lang::token_i::SEMICOLON) == bt::status_i::FAILURE)
        {
            if (parse_expression(ctx) == bt::status_i::FAILURE ||
                match_token(ctx, lang::token_i::SEMICOLON) == bt::status_i::FAILURE)
            {
                destroy_node(for_node);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }
            for_node->children.push_back(ctx->current);
        }

        // Increment
        if (match_token(ctx, lang::token_i::RIGHT_PAREN) == bt::status_i::FAILURE)
        {
            if (parse_expression(ctx) == bt::status_i::FAILURE ||
                match_token(ctx, lang::token_i::RIGHT_PAREN) == bt::status_i::FAILURE)
            {
                destroy_node(for_node);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }
            for_node->children.push_back(ctx->current);
        }

        // Body
        if (parse_statement(ctx) == bt::status_i::FAILURE)
        {
            destroy_node(for_node);
            ctx->state.pos = pos_backup;
            return bt::status_i::FAILURE;
        }
        for_node->children.push_back(ctx->current);

        return bt::status_i::SUCCESS;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_while_loop(parse_context *ctx)
    {
        const auto pos_backup = ctx->state.pos;

        auto *while_node = create_node(ir_t::NODE_LOOP);
        ctx->current = while_node;

        if (match_token(ctx, lang::token_i::LEFT_PAREN) == bt::status_i::FAILURE)
        {
            destroy_node(while_node);
            ctx->state.pos = pos_backup;
            return bt::status_i::FAILURE;
        }

        // Parse condition
        if (parse_expression(ctx) == bt::status_i::FAILURE)
        {
            destroy_node(while_node);
            ctx->state.pos = pos_backup;
            return bt::status_i::FAILURE;
        }
        while_node->children.push_back(ctx->current);

        if (match_token(ctx, lang::token_i::RIGHT_PAREN) == bt::status_i::FAILURE)
        {
            destroy_node(while_node);
            ctx->state.pos = pos_backup;
            return bt::status_i::FAILURE;
        }

        // Parse body
        if (parse_statement(ctx) == bt::status_i::FAILURE)
        {
            destroy_node(while_node);
            ctx->state.pos = pos_backup;
            return bt::status_i::FAILURE;
        }
        while_node->children.push_back(ctx->current);

        return bt::status_i::SUCCESS;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_return_statement(parse_context *ctx)
    {
        const auto pos_backup = ctx->state.pos;

        auto *return_node = create_node(ir_t::NODE_RETURN);
        ctx->current = return_node;

        // Parse return value if present
        if (match_token(ctx, lang::token_i::SEMICOLON) == bt::status_i::FAILURE)
        {
            if (parse_expression(ctx) == bt::status_i::FAILURE)
            {
                destroy_node(return_node);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }
            return_node->children.push_back(ctx->current);

            if (match_token(ctx, lang::token_i::SEMICOLON) == bt::status_i::FAILURE)
            {
                destroy_node(return_node);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }
        }

        return bt::status_i::SUCCESS;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_type(parse_context *ctx)
    {
        const auto pos_backup = ctx->state.pos;

        auto *type_node = create_node(ir_t::NODE_TYPE);
        ctx->current = type_node;

        // Parse basic type or identifier
        auto is_basic_type = false;
        static constexpr lang::token_i basic_types[] = {
            lang::token_i::U8, lang::token_i::I8,
            lang::token_i::U16, lang::token_i::I16,
            lang::token_i::U32, lang::token_i::I32,
            lang::token_i::U64, lang::token_i::I64,
            lang::token_i::F32, lang::token_i::F64,
            lang::token_i::STRING, lang::token_i::BOOLEAN,
            lang::token_i::VOID, lang::token_i::AUTO
        };

        for (const auto &basic_type: basic_types)
        {
            if (match_token(ctx, basic_type) == bt::status_i::SUCCESS)
            {
                is_basic_type = true;
                auto *basic_type_node = create_node(ir_t::NODE_IDENTIFIER);
                basic_type_node->value.str_val.text = new char[1];
                basic_type_node->value.str_val.length = 1;
                basic_type_node->value.str_val.text[0] = static_cast<char>(basic_type);
                type_node->children.push_back(basic_type_node);
                break;
            }
        }

        if (!is_basic_type)
        {
            if (match_token(ctx, lang::token_i::IDENTIFIER) == bt::status_i::FAILURE)
            {
                destroy_node(type_node);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }

            ctx->tokens->starts[static_cast<std::vector<unsigned>::size_type>(
                ctx->state.pos - 1)];
            const auto &name_length = ctx->tokens->lengths[static_cast<std::vector<unsigned>::size_type>(
                ctx->state.pos - 1)];

            auto *name_node = create_node(ir_t::NODE_IDENTIFIER);
            name_node->value.str_val.text = new char[name_length];
            name_node->value.str_val.length = name_length;
            const auto value = get_token_value(ctx->src, *ctx->tokens, ctx->state.pos - 1);
            memcpy(name_node->value.str_val.text, value.data(), value.length());
            type_node->children.push_back(name_node);
        }

        // Parse generic parameters if present
        if (match_token(ctx, lang::token_i::LESS) == bt::status_i::SUCCESS)
        {
            if (parse_generic_params(ctx) == bt::status_i::FAILURE ||
                match_token(ctx, lang::token_i::GREATER) == bt::status_i::FAILURE)
            {
                destroy_node(type_node);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }
            type_node->children.push_back(ctx->current);
        }

        return bt::status_i::SUCCESS;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_generic_params(parse_context *ctx)
    {
        const auto pos_backup = ctx->state.pos;

        auto *generic_list = create_node(ir_t::NODE_TYPE);
        ctx->current = generic_list;

        do
        {
            if (parse_type(ctx) == bt::status_i::FAILURE)
            {
                destroy_node(generic_list);
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }
            generic_list->children.push_back(ctx->current);
        }
        while (match_token(ctx, lang::token_i::COMMA) == bt::status_i::SUCCESS);

        return bt::status_i::SUCCESS;
    }

    ALWAYS_INLINE HOT_FUNCTION
    bt::status_i parse_class_header(parse_context *ctx)
    {
        const auto pos_backup = ctx->state.pos;

        // Parse class keyword is handled by parse_class()
        if (match_token(ctx, lang::token_i::IDENTIFIER) == bt::status_i::FAILURE)
        {
            ctx->state.pos = pos_backup;
            return bt::status_i::FAILURE;
        }

        ctx->tokens->starts[static_cast<std::vector<unsigned>::size_type>(ctx->state.pos - 1)];
        const auto &name_length = ctx->tokens->lengths[static_cast<std::vector<unsigned>::size_type>(
            ctx->state.pos - 1)];

        auto *name_node = create_node(ir_t::NODE_IDENTIFIER);
        name_node->value.str_val.text = new char[name_length];
        name_node->value.str_val.length = name_length;
        const auto value = get_token_value(ctx->src, *ctx->tokens, ctx->state.pos - 1);
        memcpy(name_node->value.str_val.text, value.data(), value.length());
        ctx->current->children.push_back(name_node);

        // Parse generic parameters if present
        if (match_token(ctx, lang::token_i::LESS) == bt::status_i::SUCCESS)
        {
            if (parse_generic_params(ctx) == bt::status_i::FAILURE ||
                match_token(ctx, lang::token_i::GREATER) == bt::status_i::FAILURE)
            {
                ctx->state.pos = pos_backup;
                return bt::status_i::FAILURE;
            }
            ctx->current->children.push_back(ctx->current);
        }

        return bt::status_i::SUCCESS;
    }
}