#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <memory>
#include <vector>

// Mocks
#include "mock_tool_paths.hpp"

// Components under test
#include "database/db_category.hpp"
#include "database/db_connection.hpp"
#include "database/db_user.hpp"
#include "primitives/category.hpp"
#include "database/db_subject.hpp"
#include "primitives/user.hpp"

using ::testing::Return;
using ::testing::_;

class DBCategoryTest : public ::testing::Test {
protected:
    MockToolPath mockToolPath;
    std::filesystem::path temp_db_dir;

    void SetUp() override {
        // 1. Redirect ToolPath calls to our mock object
        g_mockToolPath = &mockToolPath;

        // 2. Create a unique temporary directory for this test run
        temp_db_dir = std::filesystem::temp_directory_path() / ("canta_tema_test_db_cat_" + std::to_string(std::time(nullptr)));
        std::filesystem::create_directories(temp_db_dir);

        // 3. Configure the mock to return our temp directory
        EXPECT_CALL(mockToolPath, get_path_for_database())
            .WillRepeatedly(Return(temp_db_dir));

        // 4. Ensure a fresh start by resetting any existing connection
        DB_Connection::reset_connection();

        // 5. Initialize tables required.
        // Categories depend on Users (Foreign Key), so we create User table too.
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
};

TEST_F(DBCategoryTest, AddNewCategory_Success) {
    unsigned int user_id = create_test_user("cat_test_user_1");
    ASSERT_NE(user_id, 0);

    DB_Category db_category;
    Category category(0, "Test Category");
    category.set_user_id(user_id);

    // Perform DB operation
    rst_code_e result = db_category.add_new_category(category);

    // Verify results
    EXPECT_EQ(result, RST_OK);
    EXPECT_NE(category.get_id(), 0) << "Category ID should be auto-generated";
    
    // Verify category exists via is_category_already_present
    bool exists = false;
    EXPECT_EQ(db_category.is_category_already_present(user_id, "Test Category", exists), RST_OK);
    EXPECT_TRUE(exists);
}

TEST_F(DBCategoryTest, GetCategoryById_Success) {
    unsigned int user_id = create_test_user("cat_test_user_2");
    DB_Category db_category;
    Category original_cat(0, "Retrieval Category");
    original_cat.set_user_id(user_id);

    ASSERT_EQ(db_category.add_new_category(original_cat), RST_OK);

    // Retrieve
    std::shared_ptr<Category> retrieved_cat;
    rst_code_e result = db_category.get_category_by_id(original_cat.get_id(), retrieved_cat);

    EXPECT_EQ(result, RST_OK);
    ASSERT_NE(retrieved_cat, nullptr);
    EXPECT_EQ(retrieved_cat->get_id(), original_cat.get_id());
    EXPECT_EQ(retrieved_cat->get_name(), "Retrieval Category");
    EXPECT_EQ(retrieved_cat->get_user_id(), user_id);
}

TEST_F(DBCategoryTest, UpdateCategory_Success) {
    unsigned int user_id = create_test_user("cat_test_user_3");
    DB_Category db_category;
    Category category(0, "Update Category");
    category.set_user_id(user_id);

    ASSERT_EQ(db_category.add_new_category(category), RST_OK);

    // Modify fields
    category.set_name("Updated Name");

    // Update
    rst_code_e result = db_category.update_category(category);
    EXPECT_EQ(result, RST_OK);

    // Verify
    std::shared_ptr<Category> retrieved;
    ASSERT_EQ(db_category.get_category_by_id(category.get_id(), retrieved), RST_OK);
    EXPECT_EQ(retrieved->get_name(), "Updated Name");
}

TEST_F(DBCategoryTest, RemoveCategory_Success) {
    unsigned int user_id = create_test_user("cat_test_user_4");
    DB_Category db_category;
    Category category(0, "Delete Category");
    category.set_user_id(user_id);

    ASSERT_EQ(db_category.add_new_category(category), RST_OK);

    // Remove
    rst_code_e result = db_category.remove_category(category.get_id());
    EXPECT_EQ(result, RST_OK);

    // Verify it's gone
    bool exists = true;
    db_category.is_category_already_present(user_id, "Delete Category", exists);
    EXPECT_FALSE(exists);
    
    std::shared_ptr<Category> temp;
    EXPECT_NE(db_category.get_category_by_id(category.get_id(), temp), RST_OK);
}

TEST_F(DBCategoryTest, RemoveAllCategoriesFromUser_Success) {
    unsigned int user_id = create_test_user("cat_test_user_5");
    DB_Category db_category;
    
    Category cat1(0, "Cat 1"); cat1.set_user_id(user_id);
    Category cat2(0, "Cat 2"); cat2.set_user_id(user_id);
    
    ASSERT_EQ(db_category.add_new_category(cat1), RST_OK);
    ASSERT_EQ(db_category.add_new_category(cat2), RST_OK);

    // Remove all
    rst_code_e result = db_category.remove_all_categories_from_user(user_id);
    EXPECT_EQ(result, RST_OK);

    // Verify
    std::vector<std::shared_ptr<Category>> categories;
    EXPECT_EQ(db_category.get_all_categories_by_user(user_id, categories), RST_OK);
    EXPECT_TRUE(categories.empty());
}

TEST_F(DBCategoryTest, GetAllCategoriesByUser_Success) {
    unsigned int user_id = create_test_user("cat_test_user_6");
    DB_Category db_category;
    
    Category cat1(0, "Cat A"); cat1.set_user_id(user_id);
    Category cat2(0, "Cat B"); cat2.set_user_id(user_id);
    
    ASSERT_EQ(db_category.add_new_category(cat1), RST_OK);
    ASSERT_EQ(db_category.add_new_category(cat2), RST_OK);

    // Retrieve
    std::vector<std::shared_ptr<Category>> categories;
    rst_code_e result = db_category.get_all_categories_by_user(user_id, categories);
    
    EXPECT_EQ(result, RST_OK);
    EXPECT_EQ(categories.size(), 2);
}

#include "database/db_main.hpp"

TEST_F(DBCategoryTest, DBMainInitializeAndPurge) {
    // DB_Main is initialized in SetUp. Let's verify we can get the instance.
    DB_Main* db = DB_Main::getInstance();
    EXPECT_NE(db, nullptr);

    // Call purge, which resets connection and re-initializes
    EXPECT_NO_THROW({
        db->purge();
    });
}

TEST_F(DBCategoryTest, DBCategoryErrorsAndConstraints) {
    DB_Category db_category;

    // 1. Get non-existent category by ID should return DB_FAIL
    std::shared_ptr<Category> retrieved;
    EXPECT_EQ(db_category.get_category_by_id(999999, retrieved), DB_FAIL);

    // 2. Checking if non-existent category exists should return RST_OK and exists = false
    bool exists = true;
    EXPECT_EQ(db_category.is_category_already_present(999999, "Non Existent Cat", exists), RST_OK);
    EXPECT_FALSE(exists);
}

TEST_F(DBCategoryTest, DBCloseDatabaseConnectionErrors) {
    DB_Category db_category;
    Category cat(0, "Test Cat");
    cat.set_user_id(1);

    // 1. Close database connection
    sqlite3_close(DB_Connection::getConn().get());

    // 2. All operations should return DB_FAIL
    EXPECT_EQ(db_category.category_tables_create(), DB_FAIL);
    bool exists_flag = false;
    EXPECT_EQ(db_category.is_category_already_present(1, "Test Cat", exists_flag), DB_FAIL);

    EXPECT_EQ(db_category.add_new_category(cat), DB_FAIL);
    EXPECT_EQ(db_category.update_category(cat), DB_FAIL);
    EXPECT_EQ(db_category.remove_category(1), DB_FAIL);
    EXPECT_EQ(db_category.remove_all_categories_from_user(1), DB_FAIL);
    std::shared_ptr<Category> retrieved;
    EXPECT_EQ(db_category.get_category_by_id(1, retrieved), DB_FAIL);

    // 3. Reset connection to restore database state
    DB_Connection::reset_connection();
    // Re-create tables
    db_category.category_tables_create();
}


