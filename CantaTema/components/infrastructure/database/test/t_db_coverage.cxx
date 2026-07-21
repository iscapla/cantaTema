#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <memory>
#include <vector>
#include <string>

// Mocks
#include "mock_tool_paths.hpp"

// Components under test
#include "database/db_coverage.hpp"
#include "database/db_practice_event.hpp"
#include "database/db_subject.hpp"
#include "database/db_user.hpp"
#include "database/db_connection.hpp"
#include "primitives/subject.hpp"
#include "primitives/user.hpp"
#include "primitives/practice_event.hpp"

using ::testing::Return;
using ::testing::_;

class DBCoverageTest : public ::testing::Test {
protected:
    MockToolPath mockToolPath;
    std::filesystem::path temp_db_dir;

    void SetUp() override {
        g_mockToolPath = &mockToolPath;

        temp_db_dir = std::filesystem::temp_directory_path() / ("canta_tema_test_db_cov_" + std::to_string(std::time(nullptr)));
        std::filesystem::create_directories(temp_db_dir);

        EXPECT_CALL(mockToolPath, get_path_for_database())
            .WillRepeatedly(Return(temp_db_dir));

        DB_Connection::reset_connection();

        DB_User db_user;
        ASSERT_EQ(db_user.user_tables_create(), RST_OK);

        DB_Subject db_subject;
        ASSERT_EQ(db_subject.subject_tables_create(), RST_OK);

        DB_PracticeEvent db_pe;
        ASSERT_EQ(db_pe.practice_event_tables_create(), RST_OK);

        DB_Coverage db_cov;
        ASSERT_EQ(db_cov.create_coverage_tables(), RST_OK);
    }

    void TearDown() override {
        DB_Connection::reset_connection();
        if (std::filesystem::exists(temp_db_dir)) {
            std::filesystem::remove_all(temp_db_dir);
        }
        g_mockToolPath = nullptr;
    }

    unsigned int create_test_user(const std::string& username) {
        DB_User db_user;
        User user(username);
        user.set_passwordkey("hashed_secret");
        user.set_roleid(1);
        user.set_creationdate(1234567890);
        user.set_status(User::Account_status::ACTIVE);
        
        if (db_user.add_new_user(user) == RST_OK) {
            return user.get_useraccountid();
        }
        return 0;
    }

    unsigned int create_test_subject(unsigned int user_id, const std::string& name) {
        DB_Subject db_subject;
        Subject subject(0, name);
        subject.set_user_id(user_id);
        
        if (db_subject.add_new_subject(subject) == RST_OK) {
            return subject.get_id();
        }
        return 0;
    }

    unsigned int create_test_practice_event(unsigned int user_id, unsigned int subject_id) {
        DB_PracticeEvent db_pe;
        PracticeEvent event;
        event.set_status(PracticeEvent::RECORDED);
        event.set_user_id(user_id);
        event.set_subject_id(subject_id);
        event.set_date(100000);
        event.set_recorded_date(100050);
        event.set_duration(120);
        event.set_description("Practice Session");
        event.set_filepath("/path/to/practice.opus");

        if (db_pe.add_new_practice_event(event) == RST_OK) {
            return event.get_id();
        }
        return 0;
    }
};

TEST_F(DBCoverageTest, SaveAndRetrieveCoverageExecution) {
    unsigned int user_id = create_test_user("cov_user_1");
    unsigned int subject_id = create_test_subject(user_id, "cov_subject_1");
    unsigned int practice_id = create_test_practice_event(user_id, subject_id);

    DB_Coverage db_cov;
    
    std::string exec_id = "exec_uuid_123456";
    std::string config_json = "{\"similarity_threshold\": 0.75}";
    std::string report_json = "{\"overall_coverage\": 85.5}";

    rst_code_e result = db_cov.save_coverage_analysis_execution(
        practice_id, exec_id, 85.5, 90.0, 95.0, 5.0,
        "small", "multilingual-e5-large", "es", 0.75,
        config_json, report_json
    );

    EXPECT_EQ(result, RST_OK);

    // Get executions list
    std::string json_list;
    rst_code_e list_res = db_cov.get_analysis_executions_for_practice(practice_id, json_list);
    EXPECT_EQ(list_res, RST_OK);
    EXPECT_NE(json_list.find(exec_id), std::string::npos);

    // Get details
    std::string details_report;
    std::string details_config;
    rst_code_e details_res = db_cov.get_analysis_execution_details(exec_id, details_report, details_config);
    EXPECT_EQ(details_res, RST_OK);
    EXPECT_EQ(details_report, report_json);
    EXPECT_EQ(details_config, config_json);
}

TEST_F(DBCoverageTest, DetailsNotFound) {
    DB_Coverage db_cov;
    std::string details_report;
    std::string details_config;
    rst_code_e details_res = db_cov.get_analysis_execution_details("non_existent_exec", details_report, details_config);
    EXPECT_EQ(details_res, DB_NOT_FOUND);
}

TEST_F(DBCoverageTest, DBCloseDatabaseConnectionErrors) {
    DB_Coverage db_cov;
    sqlite3_close(DB_Connection::getConn().get());

    EXPECT_EQ(db_cov.create_coverage_tables(), DB_FAIL);
    EXPECT_EQ(db_cov.save_coverage_analysis_execution(1, "x", 0, 0, 0, 0, "", "", "", 0, "", ""), DB_FAIL);
    
    std::string tmp;
    EXPECT_EQ(db_cov.get_analysis_executions_for_practice(1, tmp), DB_FAIL);
    
    std::string tmp2;
    EXPECT_EQ(db_cov.get_analysis_execution_details("x", tmp, tmp2), DB_FAIL);

    DB_Connection::reset_connection();
    db_cov.create_coverage_tables();
}
