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

TEST_F(OperationSubjectTest, ValidationAndErrorTests) {
    // Null user cases
    Subject dummy_sub(0, "Dummy");
    dummy_sub.set_category_id(test_category->get_id());
    EXPECT_EQ(operation_subject->subject_add(nullptr, dummy_file_name, dummy_sub), SUBJECT_ERROR);
    EXPECT_EQ(operation_subject->subject_update(nullptr, dummy_sub), SUBJECT_ERROR);
    EXPECT_EQ(operation_subject->subject_remove(nullptr, 1), SUBJECT_ERROR);
    
    std::shared_ptr<Subject> fetched;
    EXPECT_EQ(operation_subject->subject_get_by_id(nullptr, 1, fetched), SUBJECT_ERROR);

    std::vector<std::shared_ptr<Subject>> fetched_list;
    EXPECT_EQ(operation_subject->subject_get_all_by_user(nullptr, fetched_list), SUBJECT_ERROR);
    EXPECT_EQ(operation_subject->subject_get_all_by_category(nullptr, test_category->get_id(), fetched_list), SUBJECT_ERROR);

    // Empty file path throws exception
    EXPECT_THROW(operation_subject->subject_add(test_user, "", dummy_sub), std::runtime_error);

    // Missing file path but valid extension returns error
    EXPECT_EQ(operation_subject->subject_add(test_user, "non_existent_file.pdf", dummy_sub), FILE_NOT_FOUND);

    // Duplicated subject name
    Subject s1(0, "Unique Name");
    s1.set_category_id(test_category->get_id());
    ASSERT_EQ(operation_subject->subject_add(test_user, dummy_file_name, s1), RST_OK);

    Subject s2(0, "Unique Name");
    s2.set_category_id(test_category->get_id());
    EXPECT_EQ(operation_subject->subject_add(test_user, dummy_file_name, s2), SUBJECT_DUPLICATED);

    // Attempting duplicate name on update
    Subject s3(0, "Another Unique Name");
    s3.set_category_id(test_category->get_id());
    ASSERT_EQ(operation_subject->subject_add(test_user, dummy_file_name, s3), RST_OK);

    std::shared_ptr<Subject> s3_fetched;
    ASSERT_EQ(operation_subject->subject_get_by_id(test_user, s3.get_id(), s3_fetched), RST_OK);
    s3_fetched->set_name("Unique Name"); // already exists
    EXPECT_EQ(operation_subject->subject_update(test_user, *s3_fetched), SUBJECT_DUPLICATED);

    // Try to update file path
    std::shared_ptr<Subject> s1_fetched;
    ASSERT_EQ(operation_subject->subject_get_by_id(test_user, s1.get_id(), s1_fetched), RST_OK);
    s1_fetched->set_filepath("some_other_path.pdf");
    EXPECT_EQ(operation_subject->subject_update(test_user, *s1_fetched), SUBJECT_ERROR);

    // Not found cases
    std::shared_ptr<Subject> not_found_sub;
    EXPECT_EQ(operation_subject->subject_get_by_id(test_user, 9999, not_found_sub), SUBJECT_NOT_FOUND);
    EXPECT_EQ(operation_subject->subject_remove(test_user, 9999), SUBJECT_NOT_FOUND);
}

TEST_F(OperationSubjectTest, CategoryAssociationValidation) {
    // 1. Create a category belonging to another user
    std::string user2_name = "SubjectTestUser2";
    std::string user2_pass = "password";
    std::shared_ptr<const User> user2;
    if (operation_user->user_identify(user2_name, user2_pass) == RST_OK) {
        operation_user->user_remove();
    }
    ASSERT_EQ(operation_user->user_add(user2_name, user2_pass), RST_OK);
    ASSERT_EQ(operation_user->user_identify(user2_name, user2_pass), RST_OK);
    ASSERT_EQ(operation_user->user_get(user2), RST_OK);

    Category cat2(0, "User 2 Category");
    ASSERT_EQ(operation_category->category_add(user2, cat2), RST_OK);
    std::vector<std::shared_ptr<Category>> categories;
    operation_category->category_get_all_by_user(user2, categories);
    ASSERT_FALSE(categories.empty());
    unsigned int cat2_id = categories.front()->get_id();

    // Switch back to test_user
    std::string username = "SubjectTestUser";
    std::string password = "password";
    ASSERT_EQ(operation_user->user_identify(username, password), RST_OK);

    // 2. Subject add with category not belonging to test_user
    Subject sub_wrong_cat(0, "Wrong Cat Subject");
    sub_wrong_cat.set_category_id(cat2_id);
    EXPECT_EQ(operation_subject->subject_add(test_user, dummy_file_name, sub_wrong_cat), SUBJECT_ERROR);

    // 3. Subject add with non-existent category
    Subject sub_non_existent_cat(0, "Non Existent Cat");
    sub_non_existent_cat.set_category_id(99999);
    EXPECT_EQ(operation_subject->subject_add(test_user, dummy_file_name, sub_non_existent_cat), SUBJECT_ERROR);

    // 4. Subject update with wrong category or non-existent category
    Subject sub_ok(0, "OK Subject");
    sub_ok.set_category_id(test_category->get_id());
    ASSERT_EQ(operation_subject->subject_add(test_user, dummy_file_name, sub_ok), RST_OK);

    std::shared_ptr<Subject> sub_ok_fetched;
    ASSERT_EQ(operation_subject->subject_get_by_id(test_user, sub_ok.get_id(), sub_ok_fetched), RST_OK);
    
    // Set non-existent category
    sub_ok_fetched->set_category_id(99999);
    EXPECT_EQ(operation_subject->subject_update(test_user, *sub_ok_fetched), SUBJECT_ERROR);

    // Set wrong category
    sub_ok_fetched->set_category_id(cat2_id);
    EXPECT_EQ(operation_subject->subject_update(test_user, *sub_ok_fetched), SUBJECT_ERROR);

    // 5. subject_get_all_by_category wrong category or non-existent category
    std::vector<std::shared_ptr<Subject>> subjects_list;
    EXPECT_EQ(operation_subject->subject_get_all_by_category(test_user, cat2_id, subjects_list), CATEGORY_NOT_FOUND);
    EXPECT_EQ(operation_subject->subject_get_all_by_category(test_user, 99999, subjects_list), CATEGORY_NOT_FOUND);

    // 6. Test null category_op/user_metrics_op constructors
    OperationSubject null_deps(operation_metrics, nullptr);
    Subject sub_null_deps(0, "Null Deps");
    sub_null_deps.set_category_id(test_category->get_id());
    EXPECT_EQ(null_deps.subject_add(test_user, dummy_file_name, sub_null_deps), SUBJECT_ERROR);
    EXPECT_EQ(null_deps.subject_update(test_user, sub_null_deps), SUBJECT_ERROR);


    // Clean up user2
    ASSERT_EQ(operation_user->user_identify(user2_name, user2_pass), RST_OK);
    EXPECT_EQ(operation_user->user_remove(), RST_OK);
}

