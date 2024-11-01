#include <iostream>
#include <string>

/**
 * @file yu-cli.cpp
 * @brief Simple Yu command line interface with color support.
 *      This file is a part of the Yu Programming Language.
 *      Licensed under the MIT License (MIT). See LICENSE file for more details.
 *
 * A basic CLI that accepts default commands, displays help, and provides
 * a "compile --help" command with colored output. Type "exit" to quit.
 */

const std::string RESET_COLOR = "\033[0m";
const std::string COLOR_DEFAULT = "\033[1;32m";
const std::string COLOR_HELP = "\033[1;36m";
const std::string COLOR_WARNING = "\033[1;33m";

void print_default()
{
    std::cout << "";
}

void print_help()
{
    std::cout << COLOR_HELP << "Help:\n";
    std::cout << "  (default)       : Default command output.\n";
    std::cout << "  --help          : Displays this help message.\n";
    std::cout << "  compile --help  : Shows help for the 'compile' command.\n";
    std::cout << "  exit            : Exits the Yu CLI." << RESET_COLOR << "\n";
}

void print_compile_help()
{
    std::cout << COLOR_HELP << "Compile Help:\n";
    std::cout << "  Usage:\n";
    std::cout << "    compile [options]\n";
    std::cout << "  Options:\n";
    std::cout << "    --help  : Show this help message for the compile command."
              << RESET_COLOR << "\n";
}

/**
 * @brief Main CLI loop for the Yu Programming Language.
 *
 * Runs an interactive command prompt where the user can type commands.
 *
 * @return Returns 0 upon successful completion.
 */
int main()
{
    std::string input;

    while (true)
    {
        std::cout << "Yu > ";
        if (!std::getline(std::cin, input))
        {
            std::cerr << "Error reading input. Exiting...\n";
            break;
        }

        input.erase(0, input.find_first_not_of(" \t\n\r\f\v"));
        input.erase(input.find_last_not_of(" \t\n\r\f\v") + 1);

        if (input.empty())
        {
            print_default();
        }
        else if (input == "--help")
        {
            print_help();
        }
        else if (input == "compile --help")
        {
            print_compile_help();
        }
        else if (input == "exit" || input == "quit")
        {
            break;
        }
        else
        {
            std::cout << COLOR_WARNING << "Unknown command. Type '--help' for a list of commands."
                      << RESET_COLOR << "\n";
        }
    }

    return 0;
}