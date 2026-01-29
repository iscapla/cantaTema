#include <gtest/gtest.h>
#include <primitives/utils_functions.hpp>
#include <ctime>

namespace {

class UtilsFunctionsTest : public ::testing::Test {
protected:
    // Helper to create a tm struct initialized like the function does (zero-init)
    // but with specific fields set.
    std::tm CreateTm(int year, int month, int day, int hour, int min, int sec, int isdst = 0) {
        std::tm tm{}; // Zero init
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = min;
        tm.tm_sec = sec;
        tm.tm_isdst = isdst; 
        return tm;
    }
};

TEST_F(UtilsFunctionsTest, ParseTimeToTimeT_ValidInput) {
    std::string date_str = "2023-01-15 10:30:00";
    const char* format = "%Y-%m-%d %H:%M:%S";
    
    // The function initializes tm with {}, so tm_isdst is 0.
    // We must match this in our expectation for mktime to behave identically.
    std::tm tm_expected = CreateTm(2023, 1, 15, 10, 30, 0, 0);
    time_t expected = mktime(&tm_expected);

    time_t result = parse_time_to_time_t(date_str, format);
    
    EXPECT_EQ(result, expected);
}

TEST_F(UtilsFunctionsTest, ParseTimeToTimeT_EmptyString) {
    std::string date_str = "";
    const char* format = "%Y-%m-%d";
    
    time_t result = parse_time_to_time_t(date_str, format);
    
    EXPECT_EQ(result, 0);
}

TEST_F(UtilsFunctionsTest, ParseTimeToTimeT_InvalidFormat) {
    std::string date_str = "not-a-date";
    const char* format = "%Y-%m-%d";
    
    time_t result = parse_time_to_time_t(date_str, format);
    
    EXPECT_EQ(result, 0);
}

TEST_F(UtilsFunctionsTest, ParseTimeTToString_ValidInput) {
    // Use current time to ensure we have a valid time_t
    std::time_t now = std::time(nullptr);
    if (now == 0) now = 1; // Ensure non-zero

    const char* format = "%Y-%m-%d %H:%M:%S";
    
    // Expected string generation using standard functions
    std::tm* tm_ptr = std::localtime(&now);
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), format, tm_ptr);
    std::string expected(buffer);
    
    std::string result = parse_time_t_to_string(now, format);
    
    EXPECT_EQ(result, expected);
}

TEST_F(UtilsFunctionsTest, ParseTimeTToString_ZeroTime) {
    time_t time_val = 0;
    const char* format = "%Y-%m-%d";
    
    std::string result = parse_time_t_to_string(time_val, format);
    
    EXPECT_TRUE(result.empty());
}

} // namespace