#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <memory>

// Mocks
#include "mock_tool_paths.hpp"

// Components under test
#include "database/db_user_metrics.hpp"
#include "database/db_user.hpp"
#include "database/db_category.hpp"
#include "database/db_subject.hpp"
#include "database/db_connection.hpp"
#include "primitives/user.hpp"
#include "primitives/user_metrics.hpp"

using ::testing::Return;
using ::testing::_;

class DBUserMetricsTest : public ::testing::Test {
protected:
    MockToolPath mockToolPath;
    std::filesystem::path temp_db_dir;

    void SetUp() override {
        // 1. Redirect ToolPath calls to our mock object
        g_mockToolPath = &mockToolPath;

        // 2. Create a unique temporary directory for this test run
        temp_db_dir = std::filesystem::temp_directory_path() / ("canta_tema_test_db_metrics_" + std::to_string(std::time(nullptr)));
        std::filesystem::create_directories(temp_db_dir);

        // 3. Configure the mock to return our temp directory
        EXPECT_CALL(mockToolPath, get_path_for_database())
            .WillRepeatedly(Return(temp_db_dir));

        // 4. Ensure a fresh start by resetting any existing connection
        DB_Connection::reset_connection();

        // 5. Initialize tables required.
        // UserMetrics depends on User (Foreign Key).
        DB_User db_user;
        ASSERT_EQ(db_user.user_tables_create(), RST_OK);

        DB_Category db_category;
        ASSERT_EQ(db_category.category_tables_create(), RST_OK);

        DB_Subject db_subject;
        ASSERT_EQ(db_subject.subject_tables_create(), RST_OK);

        DB_UserMetrics db_metrics;
        ASSERT_EQ(db_metrics.user_metrics_tables_create(), RST_OK);

        // Enable foreign keys for this connection to ensure ON DELETE CASCADE works during tests
        std::shared_ptr<sqlite3> db = DB_Connection::getConn();
        sqlite3_exec(db.get(), "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    }

    void TearDown() override {
        // 1. Close the database connection
        DB_Connection::reset_connection();

        // 2. Clean up the physical files
        if (std::filesystem::exists(temp_db_dir)) {
            std::filesystem::remove_all(temp_db_dir);
        }

        // 3. Reset the global mock pointer
        g_mockToolPath = nullptr;
    }

    // Helper to create a user and return its ID
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
};

TEST_F(DBUserMetricsTest, UpdateUserMetrics_Insert_Success) {
    unsigned int user_id = create_test_user("metrics_test_user_1");
    ASSERT_NE(user_id, 0);

    DB_UserMetrics db_metrics;
    UserMetrics metrics(user_id);
    metrics.set_space_used_kb(2048);

    // Perform DB operation (Insert)
    rst_code_e result = db_metrics.update_user_metrics(metrics);

    // Verify results
    EXPECT_EQ(result, RST_OK);
    
    // Verify metrics exist
    auto retrieved = std::make_shared<UserMetrics>(user_id);
    EXPECT_EQ(db_metrics.get_user_metrics(retrieved), RST_OK);
    EXPECT_EQ(retrieved->get_space_used_kb(), 2048);
}

TEST_F(DBUserMetricsTest, UpdateUserMetrics_Update_Success) {
    unsigned int user_id = create_test_user("metrics_test_user_2");
    ASSERT_NE(user_id, 0);

    DB_UserMetrics db_metrics;
    UserMetrics metrics(user_id);
    metrics.set_space_used_kb(1024);
    ASSERT_EQ(db_metrics.update_user_metrics(metrics), RST_OK);

    // Modify fields
    metrics.set_space_used_kb(4096);

    // Update
    rst_code_e result = db_metrics.update_user_metrics(metrics);
    EXPECT_EQ(result, RST_OK);

    // Verify
    auto retrieved = std::make_shared<UserMetrics>(user_id);
    ASSERT_EQ(db_metrics.get_user_metrics(retrieved), RST_OK);
    EXPECT_EQ(retrieved->get_space_used_kb(), 4096);
}

TEST_F(DBUserMetricsTest, RemoveUserMetrics_Success) {
    unsigned int user_id = create_test_user("metrics_test_user_3");
    ASSERT_NE(user_id, 0);

    DB_UserMetrics db_metrics;
    UserMetrics metrics(user_id);
    metrics.set_space_used_kb(512);
    ASSERT_EQ(db_metrics.update_user_metrics(metrics), RST_OK);

    // Remove
    rst_code_e result = db_metrics.remove_user_metrics(user_id);
    EXPECT_EQ(result, RST_OK);

    // Verify it's gone
    auto retrieved = std::make_shared<UserMetrics>(user_id);
    EXPECT_EQ(db_metrics.get_user_metrics(retrieved), DB_NOT_FOUND);
}

TEST_F(DBUserMetricsTest, GetUserMetrics_NotFound) {
    unsigned int user_id = create_test_user("metrics_test_user_4");
    ASSERT_NE(user_id, 0);

    DB_UserMetrics db_metrics;
    auto retrieved = std::make_shared<UserMetrics>(user_id);
    
    // Try to get metrics that haven't been created
    EXPECT_EQ(db_metrics.get_user_metrics(retrieved), DB_NOT_FOUND);
}

TEST_F(DBUserMetricsTest, CascadeDelete_Success) {
    unsigned int user_id = create_test_user("metrics_test_user_5");
    ASSERT_NE(user_id, 0);

    DB_UserMetrics db_metrics;
    UserMetrics metrics(user_id);
    metrics.set_space_used_kb(100);
    ASSERT_EQ(db_metrics.update_user_metrics(metrics), RST_OK);

    // Remove the user
    DB_User db_user;
    ASSERT_EQ(db_user.remove_user("metrics_test_user_5"), RST_OK);

    // Verify metrics are also removed (Cascade)
    auto retrieved = std::make_shared<UserMetrics>(user_id);
    EXPECT_EQ(db_metrics.get_user_metrics(retrieved), DB_NOT_FOUND);
}

TEST_F(DBUserMetricsTest, InvalidParameters) {
    DB_UserMetrics db_metrics;

    // 1. Update user metrics with invalid ID
    UserMetrics metrics(0);
    EXPECT_EQ(db_metrics.update_user_metrics(metrics), DB_BAD_PARAM);

    // 2. Remove user metrics with invalid ID
    EXPECT_EQ(db_metrics.remove_user_metrics(0), DB_BAD_PARAM);

    // 3. Get user metrics with null metrics or invalid ID
    EXPECT_EQ(db_metrics.get_user_metrics(nullptr), DB_BAD_PARAM);
    auto bad_metrics = std::make_shared<UserMetrics>(0);
    EXPECT_EQ(db_metrics.get_user_metrics(bad_metrics), DB_BAD_PARAM);
}

TEST_F(DBUserMetricsTest, DBCloseDatabaseConnectionErrors) {
    DB_UserMetrics db_metrics;
    UserMetrics metrics(1);

    sqlite3_close(DB_Connection::getConn().get());

    EXPECT_EQ(db_metrics.user_metrics_tables_create(), DB_FAIL);
    EXPECT_EQ(db_metrics.update_user_metrics(metrics), DB_FAIL);
    EXPECT_EQ(db_metrics.remove_user_metrics(1), DB_FAIL);
    auto temp = std::make_shared<UserMetrics>(1);
    EXPECT_EQ(db_metrics.get_user_metrics(temp), DB_FAIL);

    DB_Connection::reset_connection();
    db_metrics.user_metrics_tables_create();
}


