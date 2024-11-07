// This file is part of the Yu programming language and is licensed under MIT License;
// See LICENSE.txt for details

#ifndef YU_TOKEN_H
#define YU_TOKEN_H

#include <cstdint>
#include <vector>

namespace yu::lang
{
    /**
     * @brief Represents the type of token that the lexer has found.
     */
    enum class token_i : uint8_t
    {
        // Keywords
        TRUE,
        FALSE,
        NIL,
        IMPORT,
        VAR,
        CONST,
        FUNCTION,
        INLINE,
        RETURN,
        NEW,
        ENUM,
        IF,
        ELSE,
        FOR,
        WHILE,
        BREAK,
        CONTINUE,
        SWITCH,
        CASE,
        DEFAULT,
        CLASS,
        EXTENDS,
        FINAL,
        PUBLIC,
        PRIVATE,
        PROTECTED,
        STATIC,
        AWAIT,
        ASYNC,
        TRY,
        CATCH,
        FROM,
        TYPE,

        // Basic types
        U8,
        I8,
        U16,
        I16,
        U32,
        I32,
        U64,
        I64,
        F32,
        F64,
        STRING,
        BOOLEAN,
        VOID,
        AUTO,
        PTR,

        // Complex types
        NULLABLE_U8,
        NULLABLE_I8,
        NULLABLE_U16,
        NULLABLE_I16,
        NULLABLE_U32,
        NULLABLE_I32,
        NULLABLE_U64,
        NULLABLE_I64,
        NULLABLE_F32,
        NULLABLE_F64,
        NULLABLE_STRING,
        NULLABLE_BOOLEAN,

        // Single character operators
        PLUS,    // +
        MINUS,   // -
        STAR,    // *
        SLASH,   // /
        PERCENT, // %
        EQUAL,   // =
        BANG,    // !
        LESS,    // <
        GREATER, // >
        AND,     // &
        OR,      // |
        XOR,     // ^
        TILDE,   // ~
        DOT,     // .

        // Delimiters
        LEFT_PAREN,    // (
        RIGHT_PAREN,   // )
        LEFT_BRACE,    // {
        RIGHT_BRACE,   // }
        LEFT_BRACKET,  // [
        RIGHT_BRACKET, // ]
        COMMA,         // ,
        COLON,         // :
        SEMICOLON,     // ;
        QUESTION,      // ?

        // Annotations
        ALIGN_ANNOT,      // @align
        DEPRECATED_ANNOT, // @deprecated
        PACKED_ANNOT,     // @packed
        NO_DISCARD_ANNOT, // @nodiscard
        VOLATILE_ANNOT,   // @volatile
        LAZY_ANNOT,       // @lazy
        PURE_ANNOT,       // @pure
        TAIL_REC_ANNOT,   // @tailrec

        // Special tokens
        IDENTIFIER,
        NUM_LITERAL,
        STR_LITERAL,
        ANNOTATION,
        UNKNOWN,
        END_OF_FILE
    };

    /**
     * @brief Flags that can be associated with tokens to indicate errors on conditions.
    */
    enum class token_flags : uint8_t
    {
        NONE = 0,

        // String errors
        UNTERMINATED_STRING = 1 << 0,
        INVALID_ESCAPE_SEQUENCE = 1 << 1,

        // Number errors
        INVALID_DIGIT = 1 << 2,
        MULTIPLE_DECIMAL_POINTS = 1 << 3,
        INVALID_EXPONENT = 1 << 4,

        // Comment errors
        UNTERMINATED_BLOCK_COMMENT = 1 << 5,

        // Identifier errors
        INVALID_IDENTIFIER_START = 1 << 6,
        INVALID_IDENTIFIER_CHAR = 1 << 7
    };

    /**
     * @brief A map to quickly lookup tokens.
     */
    static constexpr std::pair<std::string_view, token_i> token_map[] = {
        // Keywords
        { "true", token_i::TRUE },
        { "false", token_i::FALSE },
        { "null", token_i::NIL },
        { "import", token_i::IMPORT },
        { "var", token_i::VAR },
        { "const", token_i::CONST },
        { "function", token_i::FUNCTION },
        { "inline", token_i::INLINE },
        { "return", token_i::RETURN },
        { "new", token_i::NEW },
        { "enum", token_i::ENUM },
        { "if", token_i::IF },
        { "else", token_i::ELSE },
        { "for", token_i::FOR },
        { "while", token_i::WHILE },
        { "break", token_i::BREAK },
        { "continue", token_i::CONTINUE },
        { "switch", token_i::SWITCH },
        { "case", token_i::CASE },
        { "default", token_i::DEFAULT },
        { "class", token_i::CLASS },
        { "extends", token_i::EXTENDS },
        { "final", token_i::FINAL },
        { "public", token_i::PUBLIC },
        { "private", token_i::PRIVATE },
        { "protected", token_i::PROTECTED },
        { "await", token_i::AWAIT },
        { "async", token_i::ASYNC },
        { "try", token_i::TRY },
        { "catch", token_i::CATCH },
        { "static", token_i::STATIC },
        { "from", token_i::FROM },
        { "type", token_i::TYPE },

        // Basic types
        { "u8", token_i::U8 },
        { "i8", token_i::I8 },
        { "u16", token_i::U16 },
        { "i16", token_i::I16 },
        { "u32", token_i::U32 },
        { "i32", token_i::I32 },
        { "u64", token_i::U64 },
        { "i64", token_i::I64 },
        { "f32", token_i::F32 },
        { "f64", token_i::F64 },
        { "string", token_i::STRING },
        { "boolean", token_i::BOOLEAN },
        { "void", token_i::VOID },
        { "auto", token_i::AUTO },
        { "Ptr", token_i::PTR },

        // Single character operators
        { "+", token_i::PLUS },
        { "-", token_i::MINUS },
        { "*", token_i::STAR },
        { "/", token_i::SLASH },
        { "%", token_i::PERCENT },
        { "=", token_i::EQUAL },
        { "!", token_i::BANG },
        { "<", token_i::LESS },
        { ">", token_i::GREATER },
        { "&", token_i::AND },
        { "|", token_i::OR },
        { "^", token_i::XOR },
        { "~", token_i::TILDE },
        { ".", token_i::DOT },

        // Delimiters
        { "(", token_i::LEFT_PAREN },
        { ")", token_i::RIGHT_PAREN },
        { "{", token_i::LEFT_BRACE },
        { "}", token_i::RIGHT_BRACE },
        { "[", token_i::LEFT_BRACKET },
        { "]", token_i::RIGHT_BRACKET },
        { ",", token_i::COMMA },
        { ":", token_i::COLON },
        { ";", token_i::SEMICOLON },
        { "?", token_i::QUESTION },

        // Annotations
        { "@alignas", token_i::ALIGN_ANNOT },
        { "@deprecated", token_i::DEPRECATED_ANNOT },
        { "@packed", token_i::PACKED_ANNOT },
        { "@nodiscard", token_i::NO_DISCARD_ANNOT },
        { "@volatile", token_i::VOLATILE_ANNOT },
        { "@lazy", token_i::LAZY_ANNOT },
        { "@pure", token_i::PURE_ANNOT },
        { "@tailrec", token_i::TAIL_REC_ANNOT }
    };

    /**
     * @brief Represents a token that the lexer has found.
     */
    struct alignas(8) token_t
    {
        uint32_t start;
        uint16_t length;
        token_i type;
        uint8_t flags;
    };

    /**
     * @brief A structure to efficiently store tokens.
     */
    struct alignas(8) TokenList
    {
        std::vector<uint32_t> starts;
        std::vector<uint16_t> lengths;
        std::vector<token_i> types;
        std::vector<uint8_t> flags;

        void push_back(const token_t &token);

        void reserve(const uint32_t &n);

        [[nodiscard]] size_t size() const;
    };
}

#endif
