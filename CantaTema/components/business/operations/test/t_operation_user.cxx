#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "operations/operation_user.hpp"
#include "operations/operation_user_metrics.hpp"

class FailureUserMetricsMock : public IOperationUserMetrics {
public:
    rst_code_e user_metrics_add(const std::shared_ptr<const User> &user, UserMetrics &metrics) override {
        return USER_METRICS_ERROR;
    }
    rst_code_e user_metrics_update(const std::shared_ptr<const User> &user, UserMetrics &metrics) override { return RST_OK; }
    rst_code_e user_metrics_remove(const std::shared_ptr<const User> &user) override { return RST_OK; }
    rst_code_e user_metrics_get(const std::shared_ptr<const User> &user, std::shared_ptr<UserMetrics> &metrics) override { return RST_OK; }
    rst_code_e user_metrics_can_accept_file_size(const std::shared_ptr<const User> &user, unsigned int size_in_kb) override { return RST_OK; }
};


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

TEST_F(OperationUserTest, ErrorAndBoundaryCases) {
    // 1. Double/Duplicate user creation
    std::string name = "double_creator";
    std::string pass = "123456";
    
    // clean up first just in case
    std::shared_ptr<const User> existing;
    if (operation_user->user_get_by_name(name, existing) == RST_OK) {
        operation_user->user_identify(name, pass);
        operation_user->user_remove();
    }

    ASSERT_EQ(operation_user->user_add(name, pass), RST_OK);
    EXPECT_EQ(operation_user->user_add(name, pass), USER_DUPLICATED);

    // 2. Identify non-existent user
    EXPECT_EQ(operation_user->user_identify("not_a_real_user_name_123", pass), USER_NOT_FOUND);

    // 3. User get when not authenticated
    std::shared_ptr<const User> local_got;
    EXPECT_EQ(operation_user->user_get(local_got), USER_NO_AUTH);

    // 4. Remove user when not authenticated
    EXPECT_EQ(operation_user->user_remove(), USER_NO_AUTH);

    // 5. Get user by name not found
    std::shared_ptr<const User> non_existent;
    EXPECT_EQ(operation_user->user_get_by_name("non_existent_username", non_existent), USER_NOT_FOUND);

    // 6. User update operation
    ASSERT_EQ(operation_user->user_identify(name, pass), RST_OK);
    std::shared_ptr<const User> authenticated_user;
    ASSERT_EQ(operation_user->user_get(authenticated_user), RST_OK);
    
    auto modifiable = std::const_pointer_cast<User>(authenticated_user);
    modifiable->set_firstname("UpdatedFirst");
    EXPECT_EQ(operation_user->user_update(authenticated_user), RST_OK);

    // Clean up
    EXPECT_EQ(operation_user->user_remove(), RST_OK);
}

TEST_F(OperationUserTest, AddUserMetricsFailureRollback) {
    std::shared_ptr<IOperationUserMetrics> bad_metrics = std::make_shared<FailureUserMetricsMock>();
    OperationUser bad_op_user(std::move(bad_metrics));
    EXPECT_EQ(bad_op_user.user_add("rollback_user", "password"), USER_ERROR);
}

TEST_F(OperationUserTest, SaveAndGetUserConfiguration) {
    std::string name = "cfg_user";
    std::string pass = "123456";

    // Clean up if left over
    std::shared_ptr<const User> existing;
    if (operation_user->user_get_by_name(name, existing) == RST_OK) {
        operation_user->user_identify(name, pass);
        operation_user->user_remove();
    }

    ASSERT_EQ(operation_user->user_add(name, pass), RST_OK);
    ASSERT_EQ(operation_user->user_identify(name, pass), RST_OK);

    std::shared_ptr<const User> auth_user;
    ASSERT_EQ(operation_user->user_get(auth_user), RST_OK);
    unsigned int user_id = auth_user->get_useraccountid();

    UserConfiguration cfg_to_save;
    cfg_to_save.whisper.model_name = "medium";
    cfg_to_save.comparison.similarity_threshold = 0.82f;
    cfg_to_save.reference_extraction.importance_weight_bold = 3.5f;

    EXPECT_EQ(operation_user->save_user_configuration(user_id, cfg_to_save), RST_OK);

    UserConfiguration cfg_loaded;
    EXPECT_EQ(operation_user->get_user_configuration(user_id, cfg_loaded), RST_OK);
    EXPECT_EQ(cfg_loaded.whisper.model_name, "medium");
    EXPECT_FLOAT_EQ(cfg_loaded.comparison.similarity_threshold, 0.82f);
    EXPECT_FLOAT_EQ(cfg_loaded.reference_extraction.importance_weight_bold, 3.5f);

    EXPECT_EQ(operation_user->user_remove(), RST_OK);
}
