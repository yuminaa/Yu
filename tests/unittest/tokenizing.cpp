#include <cassert>
#include <gtest/gtest.h>
#include "../../frontend/include/lexer.h"

TEST(lexer, variable)
{
    constexpr std::string_view src = "var x = 5;";
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    ASSERT_EQ(tokens->types.size(), 6);

    std::cout << "Token types: ";
    for (const auto& type : tokens->types)
        std::cout << static_cast<int>(type) << static_cast<std::string>(" ");
    std::cout << std::endl;

    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(0)], yu::lang::token_i::VAR);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(1)], yu::lang::token_i::IDENTIFIER);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(2)], yu::lang::token_i::EQUAL);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(3)], yu::lang::token_i::NUM_LITERAL);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(4)], yu::lang::token_i::SEMICOLON);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(5)], yu::lang::token_i::END_OF_FILE);

    std::cout << "Token count: " << tokens->types.size() << std::endl;
}

TEST(lexer, empty_input)
{
    yu::frontend::Lexer lexer = yu::frontend::create_lexer("");
    const auto tokens = tokenize(lexer);
    EXPECT_EQ(tokens->types.size(), static_cast<std::vector<yu::lang::token_i>::size_type>(1));
}

TEST(lexer, expression)
{
    constexpr std::string_view src = "if (x == 5)";
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);
    
    ASSERT_EQ(tokens->types.size(), 7);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(0)], yu::lang::token_i::IF);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(1)], yu::lang::token_i::LEFT_PAREN);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(2)], yu::lang::token_i::IDENTIFIER);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(3)], yu::lang::token_i::EQUAL_EQUAL);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(4)], yu::lang::token_i::NUM_LITERAL);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(5)], yu::lang::token_i::RIGHT_PAREN);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(6)], yu::lang::token_i::END_OF_FILE);
}

TEST(lexer, multi_character_operators)
{
    constexpr std::string_view src = "a += b << c";
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    ASSERT_EQ(tokens->types.size(), 6);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(0)], yu::lang::token_i::IDENTIFIER);  // a
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(1)], yu::lang::token_i::PLUS_EQUAL);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(2)], yu::lang::token_i::IDENTIFIER);  // b
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(3)], yu::lang::token_i::LEFT_SHIFT);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(4)], yu::lang::token_i::IDENTIFIER);
}

TEST(lexer, keywords_and_identifiers)
{
    constexpr std::string_view src = "if while for break continue";
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    ASSERT_EQ(tokens->types.size(), 6);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(0)], yu::lang::token_i::IF);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(1)], yu::lang::token_i::WHILE);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(2)], yu::lang::token_i::FOR);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(3)], yu::lang::token_i::BREAK);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(4)], yu::lang::token_i::CONTINUE);
}

TEST(lexer, string_literals)
{
    constexpr std::string_view src = R"("simple" "with\"escape" "multi
line")";
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    ASSERT_EQ(tokens->types.size(), static_cast<std::vector<yu::lang::token_i>::size_type>(4));
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(0)], yu::lang::token_i::STR_LITERAL);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(1)], yu::lang::token_i::STR_LITERAL);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(2)], yu::lang::token_i::STR_LITERAL);
}

TEST(lexer, number_literals)
{
    constexpr std::string_view src = "0xFF 3.14 0b101";
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    ASSERT_EQ(tokens->types.size(), 4);
    for (auto i = static_cast<std::vector<yu::lang::token_i>::size_type>(0); i < 3; ++i)
        EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(i)], yu::lang::token_i::NUM_LITERAL);
}

TEST(lexer, comments)
{
    constexpr std::string_view src = R"(
        // Single line comment
        x = 3; // Inline comment
        /* Multi
         * line
         * comment
         */
    )";
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    ASSERT_EQ(tokens->types.size(), 5);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(0)], yu::lang::token_i::IDENTIFIER); // x
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(1)], yu::lang::token_i::EQUAL);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(2)], yu::lang::token_i::NUM_LITERAL);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(3)], yu::lang::token_i::SEMICOLON);
}

TEST(lexer, whitespace_handling)
{
    constexpr std::string_view src = "if\t(x\n==\r\n5)\n{\n}";
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    ASSERT_EQ(tokens->types.size(), 9);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(0)], yu::lang::token_i::IF);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(1)], yu::lang::token_i::LEFT_PAREN);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(2)], yu::lang::token_i::IDENTIFIER);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(3)], yu::lang::token_i::EQUAL_EQUAL);
}

TEST(lexer, compound_operators)
{
    constexpr std::string_view src = "a += b -= c *= d /= e %= f";
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    ASSERT_EQ(tokens->types.size(), static_cast<std::vector<yu::lang::token_i>::size_type>(12));
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(0)], yu::lang::token_i::IDENTIFIER);  // a
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(1)], yu::lang::token_i::PLUS_EQUAL);  // +=
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(2)], yu::lang::token_i::IDENTIFIER);  // b
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(3)], yu::lang::token_i::MINUS_EQUAL); // -=
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(4)], yu::lang::token_i::IDENTIFIER);  // c
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(5)], yu::lang::token_i::STAR_EQUAL);  // *=
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(6)], yu::lang::token_i::IDENTIFIER);  // d
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(7)], yu::lang::token_i::SLASH_EQUAL); // /=
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(8)], yu::lang::token_i::IDENTIFIER);  // e
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(9)], yu::lang::token_i::PERCENT_EQUAL); // %=
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(10)], yu::lang::token_i::IDENTIFIER);   // f
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(11)], yu::lang::token_i::END_OF_FILE);
}

TEST(lexer, bitwise_operators)
{
    constexpr std::string_view src = "a &= b |= c";
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    ASSERT_EQ(tokens->types.size(), 6);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(0)], yu::lang::token_i::IDENTIFIER);    // a
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(1)], yu::lang::token_i::AND_EQUAL);     // &=
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(2)], yu::lang::token_i::IDENTIFIER);    // b
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(3)], yu::lang::token_i::OR_EQUAL);      // |=
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(4)], yu::lang::token_i::IDENTIFIER);    // c
}

TEST(lexer, comparison_operators)
{
    constexpr std::string_view src = "a == b != c";
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    ASSERT_EQ(tokens->types.size(), 6);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(0)], yu::lang::token_i::IDENTIFIER);     // a
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(1)], yu::lang::token_i::EQUAL_EQUAL);    // ==
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(2)], yu::lang::token_i::IDENTIFIER);     // b
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(3)], yu::lang::token_i::BANG_EQUAL);     // !=
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(4)], yu::lang::token_i::IDENTIFIER);     // c
}

TEST(lexer, logical_operators)
{
    constexpr std::string_view src = "a && b || c";
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    ASSERT_EQ(tokens->types.size(), 6);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(0)], yu::lang::token_i::IDENTIFIER);  // a
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(1)], yu::lang::token_i::AND_AND);     // &&
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(2)], yu::lang::token_i::IDENTIFIER);  // b
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(3)], yu::lang::token_i::OR_OR);       // ||
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(4)], yu::lang::token_i::IDENTIFIER);  // c
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(5)], yu::lang::token_i::END_OF_FILE);
}

TEST(lexer, mixed_operators)
{
    constexpr std::string_view src = "a += 5";
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    ASSERT_EQ(tokens->types.size(), static_cast<std::vector<yu::lang::token_i>::size_type>(4));
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(0)], yu::lang::token_i::IDENTIFIER);    // a
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(1)], yu::lang::token_i::PLUS_EQUAL);    // +=
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(2)], yu::lang::token_i::NUM_LITERAL);   // 5
}

TEST(lexer, shift_operators)
{
    constexpr std::string_view src = "a <<";
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    ASSERT_EQ(tokens->types.size(), static_cast<std::vector<yu::lang::token_i>::size_type>(3));
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(0)], yu::lang::token_i::IDENTIFIER);     // a
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(1)], yu::lang::token_i::LEFT_SHIFT);     // <<
}

TEST(lexer, operator_spacing)
{
    constexpr std::string_view src = "a+=b<<=c>>d";  // No spaces
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    ASSERT_EQ(tokens->types.size(), 8);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(0)], yu::lang::token_i::IDENTIFIER);     // a
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(1)], yu::lang::token_i::PLUS_EQUAL);     // +=
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(2)], yu::lang::token_i::IDENTIFIER);     // b
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(3)], yu::lang::token_i::LEFT_SHIFT_EQUAL); // <<=
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(4)], yu::lang::token_i::IDENTIFIER);     // c
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(5)], yu::lang::token_i::RIGHT_SHIFT);    // >>
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(6)], yu::lang::token_i::IDENTIFIER);     // d
}

TEST(lexer, class_with_attributes)
{
    constexpr std::string_view src = R"(
        @packed
        @aligned(16)
        class Vector3
        {
            var x: f32;
            var y: f32;
        }
    )";

    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    const std::vector expected_tokens = {
        yu::lang::token_i::PACKED_ANNOT,
        yu::lang::token_i::ALIGN_ANNOT,
        yu::lang::token_i::LEFT_PAREN,
        yu::lang::token_i::NUM_LITERAL,
        yu::lang::token_i::RIGHT_PAREN,
        yu::lang::token_i::CLASS,
        yu::lang::token_i::IDENTIFIER,
        yu::lang::token_i::LEFT_BRACE,
        yu::lang::token_i::VAR,
        yu::lang::token_i::IDENTIFIER,
        yu::lang::token_i::COLON,
        yu::lang::token_i::F32,
        yu::lang::token_i::SEMICOLON,
        yu::lang::token_i::VAR,
        yu::lang::token_i::IDENTIFIER,
        yu::lang::token_i::COLON,
        yu::lang::token_i::F32,
        yu::lang::token_i::SEMICOLON,
        yu::lang::token_i::RIGHT_BRACE
    };

    ASSERT_EQ(tokens->types.size(), expected_tokens.size() + 1);
}

TEST(lexer, generic_types)
{
    constexpr std::string_view src = R"(
        class Generic<T> {
            var value: T;
        }
    )";
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    ASSERT_GT(tokens->types.size(), static_cast<std::vector<yu::lang::token_i>::size_type>(0));
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(0)], yu::lang::token_i::CLASS);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(1)], yu::lang::token_i::IDENTIFIER);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(2)], yu::lang::token_i::LESS);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(3)], yu::lang::token_i::IDENTIFIER);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(4)], yu::lang::token_i::GREATER);
}

TEST(lexer, function_with_return_type)
{
    constexpr std::string_view src = R"(
        public function move(x: i32, y: i32) ->
    )";
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    ASSERT_EQ(tokens->types.size(), static_cast<std::vector<yu::lang::token_i>::size_type>(14)); // 13 tokens + EOF

    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(0)], yu::lang::token_i::PUBLIC);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(1)], yu::lang::token_i::FUNCTION);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(2)], yu::lang::token_i::IDENTIFIER);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(3)], yu::lang::token_i::LEFT_PAREN);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(4)], yu::lang::token_i::IDENTIFIER);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(5)], yu::lang::token_i::COLON);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(6)], yu::lang::token_i::I32);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(7)], yu::lang::token_i::COMMA);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(8)], yu::lang::token_i::IDENTIFIER);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(9)], yu::lang::token_i::COLON);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(10)], yu::lang::token_i::I32);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(11)], yu::lang::token_i::RIGHT_PAREN);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(12)], yu::lang::token_i::ARROW);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(13)], yu::lang::token_i::END_OF_FILE);
}

TEST(lexer, import_statement)
{
    constexpr std::string_view src = R"(
        import { io } from 'system';
    )";
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    ASSERT_GT(tokens->types.size(), static_cast<std::vector<yu::lang::token_i>::size_type>(0));
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(0)], yu::lang::token_i::IMPORT);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(1)], yu::lang::token_i::LEFT_BRACE);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(2)], yu::lang::token_i::IDENTIFIER); // io
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(3)], yu::lang::token_i::RIGHT_BRACE);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(4)], yu::lang::token_i::FROM);
}

TEST(lexer, smart_pointer)
{
    constexpr std::string_view src = R"(
        var strPtr: Ptr<string> = new Ptr<string>("Test");
        strPtr.release();
    )";
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    ASSERT_GT(tokens->types.size(), static_cast<std::vector<yu::lang::token_i>::size_type>(0));
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(0)], yu::lang::token_i::VAR);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(1)], yu::lang::token_i::IDENTIFIER); // strPtr
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(2)], yu::lang::token_i::COLON);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(3)], yu::lang::token_i::PTR); // Ptr
}

TEST(lexer, inline_function)
{
    constexpr std::string_view src = R"(
        public inline function GetValue() -> T
    )";
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    ASSERT_GT(tokens->types.size(), static_cast<std::vector<yu::lang::token_i>::size_type>(0));
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(0)], yu::lang::token_i::PUBLIC);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(1)], yu::lang::token_i::INLINE);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(2)], yu::lang::token_i::FUNCTION);
}

TEST(lexer, constructor)
{
    constexpr std::string_view src = R"(
        public Player() -> Player
    )";
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    const std::vector<std::pair<yu::lang::token_i, const char*>> expected = {
        {yu::lang::token_i::PUBLIC, "public"},
        {yu::lang::token_i::IDENTIFIER, "Player"},
        {yu::lang::token_i::LEFT_PAREN, "("},
        {yu::lang::token_i::RIGHT_PAREN, ")"},
        {yu::lang::token_i::ARROW, "->"},
        {yu::lang::token_i::IDENTIFIER, "Player"}
    };

    ASSERT_EQ(tokens->types.size(), expected.size() + 1); // +1 for EOF
}

TEST(lexer, static_function)
{
    constexpr std::string_view src = R"(
        public static dot() -> f32
    )";
    yu::frontend::Lexer lexer = yu::frontend::create_lexer(src);
    const auto tokens = tokenize(lexer);

    ASSERT_GT(tokens->types.size(), static_cast<std::vector<yu::lang::token_i>::size_type>(0));
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(0)], yu::lang::token_i::PUBLIC);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(1)], yu::lang::token_i::STATIC);
    EXPECT_EQ(tokens->types[static_cast<std::vector<yu::lang::token_i>::size_type>(2)], yu::lang::token_i::IDENTIFIER); // dot
}