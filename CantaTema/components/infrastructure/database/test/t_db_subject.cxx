#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <memory>
#include <vector>

// Mocks
#include "mock_tool_paths.hpp"

// Components under test
#include "database/db_subject.hpp"
#include "database/db_category.hpp"
#include "database/db_user.hpp"
#include "database/db_connection.hpp"
#include "primitives/subject.hpp"
#include "primitives/category.hpp"
#include "primitives/user.hpp"

using ::testing::Return;
using ::testing::_;

class DBSubjectTest : public ::testing::Test {
protected:
    MockToolPath mockToolPath;
    std::filesystem::path temp_db_dir;

    void SetUp() override {
        // 1. Redirect ToolPath calls to our mock object
        g_mockToolPath = &mockToolPath;

        // 2. Create a unique temporary directory for this test run
        temp_db_dir = std::filesystem::temp_directory_path() / ("canta_tema_test_db_sub_" + std::to_string(std::time(nullptr)));
        std::filesystem::create_directories(temp_db_dir);

        // 3. Configure the mock to return our temp directory
        EXPECT_CALL(mockToolPath, get_path_for_database())
            .WillRepeatedly(Return(temp_db_dir));

        // 4. Ensure a fresh start by resetting any existing connection
        DB_Connection::reset_connection();

        // 5. Initialize tables required.
        // Subjects depend on Categories, which depend on Users.
        DB_User db_user;
        ASSERT_EQ(db_user.user_tables_create(), RST_OK);

        DB_Category db_category;
        ASSERT_EQ(db_category.category_tables_create(), RST_OK);

        DB_Subject db_subject;
        ASSERT_EQ(db_subject.subject_tables_create(), RST_OK);
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

    // Helper to create a category and return its ID
    unsigned int create_test_category(unsigned int user_id, const std::string& name) {
        DB_Category db_category;
        Category category(0, name);
        category.set_user_id(user_id);
        
        if (db_category.add_new_category(category) == RST_OK) {
            return category.get_id();
        }
        return 0;
    }
};

TEST_F(DBSubjectTest, AddNewSubject_Success) {
    unsigned int user_id = create_test_user("sub_test_user_1");
    ASSERT_NE(user_id, 0);
    unsigned int cat_id = create_test_category(user_id, "sub_test_cat_1");
    ASSERT_NE(cat_id, 0);

    DB_Subject db_subject;
    Subject subject(0, "Test Subject");
    subject.set_user_id(user_id);
    subject.set_category_id(cat_id);

    // Perform DB operation
    rst_code_e result = db_subject.add_new_subject(subject);

    // Verify results
    EXPECT_EQ(result, RST_OK);
    EXPECT_NE(subject.get_id(), 0) << "Subject ID should be auto-generated";
    EXPECT_EQ(subject.get_language(), "es");
}

TEST_F(DBSubjectTest, GetSubjectById_Success) {
    unsigned int user_id = create_test_user("sub_test_user_2");
    unsigned int cat_id = create_test_category(user_id, "sub_test_cat_2");
    
    DB_Subject db_subject;
    Subject original_sub(0, "Retrieval Subject");
    original_sub.set_user_id(user_id);
    original_sub.set_category_id(cat_id);
    original_sub.set_language("en");

    ASSERT_EQ(db_subject.add_new_subject(original_sub), RST_OK);

    // Retrieve
    std::shared_ptr<Subject> retrieved_sub;
    rst_code_e result = db_subject.get_subject_by_id(original_sub.get_id(), retrieved_sub);

    EXPECT_EQ(result, RST_OK);
    ASSERT_NE(retrieved_sub, nullptr);
    EXPECT_EQ(retrieved_sub->get_id(), original_sub.get_id());
    EXPECT_EQ(retrieved_sub->get_name(), "Retrieval Subject");
    EXPECT_EQ(retrieved_sub->get_category_id(), cat_id);
    EXPECT_EQ(retrieved_sub->get_language(), "en");
}

TEST_F(DBSubjectTest, UpdateSubject_Success) {
    unsigned int user_id = create_test_user("sub_test_user_3");
    unsigned int cat_id = create_test_category(user_id, "sub_test_cat_3");

    DB_Subject db_subject;
    Subject subject(0, "Update Subject");
    subject.set_user_id(user_id);
    subject.set_category_id(cat_id);

    ASSERT_EQ(db_subject.add_new_subject(subject), RST_OK);

    // Modify fields
    subject.set_name("Updated Subject Name");
    subject.set_language("fr");

    // Update
    rst_code_e result = db_subject.update_subject(subject);
    EXPECT_EQ(result, RST_OK);

    // Verify
    std::shared_ptr<Subject> retrieved;
    ASSERT_EQ(db_subject.get_subject_by_id(subject.get_id(), retrieved), RST_OK);
    EXPECT_EQ(retrieved->get_name(), "Updated Subject Name");
    EXPECT_EQ(retrieved->get_language(), "fr");
}

TEST_F(DBSubjectTest, RemoveSubject_Success) {
    unsigned int user_id = create_test_user("sub_test_user_4");
    unsigned int cat_id = create_test_category(user_id, "sub_test_cat_4");

    DB_Subject db_subject;
    Subject subject(0, "Delete Subject");
    subject.set_user_id(user_id);
    subject.set_category_id(cat_id);

    ASSERT_EQ(db_subject.add_new_subject(subject), RST_OK);

    // Remove
    rst_code_e result = db_subject.remove_subject(subject.get_id());
    EXPECT_EQ(result, RST_OK);

    // Verify it's gone
    std::shared_ptr<Subject> temp;
    EXPECT_NE(db_subject.get_subject_by_id(subject.get_id(), temp), RST_OK);
}

TEST_F(DBSubjectTest, RemoveAllSubjectsFromUser_Success) {
    unsigned int user_id = create_test_user("sub_test_user_5");
    unsigned int cat_id = create_test_category(user_id, "sub_test_cat_5");

    DB_Subject db_subject;
    
    Subject sub1(0, "Sub 1");
    sub1.set_user_id(user_id);
    sub1.set_category_id(cat_id);
    Subject sub2(0, "Sub 2");
    sub2.set_user_id(user_id);
    sub2.set_category_id(cat_id);
    
    ASSERT_EQ(db_subject.add_new_subject(sub1), RST_OK);
    ASSERT_EQ(db_subject.add_new_subject(sub2), RST_OK);

    // Remove all
    rst_code_e result = db_subject.remove_all_subjects_from_user(user_id);
    EXPECT_EQ(result, RST_OK);

    // Verify
    std::vector<std::shared_ptr<Subject>> subjects;
    EXPECT_EQ(db_subject.get_all_subjects_by_user(user_id, subjects), RST_OK);
    EXPECT_TRUE(subjects.empty());
}

TEST_F(DBSubjectTest, GetAllSubjectsByCategory_Success) {
    unsigned int user_id = create_test_user("sub_test_user_6");
    unsigned int cat_id = create_test_category(user_id, "sub_test_cat_6");

    DB_Subject db_subject;
    
    Subject sub1(0, "Sub A");
    sub1.set_user_id(user_id);
    sub1.set_category_id(cat_id);
    Subject sub2(0, "Sub B");
    sub2.set_user_id(user_id);
    sub2.set_category_id(cat_id);
    
    ASSERT_EQ(db_subject.add_new_subject(sub1), RST_OK);
    ASSERT_EQ(db_subject.add_new_subject(sub2), RST_OK);

    // Retrieve
    std::vector<std::shared_ptr<Subject>> subjects;
    rst_code_e result = db_subject.get_all_subjects_by_category(cat_id, subjects);
    
    EXPECT_EQ(result, RST_OK);
    EXPECT_EQ(subjects.size(), 2);
}

TEST_F(DBSubjectTest, GetAllSubjectsByUser_Success) {
    unsigned int user_id = create_test_user("sub_test_user_7");
    unsigned int cat_id = create_test_category(user_id, "sub_test_cat_7");

    DB_Subject db_subject;
    
    Subject sub1(0, "Sub X");
    sub1.set_user_id(user_id);
    sub1.set_category_id(cat_id);
    Subject sub2(0, "Sub Y");
    sub2.set_user_id(user_id);
    sub2.set_category_id(cat_id);
    
    ASSERT_EQ(db_subject.add_new_subject(sub1), RST_OK);
    ASSERT_EQ(db_subject.add_new_subject(sub2), RST_OK);

    // Retrieve
    std::vector<std::shared_ptr<Subject>> subjects;
    rst_code_e result = db_subject.get_all_subjects_by_user(user_id, subjects);
    
    EXPECT_EQ(result, RST_OK);
    EXPECT_EQ(subjects.size(), 2);
}

TEST_F(DBSubjectTest, DBCloseDatabaseConnectionErrors) {
    DB_Subject db_subject;
    Subject sub(0, "Test Sub");
    sub.set_user_id(1);

    sqlite3_close(DB_Connection::getConn().get());

    EXPECT_EQ(db_subject.subject_tables_create(), DB_FAIL);
    EXPECT_EQ(db_subject.add_new_subject(sub), DB_FAIL);
    EXPECT_EQ(db_subject.update_subject(sub), DB_FAIL);
    EXPECT_EQ(db_subject.remove_subject(1), DB_FAIL);
    EXPECT_EQ(db_subject.remove_all_subjects_from_user(1), DB_FAIL);
    std::shared_ptr<Subject> retrieved;
    EXPECT_EQ(db_subject.get_subject_by_id(1, retrieved), DB_FAIL);
    std::vector<std::shared_ptr<Subject>> subjects;
    EXPECT_EQ(db_subject.get_all_subjects_by_category(1, subjects), RST_OK);
    EXPECT_EQ(db_subject.get_all_subjects_by_user(1, subjects), RST_OK);

    DB_Connection::reset_connection();
    db_subject.subject_tables_create();
}


