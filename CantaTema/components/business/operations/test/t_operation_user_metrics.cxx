#include <gtest/gtest.h>
#include <memory>

#include "operations/operation_user.hpp"
#include "operations/operation_user_metrics.hpp"
#include "primitives/user.hpp"
#include "primitives/user_metrics.hpp"

class OperationUserMetricsTest : public ::testing::Test {
protected:
    std::unique_ptr<OperationUserMetrics> operation_metrics;
    std::unique_ptr<OperationUser> operation_user;
    std::shared_ptr<const User> test_user;

    void SetUp() override {
        // Initialize OperationUser with its metrics dependency to create a valid user
        auto metrics_dep = std::make_shared<OperationUserMetrics>();
        operation_user = std::make_unique<OperationUser>(std::move(metrics_dep));

        // Initialize the unit under test
        operation_metrics = std::make_unique<OperationUserMetrics>();

        std::string username = "MetricsTestUser";
        std::string password = "password";

        // Ensure clean state
        if (operation_user->user_identify(username, password) == RST_OK) {
            operation_user->user_remove();
        }

        // Create and identify user
        ASSERT_EQ(operation_user->user_add(username, password), RST_OK);
        ASSERT_EQ(operation_user->user_identify(username, password), RST_OK);
        ASSERT_EQ(operation_user->user_get(test_user), RST_OK);
    }

    void TearDown() override {
        if (operation_user) {
            operation_user->user_remove();
        }
    }
};

TEST_F(OperationUserMetricsTest, AddAndGetMetrics) {
    UserMetrics metrics(test_user->get_useraccountid());
    // Attempt to get metrics first (OperationUser might have created them automatically)
    std::shared_ptr<UserMetrics> retrieved_metrics;
    rst_code_e result = operation_metrics->user_metrics_get(test_user, retrieved_metrics);

    if (result != RST_OK) {
        // If not present, add them manually
        EXPECT_EQ(operation_metrics->user_metrics_add(test_user, metrics), RST_OK);
        EXPECT_EQ(operation_metrics->user_metrics_get(test_user, retrieved_metrics), RST_OK);
    }
    
    ASSERT_NE(retrieved_metrics, nullptr);
}

TEST_F(OperationUserMetricsTest, UpdateMetrics) {
    std::shared_ptr<UserMetrics> retrieved_metrics;
    // Ensure metrics exist
    if (operation_metrics->user_metrics_get(test_user, retrieved_metrics) != RST_OK) {
        UserMetrics metrics(test_user->get_useraccountid());
        ASSERT_EQ(operation_metrics->user_metrics_add(test_user, metrics), RST_OK);
        operation_metrics->user_metrics_get(test_user, retrieved_metrics);
    }

    ASSERT_NE(retrieved_metrics, nullptr);
    
    // Update
    EXPECT_EQ(operation_metrics->user_metrics_update(test_user, *retrieved_metrics), RST_OK);
}

TEST_F(OperationUserMetricsTest, RemoveMetrics) {
    // Ensure metrics exist
    std::shared_ptr<UserMetrics> retrieved_metrics;
    if (operation_metrics->user_metrics_get(test_user, retrieved_metrics) != RST_OK) {
        UserMetrics metrics(test_user->get_useraccountid());
        ASSERT_EQ(operation_metrics->user_metrics_add(test_user, metrics), RST_OK);
    }

    // Remove
    EXPECT_EQ(operation_metrics->user_metrics_remove(test_user), RST_OK);

    // Verify removal
    EXPECT_NE(operation_metrics->user_metrics_get(test_user, retrieved_metrics), RST_OK);
}

TEST_F(OperationUserMetricsTest, CanAcceptFileSize) {
    // Test with a reasonable file size (e.g., 1MB = 1024KB)
    unsigned int size_kb = 1024;
    EXPECT_EQ(operation_metrics->user_metrics_can_accept_file_size(test_user, size_kb), RST_OK);
}