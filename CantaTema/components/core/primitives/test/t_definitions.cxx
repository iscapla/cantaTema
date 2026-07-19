#include <gtest/gtest.h>
#include "primitives/definitions.hpp"

TEST(DefinitionsTest, CodeToTextMapping) {
    // List of all enum values to verify get_rst_code and get_rst_txt
    std::vector<rst_code_e> codes = {
        RST_OK,
        CONFIG_FILE,
        CONFIG_PARSE,
        DB_FAIL,
        DB_NOT_FOUND,
        DB_BAD_PARAM,
        CONSOLE_EXP,
        USER_ERROR,
        USER_NOT_FOUND,
        USER_NO_AUTH,
        USER_DUPLICATED,
        USER_METRICS_ERROR,
        USER_METRICS_NOT_FOUND,
        USER_METRICS_NOT_ENOUGH_SPACE,
        CATEGORY_ERROR,
        CATEGORY_NOT_FOUND,
        CATEGORY_DUPLICATED,
        SUBJECT_ERROR,
        SUBJECT_NOT_FOUND,
        SUBJECT_DUPLICATED,
        PRACTICE_EVENT_ERROR,
        PRACTICE_EVENT_NOT_FOUND,
        PRACTICE_EVENT_ILLEGAL_CHANGE,
        PRACTICE_EVENT_DATE_MISSMATCH,
        PRACTICE_EVENT_NO_SOUND_LENGHT,
        FILE_NOT_FOUND,
        FILE_READ_ERROR,
        FILE_UPLOAD_ERROR,
        MODELS_FILE_DOWNLOAD_FAIL,
        MODELS_FILE_NOT_FOUND,
        UNKNOWN
    };

    for (auto code : codes) {
        EXPECT_EQ(get_rst_code(code), static_cast<unsigned int>(code));
        std::string txt = get_rst_txt(code);
        EXPECT_FALSE(txt.empty());
    }

    // Test default case with cast to invalid enum value
    EXPECT_EQ(get_rst_txt(static_cast<rst_code_e>(999)), "UNKNOWN");
}
