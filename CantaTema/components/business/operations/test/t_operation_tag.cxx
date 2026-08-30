/**
 * @file t_operation_tag.cxx
 * @brief Unit tests for OperationTag business logic.
 */

#include <gtest/gtest.h>
#include <memory>
#include <vector>

#include "operations/operation_tag.hpp"
#include "operations/operation_user.hpp"
#include "operations/operation_user_metrics.hpp"
#include "database/db_user.hpp"
#include "database/db_subject.hpp"
#include "database/db_main.hpp"
#include "primitives/user.hpp"
#include "primitives/subject.hpp"
#include "primitives/tag.hpp"

class OperationTagTest : public ::testing::Test {
protected:
    std::unique_ptr<OperationTag> operation_tag;
    std::unique_ptr<OperationUser> operation_user;
    std::shared_ptr<const User> test_user1;
    std::shared_ptr<const User> test_user2;

    void SetUp() override {
        DB_Main::getInstance()->purge();

        auto metrics1 = std::make_shared<OperationUserMetrics>();
        operation_user = std::make_unique<OperationUser>(std::move(metrics1));

        std::string u1 = "TagUser1";
        std::string p1 = "pass1";
        ASSERT_EQ(operation_user->user_add(u1, p1), RST_OK);
        ASSERT_EQ(operation_user->user_identify(u1, p1), RST_OK);
        ASSERT_EQ(operation_user->user_get(test_user1), RST_OK);

        std::string u2 = "TagUser2";
        std::string p2 = "pass2";
        ASSERT_EQ(operation_user->user_add(u2, p2), RST_OK);
        ASSERT_EQ(operation_user->user_identify(u2, p2), RST_OK);
        ASSERT_EQ(operation_user->user_get(test_user2), RST_OK);

        operation_tag = std::make_unique<OperationTag>();
    }

    void TearDown() override {
        operation_tag.reset();
        if (operation_user) {
            operation_user.reset();
        }
    }
};

TEST_F(OperationTagTest, TagCrudAndAuth)
{
    // Test with null / unauthenticated user
    Tag tag(0, "penal");
    EXPECT_EQ(operation_tag->tag_add(nullptr, tag), TAG_ERROR);

    auto unauth_user = std::make_shared<User>("Nobody");
    EXPECT_EQ(operation_tag->tag_add(unauth_user, tag), TAG_ERROR);

    // Other operations with null user
    EXPECT_EQ(operation_tag->tag_update(nullptr, tag), TAG_ERROR);
    EXPECT_EQ(operation_tag->tag_remove(nullptr, 1), TAG_ERROR);
    std::shared_ptr<Tag> t_dummy;
    EXPECT_EQ(operation_tag->tag_get_by_id(nullptr, 1, t_dummy), TAG_ERROR);
    EXPECT_EQ(operation_tag->tag_get_by_name(nullptr, "penal", t_dummy), TAG_ERROR);
    std::vector<std::shared_ptr<Tag>> tags_dummy;
    EXPECT_EQ(operation_tag->tag_get_all_by_user(nullptr, tags_dummy), TAG_ERROR);

    // User 1 adds tag
    EXPECT_EQ(operation_tag->tag_add(test_user1, tag), RST_OK);
    EXPECT_GT(tag.get_id(), 0u);

    // Duplicate tag for User 1
    Tag dup_tag(0, "penal");
    EXPECT_EQ(operation_tag->tag_add(test_user1, dup_tag), TAG_DUPLICATED);

    // User 2 can add tag with same name (isolated per user)
    Tag user2_tag(0, "penal");
    EXPECT_EQ(operation_tag->tag_add(test_user2, user2_tag), RST_OK);
    EXPECT_NE(tag.get_id(), user2_tag.get_id());

    // Get tag by ID
    std::shared_ptr<Tag> fetched_tag = nullptr;
    EXPECT_EQ(operation_tag->tag_get_by_id(test_user1, tag.get_id(), fetched_tag), RST_OK);
    ASSERT_NE(fetched_tag, nullptr);
    EXPECT_EQ(fetched_tag->get_name(), "penal");

    // Non-existent tag get by ID
    std::shared_ptr<Tag> non_existent = nullptr;
    EXPECT_EQ(operation_tag->tag_get_by_id(test_user1, 9999, non_existent), TAG_NOT_FOUND);

    // User 2 cannot get User 1's tag by ID
    std::shared_ptr<Tag> unauthorized_fetch = nullptr;
    EXPECT_EQ(operation_tag->tag_get_by_id(test_user2, tag.get_id(), unauthorized_fetch), TAG_NOT_FOUND);

    // Get tag by name
    std::shared_ptr<Tag> fetched_by_name = nullptr;
    EXPECT_EQ(operation_tag->tag_get_by_name(test_user1, "penal", fetched_by_name), RST_OK);
    ASSERT_NE(fetched_by_name, nullptr);

    EXPECT_EQ(operation_tag->tag_get_by_name(test_user1, "nonexistent", fetched_by_name), TAG_NOT_FOUND);

    // Update non-existent tag
    Tag nonexistent_tag(9999, "nonexistent");
    EXPECT_EQ(operation_tag->tag_update(test_user1, nonexistent_tag), TAG_NOT_FOUND);

    // Update tag
    Tag update_tag(tag.get_id(), "derecho_penal");
    EXPECT_EQ(operation_tag->tag_update(test_user1, update_tag), RST_OK);

    // User 2 cannot update User 1's tag
    EXPECT_EQ(operation_tag->tag_update(test_user2, update_tag), TAG_NOT_FOUND);

    // Update with duplicate name check
    Tag second_tag(0, "constitucional");
    ASSERT_EQ(operation_tag->tag_add(test_user1, second_tag), RST_OK);

    Tag invalid_rename(second_tag.get_id(), "derecho_penal");
    EXPECT_EQ(operation_tag->tag_update(test_user1, invalid_rename), TAG_DUPLICATED);

    // Get all tags for user
    std::vector<std::shared_ptr<Tag>> tags;
    EXPECT_EQ(operation_tag->tag_get_all_by_user(test_user1, tags), RST_OK);
    EXPECT_EQ(tags.size(), 2u);

    // Remove non-existent tag
    EXPECT_EQ(operation_tag->tag_remove(test_user1, 9999), TAG_NOT_FOUND);

    // Remove tag
    EXPECT_EQ(operation_tag->tag_remove(test_user2, tag.get_id()), TAG_NOT_FOUND); // wrong user
    EXPECT_EQ(operation_tag->tag_remove(test_user1, tag.get_id()), RST_OK);

    tags.clear();
    EXPECT_EQ(operation_tag->tag_get_all_by_user(test_user1, tags), RST_OK);
    EXPECT_EQ(tags.size(), 1u);
}

TEST_F(OperationTagTest, SubjectTagOperations)
{
    DB_Subject db_subject;

    // Create subject for user 1
    Subject sub1(0, "Tema Civil");
    sub1.set_user_id(test_user1->get_useraccountid());
    sub1.set_filepath("/data/civil.pdf");
    ASSERT_EQ(db_subject.add_new_subject(sub1), RST_OK);

    // Create tag for user 1
    Tag tag1(0, "civil");
    ASSERT_EQ(operation_tag->tag_add(test_user1, tag1), RST_OK);

    // Create tag for user 2
    Tag tag2(0, "penal");
    ASSERT_EQ(operation_tag->tag_add(test_user2, tag2), RST_OK);

    // Test with null user
    EXPECT_EQ(operation_tag->subject_add_tag(nullptr, 1, 1), TAG_ERROR);
    EXPECT_EQ(operation_tag->subject_remove_tag(nullptr, 1, 1), TAG_ERROR);
    std::vector<std::shared_ptr<Tag>> null_tags;
    EXPECT_EQ(operation_tag->subject_get_tags(nullptr, 1, null_tags), TAG_ERROR);
    std::vector<std::shared_ptr<Subject>> null_subs;
    EXPECT_EQ(operation_tag->subject_get_all_by_tag(nullptr, 1, null_subs), TAG_ERROR);

    // User 1 attaches User 1's tag to User 1's subject
    EXPECT_EQ(operation_tag->subject_add_tag(test_user1, sub1.get_id(), tag1.get_id()), RST_OK);

    // User 1 trying to attach User 2's tag -> TAG_NOT_FOUND
    EXPECT_EQ(operation_tag->subject_add_tag(test_user1, sub1.get_id(), tag2.get_id()), TAG_NOT_FOUND);

    // User 2 trying to attach tag to User 1's subject -> SUBJECT_NOT_FOUND
    EXPECT_EQ(operation_tag->subject_add_tag(test_user2, sub1.get_id(), tag2.get_id()), SUBJECT_NOT_FOUND);

    // Get tags for subject
    std::vector<std::shared_ptr<Tag>> sub_tags;
    EXPECT_EQ(operation_tag->subject_get_tags(test_user1, sub1.get_id(), sub_tags), RST_OK);
    ASSERT_EQ(sub_tags.size(), 1u);
    EXPECT_EQ(sub_tags[0]->get_id(), tag1.get_id());

    // User 2 cannot inspect User 1's subject tags
    EXPECT_EQ(operation_tag->subject_get_tags(test_user2, sub1.get_id(), sub_tags), SUBJECT_NOT_FOUND);

    // Get subjects by tag
    std::vector<std::shared_ptr<Subject>> tagged_subjects;
    EXPECT_EQ(operation_tag->subject_get_all_by_tag(test_user1, tag1.get_id(), tagged_subjects), RST_OK);
    ASSERT_EQ(tagged_subjects.size(), 1u);
    EXPECT_EQ(tagged_subjects[0]->get_id(), sub1.get_id());
    EXPECT_EQ(tagged_subjects[0]->get_tags().size(), 1u);

    // User 2 cannot query User 1's tag
    EXPECT_EQ(operation_tag->subject_get_all_by_tag(test_user2, tag1.get_id(), tagged_subjects), TAG_NOT_FOUND);

    // Remove tag from subject
    EXPECT_EQ(operation_tag->subject_remove_tag(test_user1, sub1.get_id(), tag1.get_id()), RST_OK);
    sub_tags.clear();
    EXPECT_EQ(operation_tag->subject_get_tags(test_user1, sub1.get_id(), sub_tags), RST_OK);
    EXPECT_EQ(sub_tags.size(), 0u);
}
