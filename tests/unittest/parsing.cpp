// This file is part of the Yu programming language and is licensed under MIT License;
// See LICENSE.txt for details

#include <gtest/gtest.h>
#include "../include/lexer.h"
#include "../include/parser.h"

class ParserTest : public ::testing::Test
{
protected:
    static std::unique_ptr<yu::frontend::ir_node> tryParse(const char *code)
    {
        auto lexer = yu::frontend::create_lexer(code);
        const auto tokens = tokenize(lexer);
        return yu::frontend::parse(code, tokens);
    }
};

TEST_F(ParserTest, ExpressionParsing)
{
    const std::string code = R"(
        class Calculator
        {
            public function calc() -> i32
            {
                var a: i32 = 1 + 2 * 3;
                return a;
            }
        }
    )";
    EXPECT_NE(tryParse(code.data()), nullptr);
}

TEST_F(ParserTest, VariableDeclarations)
{
    const std::string code = R"(
        class Test
        {
            var a: i32;
            var b: i32 = 42;
            var c: string = "hello";
        }
    )";
    EXPECT_NE(tryParse(code.data()), nullptr);
}

// Test visibility modifiers
TEST_F(ParserTest, Visibility)
{
    const std::string code = R"(
        class Test
        {
            private var x: i32;
            public var y: i32;
            protected var z: i32;
        }
    )";
    EXPECT_NE(tryParse(code.data()), nullptr);
}

// Test return statements
TEST_F(ParserTest, ReturnStatements)
{
    const std::string code = R"(
        class Test
        {
            public function getValue() -> i32
            {
                return 42;
            }
        }
    )";
    EXPECT_NE(tryParse(code.data()), nullptr);
}

// Test method calls
TEST_F(ParserTest, MethodCalls)
{
    const std::string code = R"(
        class Test
        {
            public function a() -> void
            {
                self.b();
            }
            private function b() -> void { }
        }
    )";
    EXPECT_NE(tryParse(code.data()), nullptr);
}
