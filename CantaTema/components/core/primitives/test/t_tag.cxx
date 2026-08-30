/**
 * @file t_tag.cxx
 * @brief Unit tests for Tag domain model, Subject tag associations, and tag printing.
 */

#include <gtest/gtest.h>
#include "primitives/tag.hpp"
#include "primitives/subject.hpp"
#include "primitives/utils_prints.hpp"
#include "primitives/definitions.hpp"

TEST(TagTest, ConstructAndGettersSetters)
{
    Tag tag(1, "derecho_penal");
    EXPECT_EQ(tag.get_id(), 1);
    EXPECT_EQ(tag.get_name(), "derecho_penal");
    EXPECT_EQ(tag.get_user_id(), 0);

    tag.set_id(42);
    EXPECT_EQ(tag.get_id(), 42);

    tag.set_user_id(7);
    EXPECT_EQ(tag.get_user_id(), 7);

    tag.set_name("derecho_constitucional");
    EXPECT_EQ(tag.get_name(), "derecho_constitucional");
}

TEST(TagTest, SubjectTagAssociations)
{
    Subject subject(10, "Tema 1: La Constitucion");
    EXPECT_EQ(subject.get_tags().size(), 0);
    EXPECT_FALSE(subject.has_tag(1));

    Tag tag1(1, "constitucional");
    tag1.set_user_id(5);
    Tag tag2(2, "urgente");
    tag2.set_user_id(5);

    // Add tag 1
    subject.add_tag(tag1);
    EXPECT_EQ(subject.get_tags().size(), 1);
    EXPECT_TRUE(subject.has_tag(1));
    EXPECT_FALSE(subject.has_tag(2));

    // Adding duplicate tag should be a no-op
    subject.add_tag(tag1);
    EXPECT_EQ(subject.get_tags().size(), 1);

    // Add tag 2
    subject.add_tag(tag2);
    EXPECT_EQ(subject.get_tags().size(), 2);
    EXPECT_TRUE(subject.has_tag(1));
    EXPECT_TRUE(subject.has_tag(2));

    // Remove tag 1
    subject.remove_tag(1);
    EXPECT_EQ(subject.get_tags().size(), 1);
    EXPECT_FALSE(subject.has_tag(1));
    EXPECT_TRUE(subject.has_tag(2));

    // Removing non-existent tag should be a no-op
    subject.remove_tag(999);
    EXPECT_EQ(subject.get_tags().size(), 1);

    // Set bulk tags
    std::vector<Tag> new_tags = { Tag(3, "dificil"), Tag(4, "revisar") };
    subject.set_tags(new_tags);
    EXPECT_EQ(subject.get_tags().size(), 2);
    EXPECT_TRUE(subject.has_tag(3));
    EXPECT_TRUE(subject.has_tag(4));
    EXPECT_FALSE(subject.has_tag(2));
}

TEST(TagTest, UtilsPrintsTagOutput)
{
    Tag tag(5, "administrativo");
    tag.set_user_id(12);

    std::string header = UtilsPrints::get_tag_header();
    EXPECT_FALSE(header.empty());
    EXPECT_NE(header.find("ID"), std::string::npos);
    EXPECT_NE(header.find("Name"), std::string::npos);

    std::string body = UtilsPrints::get_tag_body(tag);
    EXPECT_NE(body.find("administrativo"), std::string::npos);
    EXPECT_NE(body.find("5"), std::string::npos);
    EXPECT_NE(body.find("12"), std::string::npos);

    Subject subject(1, "Tema Admin");
    subject.add_tag(tag);
    std::string subject_body = UtilsPrints::get_subject_body(subject);
    EXPECT_NE(subject_body.find("administrativo"), std::string::npos);
}

TEST(TagTest, DefinitionCodes)
{
    EXPECT_EQ(get_rst_txt(TAG_ERROR), "TAG_ERROR");
    EXPECT_EQ(get_rst_txt(TAG_NOT_FOUND), "TAG_NOT_FOUND");
    EXPECT_EQ(get_rst_txt(TAG_DUPLICATED), "TAG_DUPLICATED");
}
