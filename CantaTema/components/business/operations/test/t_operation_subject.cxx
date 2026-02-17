#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <fstream>
#include <cstdio>

#include "operations/operation_subject.hpp"
#include "operations/operation_category.hpp"
#include "operations/operation_user.hpp"
#include "operations/operation_user_metrics.hpp"
#include "primitives/subject.hpp"
#include "primitives/category.hpp"
#include "primitives/user.hpp"

class OperationSubjectTest : public ::testing::Test {
protected:
    std::unique_ptr<OperationSubject> operation_subject;
    std::shared_ptr<OperationCategory> operation_category;
    std::unique_ptr<OperationUser> operation_user;
    std::shared_ptr<OperationUserMetrics> operation_metrics;

    std::shared_ptr<const User> test_user;
    std::shared_ptr<Category> test_category;
    const std::string dummy_file_name = "test_audio_subject.txt";

    void SetUp() override {
        // Create dummy file for subjects to simulate source file
        std::ofstream outfile(dummy_file_name);
        outfile << "dummy content";
        outfile.close();

        // Initialize dependencies
        operation_metrics = std::make_shared<OperationUserMetrics>();
        operation_category = std::make_shared<OperationCategory>();

        // Initialize OperationUser
        // Create a shared_ptr copy to move into the constructor (casting to interface)
        std::shared_ptr<IOperationUserMetrics> metrics_for_user = operation_metrics;
        operation_user = std::make_unique<OperationUser>(std::move(metrics_for_user));

        // Initialize OperationSubject
        std::shared_ptr<IOperationUserMetrics> metrics_for_subject = operation_metrics;
        std::shared_ptr<IOperationCategory> category_for_subject = operation_category;
        
        operation_subject = std::make_unique<OperationSubject>(
            std::move(metrics_for_subject), 
            std::move(category_for_subject)
        );

        // Create and Identify User
        std::string username = "SubjectTestUser";
        std::string password = "password";

        if (operation_user->user_identify(username, password) == RST_OK) {
            operation_user->user_remove();
        }

        ASSERT_EQ(operation_user->user_add(username, password), RST_OK);
        ASSERT_EQ(operation_user->user_identify(username, password), RST_OK);
        ASSERT_EQ(operation_user->user_get(test_user), RST_OK);

        // Create Category
        Category cat(0, "Subject Category");
        ASSERT_EQ(operation_category->category_add(test_user, cat), RST_OK);
        
        std::vector<std::shared_ptr<Category>> categories;
        operation_category->category_get_all_by_user(test_user, categories);
        ASSERT_FALSE(categories.empty());
        test_category = categories.front();
    }

    void TearDown() override {
        // Clean up dummy file
        std::remove(dummy_file_name.c_str());
        
        operation_subject.reset();
        operation_category.reset();
        if (operation_user) {
            operation_user->user_remove();
            operation_user.reset();
        }
        operation_metrics.reset();
    }
};

TEST_F(OperationSubjectTest, AddAndGetSubject) {

    Subject subject(0, "Test Subject");
    subject.set_category_id(test_category->get_id());
    
    ASSERT_EQ(operation_subject->subject_add(test_user, dummy_file_name, subject), RST_OK);
    EXPECT_GT(subject.get_id(), 0u);

    std::shared_ptr<Subject> fetched;
    EXPECT_EQ(operation_subject->subject_get_by_id(test_user, subject.get_id(), fetched), RST_OK);
    ASSERT_NE(fetched, nullptr);
    EXPECT_EQ(fetched->get_name(), "Test Subject");
    EXPECT_EQ(fetched->get_category_id(), test_category->get_id());
}

TEST_F(OperationSubjectTest, UpdateSubject) {
    Subject subject(0, "Original Subject");
    subject.set_category_id(test_category->get_id());
    ASSERT_EQ(operation_subject->subject_add(test_user, dummy_file_name, subject), RST_OK);

    // Fetch the subject to ensure we have the latest state (including any system-generated fields)
    std::shared_ptr<Subject> subject_to_update;
    ASSERT_EQ(operation_subject->subject_get_by_id(test_user, subject.get_id(), subject_to_update), RST_OK);
    ASSERT_NE(subject_to_update, nullptr);

    subject_to_update->set_name("Updated Subject");
    EXPECT_EQ(operation_subject->subject_update(test_user, *subject_to_update), RST_OK);

    std::shared_ptr<Subject> fetched;
    operation_subject->subject_get_by_id(test_user, subject.get_id(), fetched);
    ASSERT_NE(fetched, nullptr);
    EXPECT_EQ(fetched->get_name(), "Updated Subject");
}

TEST_F(OperationSubjectTest, RemoveSubject) {
    Subject subject(0, "To Remove");
    subject.set_category_id(test_category->get_id());
    ASSERT_EQ(operation_subject->subject_add(test_user, dummy_file_name, subject), RST_OK);

    EXPECT_EQ(operation_subject->subject_remove(test_user, subject.get_id()), RST_OK);

    std::shared_ptr<Subject> fetched;
    operation_subject->subject_get_by_id(test_user, subject.get_id(), fetched);
    EXPECT_EQ(fetched, nullptr);
}

TEST_F(OperationSubjectTest, GetAllByCategory) {
    Subject s1(0, "S1");
    s1.set_category_id(test_category->get_id());
    Subject s2(0, "S2");
    s2.set_category_id(test_category->get_id());
    
    ASSERT_EQ(operation_subject->subject_add(test_user, dummy_file_name, s1), RST_OK);
    ASSERT_EQ(operation_subject->subject_add(test_user, dummy_file_name, s2), RST_OK);

    std::vector<std::shared_ptr<Subject>> subjects;
    EXPECT_EQ(operation_subject->subject_get_all_by_category(test_user, test_category->get_id(), subjects), RST_OK);
    
    EXPECT_GE(subjects.size(), 2u);
}