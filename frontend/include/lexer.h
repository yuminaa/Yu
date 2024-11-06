// This file is part of the Yu programming language and is licensed under MIT License;
// See LICENSE.txt for details

#ifndef YU_LEXER_H
#define YU_LEXER_H

#include <cstdint>
#include <string_view>
#include <vector>
#include "../../common/arch.hpp"
#include "../../yu/include/tokens.h"

namespace yu::frontend
{
    enum class generic_i : uint8_t
    {
        NONE, // Not in template context
        IDENTIFIER, // Just saw an identifier, might start template
        TEMPLATE, // Inside template parameters
        DONE // Finished template, back to normal
    };

    /**
     * @brief The lexer class.
     */
    struct alignas(64) Lexer
    {
        // Hot group - 16 bytes
        const char *src{}; // 8 bytes
        uint32_t current_pos{}; // 4 bytes
        uint32_t src_length{}; // 4 bytes

        // Cold group - separate cache line
        lang::TokenList tokens; // vector - 24 bytes
        std::vector<uint32_t> line_starts; // vector - 24 bytes

        ALWAYS_INLINE HOT_FUNCTION void prefetch_next() const;
    };

    /**
     * @brief Creates a lexer object.
     * @param src The source code to tokenize.
     * @return Lexer The lexer object.
     * @throws std::runtime_error if the source file is too large (>4GiB).
     */
    Lexer create_lexer(std::basic_string_view<char> src);

    /**
     * @brief Returns the next token.
     * @param lexer The lexer object.
     * @return token_t The next token.
     */
    ALWAYS_INLINE HOT_FUNCTION lang::token_t next_token(Lexer &lexer);

    /**
     * @brief Tokenizes the source code and returns the token type.
     * @param lexer The lexer object.
     * @return token_i The token type.
     */
    lang::TokenList *tokenize(Lexer &lexer);

    /**
     * @brief Skips whitespace and comments using SIMD instructions.
     * @param lexer The lexer object.
     * @param src The source code.
     * @param current_pos The current position in the source code.
     * @param src_length The length of the source code.
     */
    ALWAYS_INLINE HOT_FUNCTION void skip_whitespace_comment(Lexer &lexer, const char *src, uint32_t &current_pos,
                                                            uint32_t src_length);

    /**
     * @brief Get the line and column for a given token.
     * @param lexer The lexer object.
     * @param token The token.
     * @return std::pair<uint32_t, uint32_t> The line and column.
     */
    HOT_FUNCTION std::pair<uint32_t, uint32_t> get_line_col(const Lexer &lexer, const lang::token_t &token);

    /**
     * @brief Get the string value of a token.
     * @param lexer The lexer object.
     * @param token The token.
     * @return std::string_view The string value of the token.
     */
    HOT_FUNCTION std::string_view get_token_value(const Lexer &lexer, const lang::token_t &token);
    HOT_FUNCTION std::string_view get_token_value(const char* src, const lang::TokenList& tokens, size_t pos);

    /**
     * @brief Checks if the token is a keyword or an identifier or a type.
     * @param lexer The lexer object.
     * @return token_t The token type.
     */
    ALWAYS_INLINE HOT_FUNCTION lang::token_t lex_identifier(const Lexer &lexer);

    /**
     * @brief Checks if the token is a number literal. Allows for floating point numbers, hexadecimals, and binary numbers.
     * @param lexer The lexer object.
     * @return token_t The token type.
     */
    ALWAYS_INLINE HOT_FUNCTION lang::token_t lex_number(const Lexer &lexer);

    /**
     * @brief Checks if the token is a string literal. Allows for escape sequences.
     * @param lexer The lexer object.
     * @return token_t The token type.
     */
    ALWAYS_INLINE HOT_FUNCTION lang::token_t lex_string(const Lexer &lexer);

    /**
     * @brief Checks if the token is a keyword or an identifier or a type.
     * @param c The character to check.
     * @return token_i The token type.
     */
    HOT_FUNCTION lang::token_i get_token_type(char c);
}

#endif
