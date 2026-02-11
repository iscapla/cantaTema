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