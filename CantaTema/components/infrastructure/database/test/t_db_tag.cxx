/**
 * @file t_db_tag.cxx
 * @brief Unit tests for DB_Tag SQLite repository and subject_tags junction persistence.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <memory>
#include <vector>

#include "mock_tool_paths.hpp"
#include "database/db_tag.hpp"
#include "database/db_subject.hpp"
#include "database/db_category.hpp"
#include "database/db_user.hpp"
#include "database/db_connection.hpp"
#include "primitives/tag.hpp"
#include "primitives/subject.hpp"
#include "primitives/user.hpp"

using ::testing::Return;

class DBTagTest : public ::testing::Test {
protected:
    MockToolPath mockToolPath;
    std::filesystem::path temp_db_dir;

    void SetUp() override {
        g_mockToolPath = &mockToolPath;
        temp_db_dir = std::filesystem::temp_directory_path() / ("canta_tema_test_db_tag_" + std::to_string(std::time(nullptr)));
        std::filesystem::create_directories(temp_db_dir);

        EXPECT_CALL(mockToolPath, get_path_for_database())
            .WillRepeatedly(Return(temp_db_dir));

        DB_Connection::reset_connection();

        DB_User db_user;
        ASSERT_EQ(db_user.user_tables_create(), RST_OK);

        DB_Category db_category;
        ASSERT_EQ(db_category.category_tables_create(), RST_OK);

        DB_Subject db_subject;
        ASSERT_EQ(db_subject.subject_tables_create(), RST_OK);

        DB_Tag db_tag;
        ASSERT_EQ(db_tag.tag_tables_create(), RST_OK);
    }

    void TearDown() override {
        DB_Connection::reset_connection();
        if (std::filesystem::exists(temp_db_dir)) {
            std::filesystem::remove_all(temp_db_dir);
        }
        g_mockToolPath = nullptr;
    }
};

TEST_F(DBTagTest, TagCrudAndUserIsolation)
{
    DB_User db_user;
    DB_Tag db_tag;

    // Create two test users
    User user1("Alice");
    user1.set_passwordkey("pass1");
    ASSERT_EQ(db_user.add_new_user(user1), RST_OK);

    User user2("Bob");
    user2.set_passwordkey("pass2");
    ASSERT_EQ(db_user.add_new_user(user2), RST_OK);

    // User 1 creates a tag
    Tag tag1(0, "penal");
    tag1.set_user_id(user1.get_useraccountid());
    EXPECT_EQ(db_tag.add_new_tag(tag1), RST_OK);
    EXPECT_GT(tag1.get_id(), 0u);

    // Check tag presence
    bool exists = false;
    EXPECT_EQ(db_tag.is_tag_already_present(user1.get_useraccountid(), "penal", exists), RST_OK);
    EXPECT_TRUE(exists);

    // User 2 can create a tag with the SAME name (user-scoped uniqueness)
    Tag tag2(0, "penal");
    tag2.set_user_id(user2.get_useraccountid());
    EXPECT_EQ(db_tag.add_new_tag(tag2), RST_OK);
    EXPECT_GT(tag2.get_id(), 0u);
    EXPECT_NE(tag1.get_id(), tag2.get_id());

    // User 1 cannot create duplicate tag name
    Tag dup_tag(0, "penal");
    dup_tag.set_user_id(user1.get_useraccountid());
    EXPECT_EQ(db_tag.add_new_tag(dup_tag), DB_FAIL);

    // Get tag by ID
    std::shared_ptr<Tag> fetched_tag = nullptr;
    EXPECT_EQ(db_tag.get_tag_by_id(tag1.get_id(), fetched_tag), RST_OK);
    ASSERT_NE(fetched_tag, nullptr);
    EXPECT_EQ(fetched_tag->get_name(), "penal");
    EXPECT_EQ(fetched_tag->get_user_id(), user1.get_useraccountid());

    // Get tag by name
    std::shared_ptr<Tag> fetched_by_name = nullptr;
    EXPECT_EQ(db_tag.get_tag_by_name(user1.get_useraccountid(), "penal", fetched_by_name), RST_OK);
    ASSERT_NE(fetched_by_name, nullptr);
    EXPECT_EQ(fetched_by_name->get_id(), tag1.get_id());

    // Update tag
    fetched_tag->set_name("derecho_penal");
    EXPECT_EQ(db_tag.update_tag(*fetched_tag), RST_OK);

    std::shared_ptr<Tag> updated_tag = nullptr;
    EXPECT_EQ(db_tag.get_tag_by_id(tag1.get_id(), updated_tag), RST_OK);
    EXPECT_EQ(updated_tag->get_name(), "derecho_penal");

    // List all tags for user 1
    Tag tag1_second(0, "constitucional");
    tag1_second.set_user_id(user1.get_useraccountid());
    EXPECT_EQ(db_tag.add_new_tag(tag1_second), RST_OK);

    std::vector<std::shared_ptr<Tag>> user1_tags;
    EXPECT_EQ(db_tag.get_all_tags_by_user(user1.get_useraccountid(), user1_tags), RST_OK);
    EXPECT_EQ(user1_tags.size(), 2u);

    std::vector<std::shared_ptr<Tag>> user2_tags;
    EXPECT_EQ(db_tag.get_all_tags_by_user(user2.get_useraccountid(), user2_tags), RST_OK);
    EXPECT_EQ(user2_tags.size(), 1u);

    // Remove single tag
    EXPECT_EQ(db_tag.remove_tag(tag1.get_id()), RST_OK);
    user1_tags.clear();
    EXPECT_EQ(db_tag.get_all_tags_by_user(user1.get_useraccountid(), user1_tags), RST_OK);
    EXPECT_EQ(user1_tags.size(), 1u);

    // Remove all tags for user 1
    EXPECT_EQ(db_tag.remove_all_tags_from_user(user1.get_useraccountid()), RST_OK);
    user1_tags.clear();
    EXPECT_EQ(db_tag.get_all_tags_by_user(user1.get_useraccountid(), user1_tags), RST_OK);
    EXPECT_EQ(user1_tags.size(), 0u);

    // User 2's tags remain intact
    user2_tags.clear();
    EXPECT_EQ(db_tag.get_all_tags_by_user(user2.get_useraccountid(), user2_tags), RST_OK);
    EXPECT_EQ(user2_tags.size(), 1u);
}

TEST_F(DBTagTest, SubjectTagManyToManyAssociations)
{
    DB_User db_user;
    DB_Subject db_subject;
    DB_Tag db_tag;

    User user("Carlos");
    user.set_passwordkey("pass123");
    ASSERT_EQ(db_user.add_new_user(user), RST_OK);

    // Create two subjects
    Subject sub1(0, "Tema 1: Constitucion");
    sub1.set_user_id(user.get_useraccountid());
    sub1.set_filepath("/data/sub1.pdf");
    ASSERT_EQ(db_subject.add_new_subject(sub1), RST_OK);

    Subject sub2(0, "Tema 2: Corona");
    sub2.set_user_id(user.get_useraccountid());
    sub2.set_filepath("/data/sub2.pdf");
    ASSERT_EQ(db_subject.add_new_subject(sub2), RST_OK);

    // Create three tags
    Tag tag1(0, "constitucional");
    tag1.set_user_id(user.get_useraccountid());
    ASSERT_EQ(db_tag.add_new_tag(tag1), RST_OK);

    Tag tag2(0, "urgente");
    tag2.set_user_id(user.get_useraccountid());
    ASSERT_EQ(db_tag.add_new_tag(tag2), RST_OK);

    Tag tag3(0, "repasar");
    tag3.set_user_id(user.get_useraccountid());
    ASSERT_EQ(db_tag.add_new_tag(tag3), RST_OK);

    // Link tag1 and tag2 to subject 1
    EXPECT_EQ(db_tag.subject_add_tag(sub1.get_id(), tag1.get_id()), RST_OK);
    EXPECT_EQ(db_tag.subject_add_tag(sub1.get_id(), tag2.get_id()), RST_OK);

    // Link tag1 and tag3 to subject 2
    EXPECT_EQ(db_tag.subject_add_tag(sub2.get_id(), tag1.get_id()), RST_OK);
    EXPECT_EQ(db_tag.subject_add_tag(sub2.get_id(), tag3.get_id()), RST_OK);

    // Fetch tags for subject 1
    std::vector<std::shared_ptr<Tag>> sub1_tags;
    EXPECT_EQ(db_tag.get_tags_by_subject(sub1.get_id(), sub1_tags), RST_OK);
    ASSERT_EQ(sub1_tags.size(), 2u);

    // Fetch subjects with tag 1 (constitucional) -> should return sub1 and sub2
    std::vector<std::shared_ptr<Subject>> tagged_subjects;
    EXPECT_EQ(db_tag.get_subjects_by_tag(user.get_useraccountid(), tag1.get_id(), tagged_subjects), RST_OK);
    ASSERT_EQ(tagged_subjects.size(), 2u);

    // Fetch subjects with tag 2 (urgente) -> should return only sub1
    tagged_subjects.clear();
    EXPECT_EQ(db_tag.get_subjects_by_tag(user.get_useraccountid(), tag2.get_id(), tagged_subjects), RST_OK);
    ASSERT_EQ(tagged_subjects.size(), 1u);
    EXPECT_EQ(tagged_subjects[0]->get_id(), sub1.get_id());

    // Unlink tag 2 from subject 1
    EXPECT_EQ(db_tag.subject_remove_tag(sub1.get_id(), tag2.get_id()), RST_OK);
    sub1_tags.clear();
    EXPECT_EQ(db_tag.get_tags_by_subject(sub1.get_id(), sub1_tags), RST_OK);
    ASSERT_EQ(sub1_tags.size(), 1u);
    EXPECT_EQ(sub1_tags[0]->get_id(), tag1.get_id());

    // Remove all tags from subject 2
    EXPECT_EQ(db_tag.subject_remove_all_tags(sub2.get_id()), RST_OK);
    std::vector<std::shared_ptr<Tag>> sub2_tags;
    EXPECT_EQ(db_tag.get_tags_by_subject(sub2.get_id(), sub2_tags), RST_OK);
    EXPECT_EQ(sub2_tags.size(), 0u);

    // Delete tag1 -> should remove association from sub1
    EXPECT_EQ(db_tag.remove_tag(tag1.get_id()), RST_OK);
    sub1_tags.clear();
    EXPECT_EQ(db_tag.get_tags_by_subject(sub1.get_id(), sub1_tags), RST_OK);
    EXPECT_EQ(sub1_tags.size(), 0u);

    // Subject 1 itself is still alive
    std::shared_ptr<Subject> check_sub1 = nullptr;
    EXPECT_EQ(db_subject.get_subject_by_id(sub1.get_id(), check_sub1), RST_OK);
    ASSERT_NE(check_sub1, nullptr);

    // Check association status queries
    std::vector<std::shared_ptr<Tag>> non_existent_sub_tags;
    EXPECT_EQ(db_tag.get_tags_by_subject(9999, non_existent_sub_tags), RST_OK);
    EXPECT_EQ(non_existent_sub_tags.size(), 0u);

    // Queries on non-existent tags
    std::shared_ptr<Tag> non_existent_tag = nullptr;
    EXPECT_EQ(db_tag.get_tag_by_id(9999, non_existent_tag), DB_FAIL);
    EXPECT_EQ(db_tag.get_tag_by_name(user.get_useraccountid(), "nonexistent", non_existent_tag), DB_FAIL);

    bool exists = false;
    EXPECT_EQ(db_tag.is_tag_already_present(user.get_useraccountid(), "nonexistent", exists), RST_OK);
    EXPECT_FALSE(exists);

    // Update non-existent tag
    Tag dummy_tag(9999, "dummy");
    EXPECT_EQ(db_tag.update_tag(dummy_tag), RST_OK);

    // Remove non-existent tag
    EXPECT_EQ(db_tag.remove_tag(9999), RST_OK);

    // Get subjects for unassigned tag
    std::vector<std::shared_ptr<Subject>> empty_subs;
    EXPECT_EQ(db_tag.get_subjects_by_tag(user.get_useraccountid(), tag3.get_id(), empty_subs), RST_OK);
    EXPECT_EQ(empty_subs.size(), 0u);

    // Get all tags for user with no tags
    std::vector<std::shared_ptr<Tag>> empty_tags;
    EXPECT_EQ(db_tag.get_all_tags_by_user(9999, empty_tags), RST_OK);
    EXPECT_EQ(empty_tags.size(), 0u);

    // Idempotent table create
    EXPECT_EQ(db_tag.tag_tables_create(), RST_OK);
}
