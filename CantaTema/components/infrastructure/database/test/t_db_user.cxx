#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <memory>

// Mocks
#include "mock_tool_paths.hpp"

// Components under test
#include "database/db_user.hpp"
#include "database/db_connection.hpp"
#include "database/db_category.hpp"
#include "database/db_subject.hpp"
#include "primitives/user.hpp"

using ::testing::Return;
using ::testing::_;

class DBUserTest : public ::testing::Test {
protected:
    MockToolPath mockToolPath;
    std::filesystem::path temp_db_dir;

    void SetUp() override {
        // 1. Redirect ToolPath calls to our mock object
        g_mockToolPath = &mockToolPath;

        // 2. Create a unique temporary directory for this test run
        temp_db_dir = std::filesystem::temp_directory_path() / ("canta_tema_test_db_" + std::to_string(std::time(nullptr)));
        std::filesystem::create_directories(temp_db_dir);

        // 3. Configure the mock to return our temp directory
        // We use WillRepeatedly because it is called by both the constructor and reset_connection
        EXPECT_CALL(mockToolPath, get_path_for_database())
            .WillRepeatedly(Return(temp_db_dir));

        // 4. Ensure a fresh start by resetting any existing connection
        // This ensures the singleton is destroyed and any previous file is cleaned up using the mock path
        DB_Connection::reset_connection();

        // 5. Initialize tables required for DB_User.
        // Note: DB_User::remove_user executes SQL that refers to 'subjects' and 'categories' tables
        // for cascading deletes. We must ensure they exist to avoid SQLITE_ERROR.
        DB_User db_user;
        ASSERT_EQ(db_user.user_tables_create(), RST_OK);

        DB_Category db_category;
        ASSERT_EQ(db_category.category_tables_create(), RST_OK);

        DB_Subject db_subject;
        ASSERT_EQ(db_subject.subject_tables_create(), RST_OK);
    }

    void TearDown() override {
        // 1. Close the database connection
        // This calls sqlite3_close and deletes the file at the mocked path
        DB_Connection::reset_connection();

        // 2. Clean up the physical files
        if (std::filesystem::exists(temp_db_dir)) {
            std::filesystem::remove_all(temp_db_dir);
        }

        // 3. Reset the global mock pointer
        g_mockToolPath = nullptr;
    }
};

TEST_F(DBUserTest, AddNewUser_Success) {
    DB_User db_user;
    User user("test_user");
    user.set_passwordkey("hashed_secret");
    user.set_roleid(1);
    user.set_creationdate(1234567890);
    user.set_status(User::Account_status::ACTIVE);
    user.set_workemail("test@example.com");

    // Perform DB operation
    rst_code_e result = db_user.add_new_user(user);

    // Verify results
    EXPECT_EQ(result, RST_OK);
    EXPECT_NE(user.get_useraccountid(), 0) << "User ID should be auto-generated";
    
    // Verify user exists via is_user_already_present
    bool exists = false;
    EXPECT_EQ(db_user.is_user_already_present("test_user", exists), RST_OK);
    EXPECT_TRUE(exists);
}

TEST_F(DBUserTest, GetUser_Success) {
    DB_User db_user;
    User original_user("retrieval_user");
    original_user.set_passwordkey("key123");
    original_user.set_roleid(2);
    original_user.set_creationdate(11111);
    original_user.set_status(User::Account_status::ACTIVE);

    ASSERT_EQ(db_user.add_new_user(original_user), RST_OK);

    // Retrieve
    User retrieved_user("retrieval_user");
    rst_code_e result = db_user.get_user(retrieved_user);

    EXPECT_EQ(result, RST_OK);
    EXPECT_EQ(retrieved_user.get_useraccountid(), original_user.get_useraccountid());
    EXPECT_EQ(retrieved_user.get_passwordkey(), "key123");
    EXPECT_EQ(retrieved_user.get_roleid(), 2);
}

TEST_F(DBUserTest, UpdateUser_Success) {
    DB_User db_user;
    User user("update_user");
    user.set_passwordkey("old_key");
    user.set_roleid(1);
    user.set_creationdate(100);
    user.set_status(User::Account_status::ACTIVE);

    ASSERT_EQ(db_user.add_new_user(user), RST_OK);

    // Modify fields
    user.set_passwordkey("new_key");
    user.set_firstname("UpdatedName");

    // Update
    rst_code_e result = db_user.update_user(user);
    EXPECT_EQ(result, RST_OK);

    // Verify
    User retrieved("update_user");
    ASSERT_EQ(db_user.get_user(retrieved), RST_OK);
    EXPECT_EQ(retrieved.get_passwordkey(), "new_key");
    EXPECT_EQ(retrieved.get_firstname(), "UpdatedName");
}

TEST_F(DBUserTest, RemoveUser_Success) {
    DB_User db_user;
    User user("delete_me");
    user.set_passwordkey("key");
    user.set_roleid(1);
    user.set_creationdate(1);
    user.set_status(User::Account_status::ACTIVE);

    ASSERT_EQ(db_user.add_new_user(user), RST_OK);

    // Remove
    rst_code_e result = db_user.remove_user("delete_me");
    EXPECT_EQ(result, RST_OK);

    // Verify it's gone
    bool exists = true;
    db_user.is_user_already_present("delete_me", exists);
    EXPECT_FALSE(exists);
    
    User temp("delete_me");
    EXPECT_EQ(db_user.get_user(temp), USER_NOT_FOUND);
}

TEST_F(DBUserTest, InvalidParams) {
    DB_User db_user;
    
    // empty name
    bool exists = false;
    EXPECT_EQ(db_user.is_user_already_present("", exists), DB_BAD_PARAM);

    User empty_name_user("");
    EXPECT_EQ(db_user.add_new_user(empty_name_user), DB_BAD_PARAM);
    EXPECT_EQ(db_user.update_user(empty_name_user), DB_BAD_PARAM);
    EXPECT_EQ(db_user.get_user(empty_name_user), DB_BAD_PARAM);
    EXPECT_EQ(db_user.remove_user(""), DB_BAD_PARAM);
}

TEST_F(DBUserTest, ExtraCoverageTests) {
    DB_User db_user;

    // 1. Get user by ID validations
    User dummy("id_test_user");
    EXPECT_EQ(db_user.get_user_by_id(0, dummy), DB_BAD_PARAM);
    EXPECT_EQ(db_user.get_user_by_id(99999, dummy), USER_NOT_FOUND);

    // 2. Full field binding and fetching checks
    User user("full_field_user");
    user.set_passwordkey("pass");
    user.set_passwordsalt("salt");
    user.set_resettoken("token");
    user.set_resetexpiration(123456);
    user.set_status(User::Account_status::LOCKED);
    user.set_creationdate(7890);
    user.set_locknotes("lock notes text");
    user.set_workemail("work@email.com");
    user.set_recoveryemail("recovery@email.com");
    user.set_firstname("First");
    user.set_lastname("Last");
    user.set_roleid(3);
    user.set_max_space_size_in_kb(5000);

    ASSERT_EQ(db_user.add_new_user(user), RST_OK);
    unsigned int generated_id = user.get_useraccountid();
    EXPECT_NE(generated_id, 0u);

    // Retrieve via ID
    User retrieved("");
    ASSERT_EQ(db_user.get_user_by_id(generated_id, retrieved), RST_OK);
    EXPECT_EQ(retrieved.get_name(), "full_field_user");
    EXPECT_EQ(retrieved.get_passwordsalt(), "salt");
    EXPECT_EQ(retrieved.get_resettoken(), "token");
    EXPECT_EQ(retrieved.get_resetexpiration(), 123456);
    EXPECT_EQ(retrieved.get_status(), User::Account_status::LOCKED);

    EXPECT_EQ(retrieved.get_creationdate(), 7890);
    EXPECT_EQ(retrieved.get_locknotes(), "lock notes text");
    EXPECT_EQ(retrieved.get_workemail(), "work@email.com");
    EXPECT_EQ(retrieved.get_recoveryemail(), "recovery@email.com");
    EXPECT_EQ(retrieved.get_firstname(), "First");
    EXPECT_EQ(retrieved.get_lastname(), "Last");
    EXPECT_EQ(retrieved.get_roleid(), 3);
    EXPECT_EQ(retrieved.get_max_space_size_in_kb(), 5000);
}

TEST_F(DBUserTest, DBCloseDatabaseConnectionErrors) {
    DB_User db_user;
    User u("test_user_db_err");

    sqlite3_close(DB_Connection::getConn().get());

    EXPECT_EQ(db_user.user_tables_create(), DB_FAIL);
    EXPECT_EQ(db_user.add_new_user(u), DB_FAIL);
    EXPECT_EQ(db_user.update_user(u), DB_FAIL);
    EXPECT_EQ(db_user.remove_user("test_user_db_err"), DB_FAIL);
    User retrieved("test_user_db_err");
    EXPECT_EQ(db_user.get_user_by_id(1, retrieved), DB_FAIL);
    EXPECT_EQ(db_user.get_user(retrieved), DB_FAIL);


    DB_Connection::reset_connection();
    db_user.user_tables_create();
}

