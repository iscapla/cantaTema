#include <gtest/gtest.h>
#include "primitives/utils_logger.hpp"

int main(int argc, char **argv) {
    util_logger_init_for_test();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
