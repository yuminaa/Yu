// This file is part of the Yu programming language and is licensed under MIT License;
// See LICENSE.txt for details

#include "../include/lexer.h"
#include <cstdint>

/**
 * @file lexer.cpp
 * @brief High-performance lexical analyzer implementation for the Yu language
 *
 * This lexer implements token recognition for identifiers, numbers (decimal/hex/binary),
 * strings, and comments.
*/

namespace yu::frontend
{
    /**
     * @brief Character classification lookup table using branchless arithmetic
     * @description Multiplies boolean conditions (0/1) by type values and sums results
     * @note This avoids multiple branches for each character classification
     */
    const std::array<uint8_t, 256> char_type = []
    {
        std::array<uint8_t, 256> types{};
        for (auto i = 0; i < 256; ++i)
        {
            types[i] = (i == ' ' || i == '\t' || i == '\n' || i == '\r') * 1 +    // whitespace
                      (i == '/') * 2 +                                             // potential comment start
                      (i == '*') * 3 +                                             // potential comment end
                      ((std::isalpha(i) || i == '_' || i == '@')) * 4 +           // identifier start
                      (std::isdigit(i)) * 5 +                                      // number start
                      (i == '"') * 6;                                              // string start
        }
        return types;
    }();

    /**
     * @brief Prefetches next portions of source code into CPU cache
     *
     * Optimizes reading speed by prefetching upcoming source code into
     * different cache levels with increasing look-ahead distances
     */
    void Lexer::prefetch_next() const
    {
        PREFETCH_L1(src + current_pos + CACHE_LINE_SIZE);
        PREFETCH_L2(src + current_pos + CACHE_LINE_SIZE * 4);
        PREFETCH_L3(src + current_pos + CACHE_LINE_SIZE * 8);
    }

    Lexer create_lexer(const std::string_view src)
    {
        if (UNLIKELY(src.length() > UINT32_MAX))
            throw std::runtime_error("Source file too large (>4GiB)");

        Lexer lexer;
        lexer.src = src.data();
        lexer.src_length = static_cast<uint32_t>(src.length());
        lexer.current_pos = 0;
        lexer.tokens.reserve(src.length() / 4);
        lexer.line_starts.reserve(src.length() / 40);
        lexer.line_starts.emplace_back(0);

        return lexer;
    }

    lang::token_t peek_next(const Lexer& lexer)
    {
        if (UNLIKELY(lexer.current_pos >= lexer.src_length))
            return {lexer.current_pos, 0, lang::token_i::END_OF_FILE, 0};

        return next_token(const_cast<Lexer&>(lexer));
    }

    void advance(Lexer& lexer)
    {
        if (LIKELY(lexer.current_pos < lexer.src_length))
            lexer.current_pos += next_token(lexer).length;
    }

    ALWAYS_INLINE HOT_FUNCTION lang::token_t next_token(Lexer& lexer)
    {
        skip_whitespace_comment(lexer, lexer.src, lexer.current_pos, lexer.src_length);
        if (UNLIKELY(lexer.current_pos >= lexer.src_length))
            return {lexer.current_pos, 0, lang::token_i::END_OF_FILE, 0};

        const char* current = lexer.src + lexer.current_pos;
        const uint32_t remaining = lexer.src_length - lexer.current_pos;

        const bool is_template_char = (*current == '<' || *current == '>' || *current == ',' ||
                                    *current == '|' || std::isspace(*current));

        const bool should_reset = !is_template_char &&
                                lexer.template_state.state != generic_i::NONE &&
                                lexer.template_state.angle_depth == 0;

        if (should_reset)
        {
            lexer.template_state.angle_depth = 0;
            lexer.template_state.state = generic_i::NONE;
        }

        switch (char_type[static_cast<uint8_t>(*current)])
        {
            case 4: // identifier
            {
                const auto token = lex_identifier(lexer);
                if (lexer.template_state.state == generic_i::NONE)
                {
                    lexer.template_state.state = generic_i::IDENTIFIER;
                }
                return token;
            }
            case 5: // number
                return lex_number(lexer);
            case 6: // string
                return lex_string(lexer);
            default:
            {
                // Update template state
                const bool is_less = (*current == '<');
                const bool is_greater = (*current == '>');
                const bool in_identifier = (lexer.template_state.state == generic_i::IDENTIFIER);
                const bool in_template = (lexer.template_state.state == generic_i::TEMPLATE);

                if (is_less && (in_identifier || in_template))
                {
                    ++lexer.template_state.angle_depth;
                    lexer.template_state.state = generic_i::TEMPLATE;
                }
                else if (is_greater && in_template)
                {
                    --lexer.template_state.angle_depth;
                    if (lexer.template_state.angle_depth == 0)
                        lexer.template_state.state = generic_i::DONE;
                }

                const bool is_template_context = in_template ||
                    (in_identifier && is_less) ||
                    (lexer.template_state.angle_depth > 0);

                if (is_template_context && is_greater)
                {
                    return {lexer.current_pos, 1, lang::token_i::GREATER, 0};
                }

                uint16_t best_length = 0;
                auto best_type = lang::token_i::UNKNOWN;
                auto found = false;

                for (uint16_t len = std::min(remaining, 3u); len > 0 && !found; --len)
                {
                    const std::string_view token_text(current, len);
                    for (const auto& [text, type] : lang::token_map)
                    {
                        if (text.length() == len && text == token_text)
                        {
                            best_length = len;
                            best_type = type;
                            found = true;
                            break;
                        }
                    }
                }

                if (best_type != lang::token_i::UNKNOWN)
                {
                    return {lexer.current_pos, best_length, best_type, 0};
                }

                return {lexer.current_pos, 1, lang::token_i::UNKNOWN, 0};
            }
        }
    }

    lang::TokenList* tokenize(Lexer& lexer)
    {
        while (true)
        {
            lang::token_t token = next_token(lexer);
            if (token.type != lang::token_i::UNKNOWN)
                lexer.tokens.push_back(token);
            if (token.type == lang::token_i::END_OF_FILE)
                break;

            lexer.current_pos += token.length;
        }
        return &lexer.tokens;
    }

    /**
     * @brief Skips whitespace and processes comments while tracking line numbers
     *
     * Uses branchless arithmetic to track line numbers and detect comment types.
     * Handles both single-line (//) and multi-line (/* *\/) comments.
     */
    void skip_whitespace_comment(Lexer& lexer, const char* src, uint32_t& current_pos, const uint32_t src_length)
    {
        while (current_pos < src_length)
        {
            const char current_char = src[current_pos];
            const uint8_t type = char_type[static_cast<uint8_t>(current_char)];

            const bool is_newline = current_char == '\n';
            lexer.line_starts.emplace_back(current_pos + 1 * is_newline);

            const bool has_next = current_pos + 1 < src_length;
            const char next_char = has_next ? src[current_pos + 1] : static_cast<char>(0);

            const bool is_single_comment = (type == 2) & (next_char == '/');
            const bool is_multi_comment = (type == 2) & (next_char == '*');

            if (is_single_comment)
            {
                current_pos += 2;
                while (current_pos < src_length && src[current_pos] != '\n')
                    ++current_pos;
                continue;
            }

            if (is_multi_comment)
            {
                current_pos += 2;
                while (current_pos + 1 < src_length)
                {
                    const bool is_end = (src[current_pos] == '*') & (src[current_pos + 1] == '/');
                    const bool is_comment_newline = src[current_pos] == '\n';
                    lexer.line_starts.emplace_back(current_pos + 1 * is_comment_newline);

                    current_pos += 1 + is_end;
                    if (is_end)
                        break;
                }
                continue;
            }

            const bool should_continue = (type == 1);
            current_pos += should_continue;

            if (!should_continue)
                return;
        }
    }

    /**
     * @brief Processes numeric literals using branchless validation
     *
     * Handles decimal, hexadecimal, and binary number formats:
     * - Decimal: [0-9]+(\.[0-9]+)?([eE][+-]?[0-9]+)?
     * - Hex: 0[xX][0-9a-fA-F]+
     * - Binary: 0[bB][01]+
     */
    lang::token_t lex_number(const Lexer& lexer)
    {
        const char* start = lexer.src + lexer.current_pos;
        const char* current = start;

        const bool has_next = current + 1 < lexer.src + lexer.src_length;
        const char next = has_next ? *(current + 1) : static_cast<char>(0);
        const bool is_hex = (*current == '0') & ((next == 'x') | (next == 'X'));
        const bool is_bin = (*current == '0') & ((next == 'b') | (next == 'B'));

        current += (is_hex | is_bin) * 2;

        while (current < lexer.src + lexer.src_length)
        {
            const char c = *current;
            const bool is_valid_hex = std::isxdigit(c) & is_hex;
            const bool is_valid_bin = (c == '0' | c == '1') & is_bin;
            const bool is_valid_decimal = !is_hex & !is_bin &
                                        (c >= '0' & c <= '9' |
                                         (c == '.') & (*(current - 1) >= '0' & *(current - 1) <= '9') |
                                         (c == 'e' | c == 'E') &
                                         (*(current + 1) == '+' | *(current + 1) == '-'));

            const bool should_advance = is_valid_hex | is_valid_bin | is_valid_decimal;
            current += should_advance;

            if (!should_advance) break;
        }

        return {lexer.current_pos,
                static_cast<uint16_t>(current - start),
                lang::token_i::NUM_LITERAL,
                0};
    }

    lang::token_t lex_string(const Lexer& lexer)
    {
        const char* start = lexer.src + lexer.current_pos;
        const char* current = start + 1;

        while (current < lexer.src + lexer.src_length)
        {
            const bool is_escape = (*current == '\\') & (current + 1 < lexer.src + lexer.src_length);

            if ((*current == '"') & (*(current - 1) != '\\'))
            {
                ++current;
                break;
            }
            current += 1 + is_escape;
        }

        const bool is_unterminated = current >= lexer.src + lexer.src_length;
        return {lexer.current_pos,
                static_cast<uint16_t>(current - start),
                lang::token_i::STR_LITERAL,
                static_cast<uint8_t>(is_unterminated)};
    }

    lang::token_t lex_identifier(const Lexer& lexer)
    {
        const char* start = lexer.src + lexer.current_pos;
        const char* current = start;

        if (*current == '@')
            ++current;

        while (current < lexer.src + lexer.src_length &&
              (std::isalnum(*current) || *current == '_'))
            ++current;

        const uint16_t length = current - start;
        const std::string_view text(start, length);

        for (const auto& [token_text, token_type] : lang::token_map)
        {
            if (token_text.length() == length &&
                memcmp(token_text.data(), text.data(), length) == 0)
                return {lexer.current_pos, length, token_type, 0};
        }

        return {lexer.current_pos, length, lang::token_i::IDENTIFIER, 0};
    }

    /**
     * @brief Calculates line and column numbers for error reporting
     *
     * Uses binary search through line starts for O(log n) position lookup
     */
    std::pair<uint32_t, uint32_t> get_line_col(const Lexer& lexer, const lang::token_t& token)
    {
        const auto it = std::upper_bound(lexer.line_starts.begin(), lexer.line_starts.end(), token.start);
        return {std::distance(lexer.line_starts.begin(), it), token.start - *(it - 1) + 1};
    }

    std::string_view get_token_value(const Lexer& lexer, const lang::token_t& token)
    {
        return {lexer.src + token.start, token.length};
    }

    lang::token_i get_token_type(const char c)
    {
        return static_cast<lang::token_i>(char_type[static_cast<uint8_t>(c)]);
    }
}
