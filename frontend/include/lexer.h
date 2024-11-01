#ifndef YU_LEXER_H
#define YU_LEXER_H

#include <cstdint>
#include <vector>
#include <__fwd/string_view.h>
#include "../../yu/include/tokens.h"
#include "../../common/arch.hpp"

namespace yu::frontend
{
    /**
     * @brief The lexer class.
     */
    struct alignas(8) Lexer
    {
        const char* src{};
        lang::TokenList tokens;
        uint32_t current_pos{};
        uint32_t src_length{};
        std::vector<uint32_t> line_starts;
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
     * @brief Peek the next token without advancing.
     * @param lexer The lexer object.
     * @return token_t The next token.
     */
     ALWAYS_INLINE HOT_FUNCTION lang::token_t peek_next(const Lexer& lexer);

    /**
     * @brief Advances the lexer to the next token.
     * @param lexer The lexer object.
     */
     ALWAYS_INLINE HOT_FUNCTION void advance(Lexer& lexer);

    /**
     * @brief Returns the next token.
     * @param lexer The lexer object.
     * @return token_t The next token.
     */
     ALWAYS_INLINE HOT_FUNCTION lang::token_t next_token(Lexer& lexer);

    /**
     * @brief Tokenizes the source code and returns the token type.
     * @param lexer The lexer object.
     * @return token_i The token type.
     */
     lang::TokenList* tokenize(Lexer& lexer);

    /**
     * @brief Skips whitespace and comments using SIMD instructions.
     * @param lexer The lexer object.
     * @param src The source code.
     * @param current_pos The current position in the source code.
     * @param src_length The length of the source code.
     */
     ALWAYS_INLINE HOT_FUNCTION void skip_whitespace_comment(Lexer& lexer, const char *src, uint32_t &current_pos, uint32_t src_length);

    /**
     * @brief Get the line and column for a given token.
     * @param lexer The lexer object.
     * @param token The token.
     * @return std::pair<uint32_t, uint32_t> The line and column.
     */
     ALWAYS_INLINE HOT_FUNCTION std::pair<uint32_t, uint32_t> get_line_col(const Lexer& lexer, const lang::token_t& token);

    /**
     * @brief Get the string value of a token.
     * @param lexer The lexer object.
     * @param token The token.
     * @return std::string_view The string value of the token.
     */
     ALWAYS_INLINE HOT_FUNCTION std::string_view get_token_value(const Lexer& lexer, const lang::token_t& token);

    /**
     * @brief Checks if the token is a keyword or an identifier or a type.
     * @param lexer The lexer object.
     * @return token_t The token type.
     */
     ALWAYS_INLINE HOT_FUNCTION lang::token_t lex_identifier(const Lexer& lexer);

    /**
     * @brief Checks if the token is a number literal. Allows for floating point numbers, hexadecimals, and binary numbers.
     * @param lexer The lexer object.
     * @return token_t The token type.
     */
     ALWAYS_INLINE HOT_FUNCTION lang::token_t lex_number(const Lexer& lexer);

     /**
      * @brief Checks if the token is a string literal. Allows for escape sequences.
      * @param lexer The lexer object.
      * @return token_t The token type.
      */
      ALWAYS_INLINE HOT_FUNCTION lang::token_t lex_string(const Lexer& lexer);

      /**
       * @brief Checks if the token is a keyword or an identifier or a type.
       * @param c The character to check.
       * @return token_i The token type.
       */
       ALWAYS_INLINE HOT_FUNCTION lang::token_i get_token_type(char c);
}

#endif
