#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "operations/operation_user.hpp"
#include "operations/operation_user_metrics.hpp"

class OperationUserTest : public ::testing::Test {
protected:
    std::unique_ptr<OperationUser> operation_user;

    void SetUp() override {
        // Initialize with real metrics instance
        std::shared_ptr<IOperationUserMetrics> metrics = std::make_shared<OperationUserMetrics>();
        operation_user = std::make_unique<OperationUser>(std::move(metrics));
    }

    void TearDown() override {
        // Reset the pointer to ensure destructor is called safely
        operation_user.reset();
    }
};

TEST_F(OperationUserTest, InitialStateNotAuthenticated) {
    EXPECT_FALSE(operation_user->user_is_authenticated());
}

TEST_F(OperationUserTest, AddAndIdentifyUser) {
    std::string username = "test_user_auth";
    std::string password = "password123";

    // Add user
    operation_user->user_add(username, password);

    // Identify (login) user
    operation_user->user_identify(username, password);

    EXPECT_TRUE(operation_user->user_is_authenticated());

    // Cleanup
    operation_user->user_remove();
}

TEST_F(OperationUserTest, IdentifyWithWrongPasswordFails) {
    std::string username = "test_user_fail";
    std::string password = "password123";
    std::string wrong_password = "wrong_password";

    operation_user->user_add(username, password);
    
    // Attempt login with wrong password
    operation_user->user_identify(username, wrong_password);

    EXPECT_FALSE(operation_user->user_is_authenticated());
}

TEST_F(OperationUserTest, UserRemoveDeauthenticates) {
    std::string username = "test_user_remove";
    std::string password = "password123";

    operation_user->user_add(username, password);
    operation_user->user_identify(username, password);
    
    ASSERT_TRUE(operation_user->user_is_authenticated());

    operation_user->user_remove();

    EXPECT_FALSE(operation_user->user_is_authenticated());
}

TEST_F(OperationUserTest, UserGetRetrievesUser) {
    std::string username = "test_user_get";
    std::string password = "password123";

    operation_user->user_add(username, password);
    operation_user->user_identify(username, password);

    std::shared_ptr<const User> user;
    operation_user->user_get(user);

    EXPECT_NE(user, nullptr);

    operation_user->user_remove();
}