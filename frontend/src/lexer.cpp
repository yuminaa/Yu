#include <cstdint>
#include "../include/lexer.h"

namespace yu::frontend
{
    const std::array<uint8_t, 256> char_type = []
    {
        std::array<uint8_t, 256> types{};
        for (auto i = 0; i < 256; ++i)
        {
            if (i == ' ' || i == '\t' || i == '\n' || i == '\r')
                types[i] = 1;  // whitespace
            else if (i == '/')
                types[i] = 2;  // potential comment start
            else if (i == '*')
                types[i] = 3;  // potential comment end
            else if (std::isalpha(i) || i == '_' || i == '@')
                types[i] = 4;  // identifier start
            else if (std::isdigit(i))
                types[i] = 5;  // number start
            else if (i == '"')
                types[i] = 6;  // string start
            else
                types[i] = 0;  // other
        }
        return types;
    }();

    void Lexer::prefetch_next() const
    {
        PREFETCH_L1(src + current_pos + CACHE_LINE_SIZE);
        PREFETCH_L2(src + current_pos + CACHE_LINE_SIZE * 4);
        PREFETCH_L3(src + current_pos + CACHE_LINE_SIZE * 8);
    }

    Lexer create_lexer(std::string_view src)
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

    lang::token_t next_token(Lexer& lexer)
    {
        skip_whitespace_comment(lexer, lexer.src, lexer.current_pos, lexer.src_length);
        if (UNLIKELY(lexer.current_pos >= lexer.src_length))
            return {lexer.current_pos, 0, lang::token_i::END_OF_FILE, 0};

        const char* current = lexer.src + lexer.current_pos;
        const uint8_t type = char_type[static_cast<uint8_t>(*current)];

        switch (type)
        {
            case 4: // identifier
                return lex_identifier(lexer);
            case 5: // number
                return lex_number(lexer);
            case 6: // string
                return lex_string(lexer);
            default:
            {
                if (lexer.current_pos + 2 < lexer.src_length)
                {
                    std::string_view three_char(current, 3);
                    for (const auto& [token_text, token_type] : lang::token_map)
                    {
                        if (token_text == three_char)
                            return {lexer.current_pos, 3, token_type, 0};
                    }
                }

                if (lexer.current_pos + 1 < lexer.src_length)
                {
                    std::string_view two_char(current, 2);
                    for (const auto& [token_text, token_type] : lang::token_map)
                    {
                        if (token_text == two_char)
                            return {lexer.current_pos, 2, token_type, 0};
                    }
                }

                std::string_view one_char(current, 1);
                for (const auto& [token_text, token_type] : lang::token_map)
                {
                    if (token_text == one_char)
                        return {lexer.current_pos, 1, token_type, 0};
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

    void skip_whitespace_comment(Lexer& lexer, const char* src, uint32_t& current_pos, uint32_t src_length)
    {
        while (current_pos < src_length)
        {
            const uint8_t type = char_type[static_cast<uint8_t>(src[current_pos])];

            if (type == 0)
                return;

            if (type == 1)
            {
                if (src[current_pos] == '\n')
                    lexer.line_starts.push_back(current_pos + 1);
                ++current_pos;
                continue;
            }

            // Handle comments
            if (type == 2 && current_pos + 1 < src_length)  // Potential comment start
            {
                if (src[current_pos + 1] == '/')  // Single-line comment
                {
                    current_pos += 2;
                    while (current_pos < src_length && src[current_pos] != '\n')
                        ++current_pos;
                    continue;
                }

                if (src[current_pos + 1] == '*')  // Multi-line comment
                {
                    current_pos += 2;
                    while (current_pos + 1 < src_length)
                    {
                        if (src[current_pos] == '*' && src[current_pos + 1] == '/')
                        {
                            current_pos += 2;
                            break;
                        }
                        if (src[current_pos] == '\n')
                            lexer.line_starts.push_back(current_pos + 1);
                        ++current_pos;
                    }
                    continue;
                }
            }

            return;  // Not whitespace or comment
        }
    }

    lang::token_t lex_number(const Lexer& lexer)
    {
        const char* start = lexer.src + lexer.current_pos;
        const char* current = start;

        // Check for hex/binary prefix
        if (*current == '0' && current + 1 < lexer.src + lexer.src_length)
        {
            const char next = *(current + 1);
            if (next == 'x' || next == 'X')
            {
                current += 2;
                while (current < lexer.src + lexer.src_length && std::isxdigit(*current))
                    ++current;
                goto end_number;
            }
            if (next == 'b' || next == 'B')
            {
                current += 2;
                while (current < lexer.src + lexer.src_length && (*current == '0' || *current == '1'))
                    ++current;
                goto end_number;
            }
        }

        // Parse decimal or floating point
        while (current < lexer.src + lexer.src_length)
        {
            char c = *current;
            if (c == '.')
            {
                if (*(current - 1) >= '0' && *(current - 1) <= '9')  // Ensure decimal follows a digit
                    ++current;
                else
                    break;
            }
            else if (c == 'e' || c == 'E')
            {
                ++current;
                if (current < lexer.src + lexer.src_length &&
                    (*current == '+' || *current == '-'))
                    ++current;
            }
            else if (c < '0' || c > '9')
                break;
            else
                ++current;
        }

        end_number:
            return {lexer.current_pos, static_cast<uint16_t>(current - start),
                    lang::token_i::NUM_LITERAL, 0};
    }

    lang::token_t lex_string(const Lexer& lexer)
    {
        const char* start = lexer.src + lexer.current_pos;
        const char* current = start + 1;

        while (current < lexer.src + lexer.src_length)
        {
            if (*current == '"' && *(current - 1) != '\\')
            {
                ++current;
                break;
            }
            if (*current == '\\' && current + 1 < lexer.src + lexer.src_length)
                current += 2;
            else
                ++current;
        }

        return {lexer.current_pos,
                static_cast<uint16_t>(current - start),
                lang::token_i::STR_LITERAL,
                static_cast<uint8_t>(current >= lexer.src + lexer.src_length ? 1u : 0u)};
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
        std::string_view text(start, length);

        for (const auto& [token_text, token_type] : lang::token_map)
        {
            if (token_text.length() == length &&
                memcmp(token_text.data(), text.data(), length) == 0)
                return {lexer.current_pos, length, token_type, 0};
        }

        return {lexer.current_pos, length, lang::token_i::IDENTIFIER, 0};
    }

    // Utility functions unchanged
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
