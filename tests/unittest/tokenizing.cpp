// This file is part of the Yu programming language and is licensed under MIT License;
// See LICENSE.txt for details

// ReSharper disable CppDFAUnusedValue
#include <iomanip>
#include <gtest/gtest.h>
#include "../../frontend/include/lexer.h"

using namespace yu::frontend;
using namespace yu::lang;

class TokenPrinter
{
    static constexpr auto YELLOW = "\033[33m";
    static constexpr auto RESET = "\033[0m";

    static std::string get_token_name(const token_i type)
    {
        switch (type)
        {
            case token_i::TRUE:
                return "TRUE";
            case token_i::FALSE:
                return "FALSE";
            case token_i::NIL:
                return "NIL";
            case token_i::IMPORT:
                return "IMPORT";
            case token_i::VAR:
                return "VAR";
            case token_i::CONST:
                return "CONST";
            case token_i::FUNCTION:
                return "FUNCTION";
            case token_i::INLINE:
                return "INLINE";
            case token_i::RETURN:
                return "RETURN";
            case token_i::NEW:
                return "NEW";
            case token_i::ENUM:
                return "ENUM";
            case token_i::IF:
                return "IF";
            case token_i::ELSE:
                return "ELSE";
            case token_i::FOR:
                return "FOR";
            case token_i::WHILE:
                return "WHILE";
            case token_i::BREAK:
                return "BREAK";
            case token_i::CONTINUE:
                return "CONTINUE";
            case token_i::SWITCH:
                return "SWITCH";
            case token_i::CASE:
                return "CASE";
            case token_i::DEFAULT:
                return "DEFAULT";
            case token_i::CLASS:
                return "CLASS";
            case token_i::EXTENDS:
                return "EXTENDS";
            case token_i::FINAL:
                return "FINAL";
            case token_i::PUBLIC:
                return "PUBLIC";
            case token_i::PRIVATE:
                return "PRIVATE";
            case token_i::PROTECTED:
                return "PROTECTED";
            case token_i::STATIC:
                return "STATIC";
            case token_i::AWAIT:
                return "AWAIT";
            case token_i::ASYNC:
                return "ASYNC";
            case token_i::TRY:
                return "TRY";
            case token_i::CATCH:
                return "CATCH";
            case token_i::FROM:
                return "FROM";
            // Basic types
            case token_i::U8:
                return "U8";
            case token_i::I8:
                return "I8";
            case token_i::U16:
                return "U16";
            case token_i::I16:
                return "I16";
            case token_i::U32:
                return "U32";
            case token_i::I32:
                return "I32";
            case token_i::U64:
                return "U64";
            case token_i::I64:
                return "I64";
            case token_i::F32:
                return "F32";
            case token_i::F64:
                return "F64";
            case token_i::STRING:
                return "STRING";
            case token_i::BOOLEAN:
                return "BOOLEAN";
            case token_i::VOID:
                return "VOID";
            case token_i::AUTO:
                return "AUTO";
            case token_i::PTR:
                return "PTR";
            // Operators
            case token_i::PLUS:
                return "PLUS";
            case token_i::MINUS:
                return "MINUS";
            case token_i::STAR:
                return "STAR";
            case token_i::SLASH:
                return "SLASH";
            case token_i::PERCENT:
                return "PERCENT";
            case token_i::EQUAL:
                return "EQUAL";
            case token_i::BANG:
                return "BANG";
            case token_i::LESS:
                return "LESS";
            case token_i::GREATER:
                return "GREATER";
            case token_i::AND:
                return "AND";
            case token_i::OR:
                return "OR";
            case token_i::XOR:
                return "XOR";
            case token_i::TILDE:
                return "TILDE";
            case token_i::DOT:
                return "DOT";
            // Delimiters
            case token_i::LEFT_PAREN:
                return "LEFT_PAREN";
            case token_i::RIGHT_PAREN:
                return "RIGHT_PAREN";
            case token_i::LEFT_BRACE:
                return "LEFT_BRACE";
            case token_i::RIGHT_BRACE:
                return "RIGHT_BRACE";
            case token_i::LEFT_BRACKET:
                return "LEFT_BRACKET";
            case token_i::RIGHT_BRACKET:
                return "RIGHT_BRACKET";
            case token_i::COMMA:
                return "COMMA";
            case token_i::COLON:
                return "COLON";
            case token_i::SEMICOLON:
                return "SEMICOLON";
            case token_i::QUESTION:
                return "QUESTION";
            // Special tokens
            case token_i::IDENTIFIER:
                return "IDENTIFIER";
            case token_i::NUM_LITERAL:
                return "NUM_LITERAL";
            case token_i::STR_LITERAL:
                return "STR_LITERAL";
            case token_i::ANNOTATION:
                return "ANNOTATION";
            case token_i::UNKNOWN:
                return "UNKNOWN";
            case token_i::END_OF_FILE:
                return "EOF";
            default:
                return "UNHANDLED_TOKEN";
        }
    }

public:
    static std::string print(const TokenList &tokens, const Lexer &lexer)
    {
        std::stringstream ss;
        ss << YELLOW << "Tokens:" << RESET << "\n";

        for (std::vector<unsigned>::size_type i = 0; i < tokens.types.size(); ++i)
        {
            auto [line, col] = get_line_col(lexer, {
                                                tokens.starts[i], tokens.lengths[i],
                                                tokens.types[i], tokens.flags[i]
                                            });
            const std::string_view token_text = get_token_value(lexer, {
                                                                    tokens.starts[i], tokens.lengths[i],
                                                                    tokens.types[i], tokens.flags[i]
                                                                });

            ss << YELLOW
                    << "[" << std::setw(3) << i << "] "
                    << std::setw(15) << get_token_name(tokens.types[i]) << " | "
                    << "'" << token_text << "'"
                    << " (line: " << line << ", col: " << col
                    << ", pos: " << tokens.starts[i] << ", len: " << tokens.lengths[i] << ")"
                    << RESET << "\n";
        }

        return ss.str();
    }

    static std::string print(const token_t &token, const Lexer &lexer)
    {
        std::stringstream ss;
        auto [line, col] = get_line_col(lexer, token);
        const std::string_view token_text = get_token_value(lexer, token);

        ss << YELLOW
                << get_token_name(token.type) << " | "
                << "'" << token_text << "'"
                << " (line: " << line << ", col: " << col
                << ", pos: " << token.start << ", len: " << token.length << ")"
                << RESET;

        return ss.str();
    }
};


class LexerTest : public testing::Test
{
protected:
    void verify_token(const TokenList *tokens, auto index, const token_i expected_type,
                      const std::string_view &source)
    {
        ASSERT_LT(index, tokens->types.size())
            << "Token index out of range";

        const token_t token = {
            tokens->starts[index],
            tokens->lengths[index],
            tokens->types[index],
            tokens->flags[index]
        };

        EXPECT_EQ(tokens->types[index], expected_type)
            << "Token type mismatch at index " << index << ":\n"
            << "Expected: " << TokenPrinter::print({ token.start, token.length, expected_type, token.flags }, lexer) <<
 "\n"
            << "Actual:   " << TokenPrinter::print(token, lexer);

        auto actual_value = get_token_value(lexer, token);
        auto expected_value = std::string_view(source.data() + tokens->starts[index], tokens->lengths[index]);
        EXPECT_EQ(expected_value, actual_value)
            << "Token value mismatch at index " << index << ":\n"
            << "Expected: '" << expected_value << "'\n"
            << "Actual:   '" << actual_value << "'";
    }

    void dump_tokens(const TokenList *tokens) const
    {
        std::cout << TokenPrinter::print(*tokens, lexer);
    }

    void SetUp() override {}

    void TearDown() override {}

    Lexer lexer;
};

TEST_F(LexerTest, NestedGenerics)
{
    constexpr std::string_view source = "var matrix: Array<Array<Vector3<T>>>;";
    lexer = create_lexer(source);
    const auto tokens = tokenize(lexer);

    size_t i = 0;
    verify_token(tokens, i++, token_i::VAR, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // matrix
    verify_token(tokens, i++, token_i::COLON, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // Array
    verify_token(tokens, i++, token_i::LESS, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // Array
    verify_token(tokens, i++, token_i::LESS, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // Vector3
    verify_token(tokens, i++, token_i::LESS, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // T
    verify_token(tokens, i++, token_i::GREATER, source);
    verify_token(tokens, i++, token_i::GREATER, source);
    verify_token(tokens, i++, token_i::GREATER, source);
    verify_token(tokens, i++, token_i::SEMICOLON, source);
}

TEST_F(LexerTest, GenericConstraints)
{
    constexpr std::string_view source = R"(
        class DataStructure<T: Comparable & Serializable, U: Container<T>>
        {
            var data: U;
        }
    )";

    lexer = create_lexer(source);
    const auto tokens = tokenize(lexer);

    size_t i = 0;
    verify_token(tokens, i++, token_i::CLASS, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // DataStructure
    verify_token(tokens, i++, token_i::LESS, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // T
    verify_token(tokens, i++, token_i::COLON, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // Comparable
    verify_token(tokens, i++, token_i::AND, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // Serializable
    verify_token(tokens, i++, token_i::COMMA, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // U
    verify_token(tokens, i++, token_i::COLON, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // Container
    verify_token(tokens, i++, token_i::LESS, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // T
    verify_token(tokens, i++, token_i::GREATER, source);
}

TEST_F(LexerTest, ComplexTypeAnnotations)
{
    constexpr std::string_view source = R"(
        @nodiscard
        @pure
        @alignas(32)
        public function transform<T, U>(input: Array<T>) -> Result<U, Error>
        {
            return Result.ok(transform_internal(input));
        }
    )";

    lexer = create_lexer(source);
    const auto tokens = tokenize(lexer);

    size_t i = 0;
    verify_token(tokens, i++, token_i::NO_DISCARD_ANNOT, source);
    verify_token(tokens, i++, token_i::PURE_ANNOT, source);
    verify_token(tokens, i++, token_i::ALIGN_ANNOT, source);
    verify_token(tokens, i++, token_i::LEFT_PAREN, source);
    verify_token(tokens, i++, token_i::NUM_LITERAL, source);
    verify_token(tokens, i++, token_i::RIGHT_PAREN, source);
}

TEST_F(LexerTest, ComplexNestedExpressions)
{
    constexpr std::string_view source = R"(
        result = matrix[i * 2 + 1][j - (k * 3)].transform<U>().value;
    )";

    lexer = create_lexer(source);
    auto tokens = tokenize(lexer);

    size_t i = 0;
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // result
    verify_token(tokens, i++, token_i::EQUAL, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // matrix
    verify_token(tokens, i++, token_i::LEFT_BRACKET, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // i
    verify_token(tokens, i++, token_i::STAR, source);
    verify_token(tokens, i++, token_i::NUM_LITERAL, source); // 2
    verify_token(tokens, i++, token_i::PLUS, source);
    verify_token(tokens, i++, token_i::NUM_LITERAL, source); // 1
    verify_token(tokens, i++, token_i::RIGHT_BRACKET, source);
}

TEST_F(LexerTest, ComplexStringLiterals)
{
    constexpr std::string_view source = R"(
        var str1 = "String with \"escaped\" quotes";
        var str2 = "Multi-line
                    string with
                    line breaks";
    )";

    lexer = create_lexer(source);
    auto tokens = tokenize(lexer);

    size_t i = 0;
    verify_token(tokens, i++, token_i::VAR, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // str1
    verify_token(tokens, i++, token_i::EQUAL, source);
    verify_token(tokens, i++, token_i::STR_LITERAL, source);
    verify_token(tokens, i++, token_i::SEMICOLON, source);
}

TEST_F(LexerTest, MethodChainingWithGenerics)
{
    constexpr std::string_view source = R"(
        result = value.map<U>()
                     .filter<V>()
                     .transform<W>()
                     .unwrap_or(default_value);
    )";

    lexer = create_lexer(source);
    const auto tokens = tokenize(lexer);

    size_t i = 0;
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // result
    verify_token(tokens, i++, token_i::EQUAL, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // value
    verify_token(tokens, i++, token_i::DOT, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // map
    verify_token(tokens, i++, token_i::LESS, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // U
    verify_token(tokens, i++, token_i::GREATER, source);
}

TEST_F(LexerTest, ComplexTemplateInstantiation)
{
    constexpr std::string_view source = R"(
        var data = HashMap<string, vector<result<T, error>>>(
            { capacity = 100, load_factor = 0.75 }
        );
    )";

    lexer = create_lexer(source);
    auto tokens = tokenize(lexer);

    size_t i = 0;
    verify_token(tokens, i++, token_i::VAR, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // data
    verify_token(tokens, i++, token_i::EQUAL, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // HashMap
    verify_token(tokens, i++, token_i::LESS, source);
    verify_token(tokens, i++, token_i::STRING, source); // string
    verify_token(tokens, i++, token_i::COMMA, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // vector
}

TEST_F(LexerTest, OperatorPrecedence)
{
    constexpr std::string_view source = "x = (-a * (b + c)) / (d - e);";

    lexer = create_lexer(source);
    const auto tokens = tokenize(lexer);

    size_t i = 0;
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // x
    verify_token(tokens, i++, token_i::EQUAL, source);
    verify_token(tokens, i++, token_i::LEFT_PAREN, source);
    verify_token(tokens, i++, token_i::MINUS, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // a
    verify_token(tokens, i++, token_i::STAR, source);
}

TEST_F(LexerTest, NumberLiterals)
{
    constexpr std::string_view source = R"(
        var decimal = 123;
        var hex = 0xFF;
        var binary = 0b1010;
        var float_num = 1.234;
        var scientific = 1.23e-4;
        var big_hex = 0xFFFFFFFFFFFFFFFF;
    )";

    lexer = create_lexer(source);
    const auto tokens = tokenize(lexer);

    size_t i = 0;
    verify_token(tokens, i++, token_i::VAR, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // decimal
    verify_token(tokens, i++, token_i::EQUAL, source);
    verify_token(tokens, i++, token_i::NUM_LITERAL, source); // 123
    verify_token(tokens, i++, token_i::SEMICOLON, source);

    verify_token(tokens, i++, token_i::VAR, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // hex
    verify_token(tokens, i++, token_i::EQUAL, source);
    verify_token(tokens, i++, token_i::NUM_LITERAL, source); // 0xFF
    verify_token(tokens, i++, token_i::SEMICOLON, source);

    verify_token(tokens, i++, token_i::VAR, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // binary
    verify_token(tokens, i++, token_i::EQUAL, source);
    verify_token(tokens, i++, token_i::NUM_LITERAL, source); // 0b1010
    verify_token(tokens, i++, token_i::SEMICOLON, source);

    verify_token(tokens, i++, token_i::VAR, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // float_num
    verify_token(tokens, i++, token_i::EQUAL, source);
    verify_token(tokens, i++, token_i::NUM_LITERAL, source); // 1.234
    verify_token(tokens, i++, token_i::SEMICOLON, source);

    verify_token(tokens, i++, token_i::VAR, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // scientific
    verify_token(tokens, i++, token_i::EQUAL, source);
    verify_token(tokens, i++, token_i::NUM_LITERAL, source); // 1.23e-4
    verify_token(tokens, i++, token_i::SEMICOLON, source);
}

TEST_F(LexerTest, ErrorRecovery)
{
    constexpr std::string_view source = R"(
        var x = @#$%^;
        var y = 42;
        var z = @;
        var valid = true;
    )";

    lexer = create_lexer(source);
    const auto tokens = tokenize(lexer);

    size_t i = 0;
    verify_token(tokens, i++, token_i::VAR, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source);
    verify_token(tokens, i++, token_i::EQUAL, source);

    while (i < tokens->types.size() && tokens->types[static_cast<std::vector<token_i>::size_type>(i)] !=
           token_i::SEMICOLON)
        i++;
    verify_token(tokens, i++, token_i::SEMICOLON, source); // ;

    verify_token(tokens, i++, token_i::VAR, source);         // var
    verify_token(tokens, i++, token_i::IDENTIFIER, source);  // y
    verify_token(tokens, i++, token_i::EQUAL, source);       // =
    verify_token(tokens, i++, token_i::NUM_LITERAL, source); // 42
    verify_token(tokens, i++, token_i::SEMICOLON, source);   // ;

    verify_token(tokens, i++, token_i::VAR, source);        // var
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // z
    verify_token(tokens, i++, token_i::EQUAL, source);      // =
    i++;                                                    // Skip the @ token
    verify_token(tokens, i++, token_i::SEMICOLON, source);  // ;

    verify_token(tokens, i++, token_i::VAR, source);        // var
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // valid
    verify_token(tokens, i++, token_i::EQUAL, source);      // =
    verify_token(tokens, i++, token_i::TRUE, source);       // true
    verify_token(tokens, i++, token_i::SEMICOLON, source);  // ;

    ASSERT_EQ(tokens->types[static_cast<std::vector<token_i>::size_type>(i)], token_i::END_OF_FILE);
}

TEST_F(LexerTest, ComplexEscapeSequences)
{
    constexpr std::string_view source = R"(
        var str1 = "Escaped \"quotes\" and \n newlines";
        var str2 = "Tabs\t and \r returns";
        var str3 = "\x48\x65\x6C\x6C\x6F"; // Hello in hex
        var str4 = "Mixed \n\t\"escapes\"";
    )";

    lexer = create_lexer(source);
    const auto tokens = tokenize(lexer);

    size_t i = 0;
    verify_token(tokens, i++, token_i::VAR, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source);
    verify_token(tokens, i++, token_i::EQUAL, source);
    verify_token(tokens, i++, token_i::STR_LITERAL, source);
    verify_token(tokens, i++, token_i::SEMICOLON, source);
}

TEST_F(LexerTest, CommentRecovery)
{
    constexpr std::string_view source = R"(
        var x = 1; // normal comment
        var y = 2; /* multi
        line comment */ var z = 3;
        // unterminated /*
        var a = 4;
        /* unterminated
        var b = 5;
    )";

    lexer = create_lexer(source);
    const auto tokens = tokenize(lexer);

    size_t i = 0;
    verify_token(tokens, i++, token_i::VAR, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // x
    verify_token(tokens, i++, token_i::EQUAL, source);
    verify_token(tokens, i++, token_i::NUM_LITERAL, source); // 1
    verify_token(tokens, i++, token_i::SEMICOLON, source);

    verify_token(tokens, i++, token_i::VAR, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // y
    verify_token(tokens, i++, token_i::EQUAL, source);
    verify_token(tokens, i++, token_i::NUM_LITERAL, source); // 2
    verify_token(tokens, i++, token_i::SEMICOLON, source);

    verify_token(tokens, i++, token_i::VAR, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // z
    verify_token(tokens, i++, token_i::EQUAL, source);
    verify_token(tokens, i++, token_i::NUM_LITERAL, source); // 3
    verify_token(tokens, i++, token_i::SEMICOLON, source);

    verify_token(tokens, i++, token_i::VAR, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source); // a
    verify_token(tokens, i++, token_i::EQUAL, source);
    verify_token(tokens, i++, token_i::NUM_LITERAL, source); // 4
    verify_token(tokens, i++, token_i::SEMICOLON, source);
}

TEST_F(LexerTest, StringRecovery)
{
    constexpr std::string_view source = R"(
        var str1 = "valid string";
        var str2 = "unterminated string
        var str3 = "string with \"escaped\" quotes";
        var str4 = "string with \n\t escapes";
    )";

    lexer = create_lexer(source);
    const auto tokens = tokenize(lexer);

    size_t i = 0;
    verify_token(tokens, i++, token_i::VAR, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source);
    verify_token(tokens, i++, token_i::EQUAL, source);
    verify_token(tokens, i++, token_i::STR_LITERAL, source);
    verify_token(tokens, i++, token_i::SEMICOLON, source);

    verify_token(tokens, i++, token_i::VAR, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source);
    verify_token(tokens, i++, token_i::EQUAL, source);
    while (tokens->types[static_cast<std::vector<token_i>::size_type>(i)] != token_i::VAR)
        i++;

    verify_token(tokens, i++, token_i::VAR, source);
    verify_token(tokens, i++, token_i::IDENTIFIER, source);
    verify_token(tokens, i++, token_i::EQUAL, source);
    verify_token(tokens, i++, token_i::STR_LITERAL, source);
    verify_token(tokens, i++, token_i::SEMICOLON, source);
}

TEST_F(LexerTest, ComplexIdentifiers)
{
    constexpr std::string_view source = R"(
        var _underscore = 1;
        var camelCase = 2;
        var PascalCase = 3;
        var with_numbers123 = 4;
        var @annotated = 5;
        var $invalid = 6;
        var 123invalid = 7;
    )";

    lexer = create_lexer(source);
    const auto tokens = tokenize(lexer);

    size_t i = 0;
    auto verify_identifier = [&](const std::string_view &expected)
    {
        verify_token(tokens, i++, token_i::VAR, source);
        verify_token(tokens, i++, token_i::IDENTIFIER, source);
        EXPECT_EQ(get_token_value(lexer, {tokens->starts[static_cast<std::vector<unsigned>::size_type>(i - 1)],
                      tokens->lengths[static_cast<std::vector<unsigned>::size_type>(i - 1)],
                      tokens->types[static_cast<std::vector<unsigned>::size_type>(i - 1)],
                      tokens->flags[static_cast<std::vector<unsigned>::size_type>(i - 1)]}),
                  expected);

        verify_token(tokens, i++, token_i::EQUAL, source);
        verify_token(tokens, i++, token_i::NUM_LITERAL, source);
        verify_token(tokens, i++, token_i::SEMICOLON, source);
    };

    verify_identifier("_underscore");
    verify_identifier("camelCase");
    verify_identifier("PascalCase");
    verify_identifier("with_numbers123");
    verify_identifier("@annotated");
}

TEST_F(LexerTest, Array)
{
    constexpr std::string_view source = R"(
        var arr: [ u8 ] = { 1, 2, 3, 4, 5 };
        var nested: [ [ u8 ] ] = { { 1, 2 }, { 3, 4 }, { 5 } };
    )";

    lexer = create_lexer(source);
    const auto tokens = tokenize(lexer);

    std::vector<unsigned>::size_type i = 0; // Using iterative index

    EXPECT_EQ(tokens->types[i++], token_i::VAR);
    EXPECT_EQ(tokens->types[i++], token_i::IDENTIFIER);
    EXPECT_EQ(tokens->types[i++], token_i::COLON);

    // Array type syntax [ u8 ]
    EXPECT_EQ(tokens->types[i++], token_i::LEFT_BRACKET);
    EXPECT_EQ(tokens->types[i++], token_i::U8);
    EXPECT_EQ(tokens->types[i++], token_i::RIGHT_BRACKET);

    EXPECT_EQ(tokens->types[i++], token_i::EQUAL);

    // Array literal { 1, 2, 3, 4, 5 }
    EXPECT_EQ(tokens->types[i++], token_i::LEFT_BRACE);
    EXPECT_EQ(tokens->types[i++], token_i::NUM_LITERAL);
    EXPECT_EQ(tokens->types[i++], token_i::COMMA);
    EXPECT_EQ(tokens->types[i++], token_i::NUM_LITERAL);
    EXPECT_EQ(tokens->types[i++], token_i::COMMA);
    EXPECT_EQ(tokens->types[i++], token_i::NUM_LITERAL);
    EXPECT_EQ(tokens->types[i++], token_i::COMMA);
    EXPECT_EQ(tokens->types[i++], token_i::NUM_LITERAL);
    EXPECT_EQ(tokens->types[i++], token_i::COMMA);
    EXPECT_EQ(tokens->types[i++], token_i::NUM_LITERAL);
    EXPECT_EQ(tokens->types[i++], token_i::RIGHT_BRACE);
    EXPECT_EQ(tokens->types[i++], token_i::SEMICOLON);

    // Second array declaration (nested): var nested: [ [ u8 ] ] = { { 1, 2 }, { 3, 4 }, { 5 } };
    EXPECT_EQ(tokens->types[i++], token_i::VAR);

    EXPECT_EQ(tokens->types[i++], token_i::IDENTIFIER);

    EXPECT_EQ(tokens->types[i++], token_i::COLON);

    // Nested array type [ [ u8 ] ]
    EXPECT_EQ(tokens->types[i++], token_i::LEFT_BRACKET);
    EXPECT_EQ(tokens->types[i++], token_i::LEFT_BRACKET);
    EXPECT_EQ(tokens->types[i++], token_i::U8);
    EXPECT_EQ(tokens->types[i++], token_i::RIGHT_BRACKET);
    EXPECT_EQ(tokens->types[i++], token_i::RIGHT_BRACKET);

    EXPECT_EQ(tokens->types[i++], token_i::EQUAL);

    // Nested array literal { { 1, 2 }, { 3, 4 }, { 5 } }
    EXPECT_EQ(tokens->types[i++], token_i::LEFT_BRACE);

    // First inner array { 1, 2 }
    EXPECT_EQ(tokens->types[i++], token_i::LEFT_BRACE);
    EXPECT_EQ(tokens->types[i++], token_i::NUM_LITERAL);
    EXPECT_EQ(tokens->types[i++], token_i::COMMA);
    EXPECT_EQ(tokens->types[i++], token_i::NUM_LITERAL);
    EXPECT_EQ(tokens->types[i++], token_i::RIGHT_BRACE);
    EXPECT_EQ(tokens->types[i++], token_i::COMMA);

    // Second inner array { 3, 4 }
    EXPECT_EQ(tokens->types[i++], token_i::LEFT_BRACE);
    EXPECT_EQ(tokens->types[i++], token_i::NUM_LITERAL);
    EXPECT_EQ(tokens->types[i++], token_i::COMMA);
    EXPECT_EQ(tokens->types[i++], token_i::NUM_LITERAL);
    EXPECT_EQ(tokens->types[i++], token_i::RIGHT_BRACE);
    EXPECT_EQ(tokens->types[i++], token_i::COMMA);

    EXPECT_EQ(tokens->types[i++], token_i::LEFT_BRACE);
    EXPECT_EQ(tokens->types[i++], token_i::NUM_LITERAL);
    EXPECT_EQ(tokens->types[i++], token_i::RIGHT_BRACE);

    EXPECT_EQ(tokens->types[i++], token_i::RIGHT_BRACE);
    EXPECT_EQ(tokens->types[i++], token_i::SEMICOLON);

    // Verify we got all tokens and the count is correct
    EXPECT_EQ(tokens->types[i++], token_i::END_OF_FILE);
    EXPECT_EQ(i, tokens->size());
}
