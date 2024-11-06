// This file is part of the Yu programming language and is licensed under MIT License;
// See LICENSE.txt for details

#include "../include/lexer.h"
#include <array>
#include <cstdint>

namespace yu::frontend 
{

    /** Char type lookup using branchless multiplication for maximum throughput
     *  Each char maps to a type: whitespace(1), comment(/=2), etc.
     *  Using multiplication instead of branches improves pipelining
    */
    static const std::array<uint8_t, 256> char_type = []
    {
        std::array<uint8_t, 256> types{};
        for (auto i = 0; i < 256; ++i)
        {
            types[i] = ((i == ' ' || i == '\t' || i == '\n' || i == '\r')) * 1 +  
                       (i == '/') * 2 +                                            
                       (i == '*') * 3 +                                            
                       ((std::isalpha(i) || i == '_' || i == '@')) * 4 +          
                       (std::isdigit(i)) * 5 +                                     
                       (i == '"') * 6;                                             
        }
        return types;
    }();

    static const std::array<lang::token_i, 256> single_char_tokens = []
    {
        std::array<lang::token_i, 256> tokens{};
        std::fill(tokens.begin(), tokens.end(), lang::token_i::UNKNOWN);
        
        tokens['+'] = lang::token_i::PLUS;
        tokens['-'] = lang::token_i::MINUS;
        tokens['*'] = lang::token_i::STAR;
        tokens['/'] = lang::token_i::SLASH;
        tokens['%'] = lang::token_i::PERCENT;
        tokens['='] = lang::token_i::EQUAL;
        tokens['!'] = lang::token_i::BANG;
        tokens['<'] = lang::token_i::LESS;
        tokens['>'] = lang::token_i::GREATER;
        tokens['&'] = lang::token_i::AND;
        tokens['|'] = lang::token_i::OR;
        tokens['^'] = lang::token_i::XOR;
        tokens['~'] = lang::token_i::TILDE;
        tokens['.'] = lang::token_i::DOT;
        tokens['('] = lang::token_i::LEFT_PAREN;
        tokens[')'] = lang::token_i::RIGHT_PAREN;
        tokens['{'] = lang::token_i::LEFT_BRACE;
        tokens['}'] = lang::token_i::RIGHT_BRACE;
        tokens['['] = lang::token_i::LEFT_BRACKET;
        tokens[']'] = lang::token_i::RIGHT_BRACKET;
        tokens[','] = lang::token_i::COMMA;
        tokens[':'] = lang::token_i::COLON;
        tokens[';'] = lang::token_i::SEMICOLON;
        tokens['?'] = lang::token_i::QUESTION;
        
        return tokens;
    }();

    static constexpr std::array<lang::token_i, 7> type_to_token = {
        lang::token_i::UNKNOWN,      // 0
        lang::token_i::UNKNOWN,      // type 1 (whitespace)
        lang::token_i::UNKNOWN,      // type 2 (comment start)
        lang::token_i::UNKNOWN,      // type 3 (comment end)
        lang::token_i::IDENTIFIER,   // type 4
        lang::token_i::NUM_LITERAL,  // type 5
        lang::token_i::STR_LITERAL   // type 6
    };

    static constexpr std::array<uint8_t, 256> hex_lookup = []
    {
        std::array<uint8_t, 256> table{};
        for (int i = '0'; i <= '9'; ++i) table[i] = 1;
        for (int i = 'a'; i <= 'f'; ++i) table[i] = 1;
        for (int i = 'A'; i <= 'F'; ++i) table[i] = 1;
        return table;
    }();

    static constexpr std::array<uint8_t, 256> bin_lookup = []
    {
        std::array<uint8_t, 256> table{};
        table['0'] = table['1'] = 1;
        return table;
    }();

    static constexpr std::array<uint8_t, 256> valid_escapes = []
    {
        std::array<uint8_t, 256> table{};
        table['n'] = table['t'] = table['r'] = table['\\'] = 
        table['"'] = table['0'] = table['x'] = 1;
        return table;
    }();

    ALWAYS_INLINE constexpr uint8_t make_flag(const bool condition, lang::token_flags flag)
    {
        return condition ? static_cast<uint8_t>(flag) : 0;
    }

    /** Prefetching strategy for optimal cache utilization
     *  L1: Next immediate chunk (64 bytes)
     *  L2: Near-future chunk (256 bytes)
     *  L3: Far-future chunk (512 bytes)
     *  This pattern matches typical token parsing flow where we need
     *  to look ahead for multi-character tokens and comments
    */
    ALWAYS_INLINE void Lexer::prefetch_next() const
    {
        PREFETCH_L1(src + current_pos + CACHE_LINE_SIZE);
        PREFETCH_L2(src + current_pos + CACHE_LINE_SIZE * 4);
        PREFETCH_L3(src + current_pos + CACHE_LINE_SIZE * 8);
    }

    Lexer create_lexer(const std::string_view src)
    {
        Lexer lexer;
        lexer.src = src.data();
        lexer.src_length = static_cast<uint32_t>(src.length());
        lexer.current_pos = 0;
        lexer.tokens.reserve(src.length() / 4);
        lexer.line_starts.reserve(src.length() / 40);
        lexer.line_starts.emplace_back(0);
        return lexer;
    }

    ALWAYS_INLINE HOT_FUNCTION
    void skip_whitespace_comment(Lexer& lexer, const char* src,
                                                      uint32_t& current_pos, const uint32_t src_length)
    {
        while (current_pos < src_length)
        {
            const char current_char = src[current_pos];
            const uint8_t type = char_type[static_cast<uint8_t>(current_char)];

            if (current_char == '\n')
                lexer.line_starts.emplace_back(current_pos + 1);

            const uint32_t has_next = current_pos + 1 < src_length;
            const char next_char = src[current_pos + has_next];

            const uint32_t is_slash = (type == 2);
            const uint32_t is_single = (next_char == '/');
            const uint32_t is_multi = (next_char == '*');

            if (is_slash & is_single)
            {
                current_pos += 2;
                while (current_pos < src_length && src[current_pos] != '\n')
                    ++current_pos;
                continue;
            }

            if (is_slash & is_multi)
            {
                current_pos += 2;
                while (current_pos + 1 < src_length)
                {
                    const uint32_t is_end = (src[current_pos] == '*') &
                                          (src[current_pos + 1] == '/');
                    if (src[current_pos] == '\n')
                        lexer.line_starts.emplace_back(current_pos + 1);

                    current_pos += 1 + is_end;
                    if (is_end)
                        break;
                }
                continue;
            }

            const uint32_t should_continue = (type == 1);
            current_pos += should_continue;
            if (!should_continue)
                return;
        }
    }

    ALWAYS_INLINE HOT_FUNCTION lang::token_t next_token(Lexer& lexer)
    {
        skip_whitespace_comment(lexer, lexer.src, lexer.current_pos, lexer.src_length);
        
        if (UNLIKELY(lexer.current_pos >= lexer.src_length))
            return {lexer.current_pos, 0, lang::token_i::END_OF_FILE, 0};

        switch (const char c = lexer.src[lexer.current_pos]; char_type[static_cast<uint8_t>(c)])
        {
            case 4:
                return lex_identifier(lexer);
            case 5:
                return lex_number(lexer);
            case 6:
                return lex_string(lexer);
            default:
                return {lexer.current_pos, 1, single_char_tokens[static_cast<uint8_t>(c)], 0};
        }
    }

    ALWAYS_INLINE HOT_FUNCTION lang::token_t lex_number(const Lexer& lexer)
    {
        const char* start = lexer.src + lexer.current_pos;
        const char* current = start;
        const char* end = lexer.src + lexer.src_length;
        uint8_t flags = 0;

        const uint32_t has_next = current + 1 < end;
        const char next = current[has_next];
        const uint32_t is_zero = (*current == '0');
        const uint32_t is_x = (next == 'x' || next == 'X');
        const uint32_t is_b = (next == 'b' || next == 'B');
        const uint32_t is_hex = is_zero & is_x;
        const uint32_t is_bin = is_zero & is_b;

        current += 2 * (is_hex | is_bin);

        uint32_t has_decimal = 0;
        while (current < end)
        {
            const char c = *current;
            const uint32_t is_digit = std::isdigit(c);
            const uint32_t is_dot = (c == '.');
            const uint32_t is_valid_hex = hex_lookup[static_cast<uint8_t>(c)];
            const uint32_t is_valid_bin = bin_lookup[static_cast<uint8_t>(c)];

            const uint32_t is_valid = (is_hex & is_valid_hex) |
                                    (is_bin & is_valid_bin) |
                                    (!is_hex & !is_bin & (is_digit | is_dot));

            if (!is_valid)
                break;

            has_decimal |= is_dot;
            flags |= make_flag(has_decimal > 1, lang::token_flags::MULTIPLE_DECIMAL_POINTS);

            ++current;
        }

        if (current < end && (*current == 'e' || *current == 'E'))
        {
            ++current;
            if (current < end && (*current == '+' || *current == '-'))
                ++current;

            if (current >= end || !std::isdigit(*current))
            {
                flags |= make_flag(true, lang::token_flags::INVALID_EXPONENT);
                return {lexer.current_pos,
                        static_cast<uint16_t>(current - start),
                        lang::token_i::NUM_LITERAL,
                        flags};
            }

            while (current < end && std::isdigit(*current))
                ++current;
        }

        return {lexer.current_pos,
                static_cast<uint16_t>(current - start),
                lang::token_i::NUM_LITERAL,
                flags};
    }

    ALWAYS_INLINE HOT_FUNCTION lang::token_t lex_string(const Lexer& lexer)
    {
        const char* start = lexer.src + lexer.current_pos;
        const char* current = start + 1;
        const char* end = lexer.src + lexer.src_length;
        uint8_t flags = 0;

        while (current < end)
        {
            const char c = *current;
            const uint32_t is_escape = (c == '\\');
            const uint32_t has_next = current + 1 < end;
            const char next = current[has_next];

            if (is_escape)
            {
                if (!valid_escapes[static_cast<uint8_t>(next)])
                {
                    flags |= make_flag(true, lang::token_flags::INVALID_ESCAPE_SEQUENCE);
                    break;
                }

                if (next == 'x')
                {
                    if (current + 3 >= end ||
                        !std::isxdigit(*(current + 2)) ||
                        !std::isxdigit(*(current + 3)))
                    {
                        flags |= make_flag(true, lang::token_flags::INVALID_ESCAPE_SEQUENCE);
                        break;
                    }
                    current += 4;  // Skip \xHH
                    continue;
                }

                current += 2;  // Skip basic escape sequence
                continue;
            }

            if (c == '"')
            {
                ++current;
                break;
            }

            if (c == '\n' || c == '\r')
            {
                flags |= make_flag(true, lang::token_flags::UNTERMINATED_STRING);
                break;
            }

            ++current;
        }

        flags |= make_flag(current >= end || *(current - 1) != '"',
                          lang::token_flags::UNTERMINATED_STRING);

        return {lexer.current_pos,
                static_cast<uint16_t>(current - start),
                lang::token_i::STR_LITERAL,
                flags};
    }

    ALWAYS_INLINE HOT_FUNCTION lang::token_t lex_identifier(const Lexer& lexer)
    {
        const char* start = lexer.src + lexer.current_pos;
        const char* current = start;
        uint8_t flags = 0;

        const uint32_t is_valid_start = (*current == '_' || *current == '@' ||
                                       std::isalpha(*current));
        flags |= make_flag(!is_valid_start, lang::token_flags::INVALID_IDENTIFIER_START);

        current += (*current == '@');

        while (current < lexer.src + lexer.src_length)
        {
            const char c = *current;
            if (!std::isalnum(c) && c != '_') {
                if (!std::isspace(c) && !std::ispunct(c)) {
                    flags |= make_flag(true, lang::token_flags::INVALID_IDENTIFIER_CHAR);
                }
                break;
            }
            ++current;
        }

        const uint16_t length = current - start;
        const std::string_view text(start, length);

        for (const auto& [token_text, token_type] : lang::token_map)
        {
            if (token_text.length() == length &&
                memcmp(token_text.data(), text.data(), length) == 0)
            {
                return {lexer.current_pos, length, token_type, flags};
            }
        }

        return {lexer.current_pos, length, lang::token_i::IDENTIFIER, flags};
    }

    lang::TokenList* tokenize(Lexer& lexer)
    {
        while (true)
        {
            lang::token_t token = next_token(lexer);
            lexer.tokens.push_back(token);

            if (token.type == lang::token_i::END_OF_FILE)
                break;

            lexer.current_pos += token.length;
            lexer.prefetch_next();  // Prefetch next chunk while processing current token
        }

        return &lexer.tokens;
    }

    HOT_FUNCTION std::pair<uint32_t, uint32_t> get_line_col(const Lexer& lexer, const lang::token_t& token)
    {
        const auto it = std::upper_bound(lexer.line_starts.begin(), lexer.line_starts.end(), token.start);
        return {std::distance(lexer.line_starts.begin(), it), token.start - *(it - 1) + 1};
    }

    HOT_FUNCTION lang::token_i get_token_type(const char c)
    {
        const uint8_t char_class = char_type[static_cast<uint8_t>(c)];
        const lang::token_i single_token = single_char_tokens[static_cast<uint8_t>(c)];
        const lang::token_i type_token = type_to_token[char_class];

        return (single_token != lang::token_i::UNKNOWN) ? single_token : type_token;
    }

    std::string_view get_token_value(const Lexer& lexer, const lang::token_t& token)
    {
        return {lexer.src + token.start, token.length};
    }

    std::string_view get_token_value(const char* src, const lang::TokenList& tokens, size_t pos)
    {
        return {src + tokens.starts[static_cast<std::vector<unsigned>::size_type>(pos)], tokens.lengths[static_cast<std::vector<unsigned>::size_type>(pos)]};
    }
}