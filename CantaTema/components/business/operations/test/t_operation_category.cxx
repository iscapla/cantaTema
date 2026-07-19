#include <gtest/gtest.h>
#include <memory>
#include <vector>

#include "operations/operation_category.hpp"
#include "operations/operation_user.hpp"
#include "operations/operation_user_metrics.hpp"
#include "primitives/category.hpp"
#include "primitives/user.hpp"

class OperationCategoryTest : public ::testing::Test {
protected:
    std::unique_ptr<OperationCategory> operation_category;
    std::unique_ptr<OperationUser> operation_user;
    std::shared_ptr<const User> test_user;

    void SetUp() override {
        auto metrics = std::make_shared<OperationUserMetrics>();
        operation_user = std::make_unique<OperationUser>(std::move(metrics));

        std::string username = "CategoryTestUser";
        std::string password = "password";

        if (operation_user->user_identify(username, password) == RST_OK) {
            operation_user->user_remove();
        }

        ASSERT_EQ(operation_user->user_add(username, password), RST_OK);
        ASSERT_EQ(operation_user->user_identify(username, password), RST_OK);
        ASSERT_EQ(operation_user->user_get(test_user), RST_OK);

        operation_category = std::make_unique<OperationCategory>();
    }

    void TearDown() override {
        operation_category.reset();
        if (operation_user) {
            operation_user->user_remove();
            operation_user.reset();
        }
    }
};

TEST_F(OperationCategoryTest, AddAndRetrieveAllCategories) {
    Category category(0, "Test Category");
    // Add a category
    operation_category->category_add(test_user, category);

    // Retrieve all categories for the user
    std::vector<std::shared_ptr<Category>> categories;
    operation_category->category_get_all_by_user(test_user, categories);

    EXPECT_FALSE(categories.empty());
}

TEST_F(OperationCategoryTest, GetCategoryById) {
    Category category(0, "Test Category");
    operation_category->category_add(test_user, category);

    std::vector<std::shared_ptr<Category>> categories;
    operation_category->category_get_all_by_user(test_user, categories);
    ASSERT_FALSE(categories.empty());

    // Retrieve the ID from the added category
    unsigned int id = categories.front()->get_id();

    std::shared_ptr<Category> fetched_category;
    operation_category->category_get_by_id(id, fetched_category);

    ASSERT_NE(fetched_category, nullptr);
    EXPECT_EQ(fetched_category->get_id(), id);
}

TEST_F(OperationCategoryTest, UpdateCategory) {
    Category category(0, "Original Name");
    operation_category->category_add(test_user, category);

    std::vector<std::shared_ptr<Category>> categories;
    operation_category->category_get_all_by_user(test_user, categories);
    ASSERT_FALSE(categories.empty());

    auto cat_to_update = categories.front();
    cat_to_update->set_name("Updated Name");
    operation_category->category_update(test_user, *cat_to_update);
    
    // Verification would typically involve checking fields, but we check for no crash/error here.
    std::shared_ptr<Category> fetched_category;
    operation_category->category_get_by_id(cat_to_update->get_id(), fetched_category);
    ASSERT_NE(fetched_category, nullptr);
    EXPECT_EQ(fetched_category->get_name(), "Updated Name");
}

TEST_F(OperationCategoryTest, RemoveCategory) {
    Category category(0, "To Remove");
    operation_category->category_add(test_user, category);

    std::vector<std::shared_ptr<Category>> categories;
    operation_category->category_get_all_by_user(test_user, categories);
    ASSERT_FALSE(categories.empty());

    unsigned int id = categories.front()->get_id();
    operation_category->category_remove(id);

    // Verify removal
    std::shared_ptr<Category> fetched_category;
    operation_category->category_get_by_id(id, fetched_category);
    
    // Depending on implementation, fetched_category might be null or the call returns an error code
    // If the pointer is null, it confirms removal.
    EXPECT_EQ(fetched_category, nullptr);
}

TEST_F(OperationCategoryTest, EdgeAndErrorCases) {
    // 1. Add null user
    Category cat1(0, "Cat 1");
    EXPECT_EQ(operation_category->category_add(nullptr, cat1), CATEGORY_ERROR);

    // 2. Add duplicate category name
    Category cat2(0, "Duplicate Cat");
    ASSERT_EQ(operation_category->category_add(test_user, cat2), RST_OK);

    Category cat3(0, "Duplicate Cat");
    EXPECT_EQ(operation_category->category_add(test_user, cat3), CATEGORY_DUPLICATED);

    // 3. Update null user
    EXPECT_EQ(operation_category->category_update(nullptr, cat2), CATEGORY_ERROR);

    // 4. Update duplicate name collision
    Category cat4(0, "Other Name");
    ASSERT_EQ(operation_category->category_add(test_user, cat4), RST_OK);

    cat4.set_name("Duplicate Cat"); // collision
    EXPECT_EQ(operation_category->category_update(test_user, cat4), CATEGORY_DUPLICATED);

    // 5. Get all by user with null user
    std::vector<std::shared_ptr<Category>> cats;
    EXPECT_EQ(operation_category->category_get_all_by_user(nullptr, cats), CATEGORY_ERROR);

    // 6. Get by id non-existent
    std::shared_ptr<Category> non_existent;
    EXPECT_EQ(operation_category->category_get_by_id(99999, non_existent), CATEGORY_NOT_FOUND);
}
