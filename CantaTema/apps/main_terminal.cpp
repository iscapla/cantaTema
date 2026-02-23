#include <iostream>
#include <string>


#include "primitives/utils_logger.hpp"
#include "terminal/terminal_cli.hpp"

int main(int argc, char const *argv[])
{

    util_logger_init();

    logger->info("Welcome to CantaTema!");

    terminal_cli_start();

    return 0;
}
