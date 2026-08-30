/**
 * @file operation_tag.cpp
 * @brief Implementation of OperationTag business logic and subject-tag management.
 */

#include "operations/operation_tag.hpp"
#include "database/db_main.hpp"
#include "database/db_tag.hpp"
#include "database/db_subject.hpp"

OperationTag::OperationTag()
{
    DB_Main::getInstance();
}

OperationTag::~OperationTag() {}

rst_code_e OperationTag::tag_add(const std::shared_ptr<const User> &user, Tag &tag)
{
    if (user == nullptr || user->get_useraccountid() == 0)
    {
        logger->error("User info error");
        return TAG_ERROR;
    }

    DB_Tag db_tag;
    bool already_exists = false;

    tag.set_user_id(user->get_useraccountid());

    rst_code_e rst = db_tag.is_tag_already_present(tag.get_user_id(), tag.get_name(), already_exists);
    if (rst != RST_OK)
    {
        logger->error("Error checking for duplicate tag");
        return TAG_ERROR;
    }

    if (already_exists)
    {
        logger->warn("Tag '{}' already exists for user.", tag.get_name());
        return TAG_DUPLICATED;
    }

    rst = db_tag.add_new_tag(tag);
    if (rst != RST_OK)
    {
        logger->warn("Error when adding a new tag: {}", get_rst_txt(rst));
        return TAG_ERROR;
    }

    return RST_OK;
}

rst_code_e OperationTag::tag_update(const std::shared_ptr<const User> &user, const Tag &tag)
{
    if (user == nullptr || user->get_useraccountid() == 0)
    {
        logger->error("User info error");
        return TAG_ERROR;
    }

    DB_Tag db_tag;
    std::shared_ptr<Tag> existing = nullptr;
    rst_code_e rst = db_tag.get_tag_by_id(tag.get_id(), existing);
    if (rst != RST_OK || existing == nullptr)
    {
        logger->warn("Tag not found for update");
        return TAG_NOT_FOUND;
    }

    if (existing->get_user_id() != user->get_useraccountid())
    {
        logger->error("Tag does not belong to user");
        return TAG_NOT_FOUND;
    }

    // Check duplicate name among user's other tags
    std::vector<std::shared_ptr<Tag>> user_tags;
    rst = db_tag.get_all_tags_by_user(user->get_useraccountid(), user_tags);
    if (rst != RST_OK)
    {
        return TAG_ERROR;
    }

    for (const auto &t : user_tags)
    {
        if (t->get_name() == tag.get_name() && t->get_id() != tag.get_id())
        {
            return TAG_DUPLICATED;
        }
    }

    Tag updated_tag = tag;
    updated_tag.set_user_id(user->get_useraccountid());

    rst = db_tag.update_tag(updated_tag);
    if (rst != RST_OK)
    {
        logger->warn("Update tag error: {}", get_rst_txt(rst));
        return TAG_ERROR;
    }

    return RST_OK;
}

rst_code_e OperationTag::tag_remove(const std::shared_ptr<const User> &user, unsigned int id)
{
    if (user == nullptr || user->get_useraccountid() == 0)
    {
        logger->error("User info error");
        return TAG_ERROR;
    }

    DB_Tag db_tag;
    std::shared_ptr<Tag> existing = nullptr;
    rst_code_e rst = db_tag.get_tag_by_id(id, existing);
    if (rst != RST_OK || existing == nullptr)
    {
        logger->warn("Tag not found for removal");
        return TAG_NOT_FOUND;
    }

    if (existing->get_user_id() != user->get_useraccountid())
    {
        logger->error("Tag does not belong to user");
        return TAG_NOT_FOUND;
    }

    rst = db_tag.remove_tag(id);
    if (rst != RST_OK)
    {
        logger->warn("Error removing tag: {}", get_rst_txt(rst));
        return TAG_ERROR;
    }

    return RST_OK;
}

rst_code_e OperationTag::tag_get_by_id(const std::shared_ptr<const User> &user, unsigned int id, std::shared_ptr<Tag> &tag)
{
    if (user == nullptr || user->get_useraccountid() == 0)
    {
        logger->error("User info error");
        return TAG_ERROR;
    }

    DB_Tag db_tag;
    rst_code_e rst = db_tag.get_tag_by_id(id, tag);
    if (rst != RST_OK || tag == nullptr)
    {
        logger->warn("Tag not found by id");
        return TAG_NOT_FOUND;
    }

    if (tag->get_user_id() != user->get_useraccountid())
    {
        logger->error("Tag does not belong to user");
        tag = nullptr;
        return TAG_NOT_FOUND;
    }

    return RST_OK;
}

rst_code_e OperationTag::tag_get_by_name(const std::shared_ptr<const User> &user, const std::string &name, std::shared_ptr<Tag> &tag)
{
    if (user == nullptr || user->get_useraccountid() == 0)
    {
        logger->error("User info error");
        return TAG_ERROR;
    }

    DB_Tag db_tag;
    rst_code_e rst = db_tag.get_tag_by_name(user->get_useraccountid(), name, tag);
    if (rst != RST_OK || tag == nullptr)
    {
        logger->warn("Tag not found by name");
        return TAG_NOT_FOUND;
    }

    return RST_OK;
}

rst_code_e OperationTag::tag_get_all_by_user(const std::shared_ptr<const User> &user, std::vector<std::shared_ptr<Tag>> &tags)
{
    if (user == nullptr || user->get_useraccountid() == 0)
    {
        logger->error("User info error");
        return TAG_ERROR;
    }

    DB_Tag db_tag;
    rst_code_e rst = db_tag.get_all_tags_by_user(user->get_useraccountid(), tags);
    if (rst != RST_OK)
    {
        logger->warn("Error getting all tags for user: {}", get_rst_txt(rst));
        return TAG_ERROR;
    }

    return RST_OK;
}

rst_code_e OperationTag::subject_add_tag(const std::shared_ptr<const User> &user, unsigned int subject_id, unsigned int tag_id)
{
    if (user == nullptr || user->get_useraccountid() == 0)
    {
        logger->error("User info error");
        return TAG_ERROR;
    }

    DB_Subject db_subject;
    std::shared_ptr<Subject> subject = nullptr;
    rst_code_e rst = db_subject.get_subject_by_id(subject_id, subject);
    if (rst != RST_OK || subject == nullptr || subject->get_user_id() != user->get_useraccountid())
    {
        logger->error("Subject not found or does not belong to user");
        return SUBJECT_NOT_FOUND;
    }

    DB_Tag db_tag;
    std::shared_ptr<Tag> tag = nullptr;
    rst = db_tag.get_tag_by_id(tag_id, tag);
    if (rst != RST_OK || tag == nullptr || tag->get_user_id() != user->get_useraccountid())
    {
        logger->error("Tag not found or does not belong to user");
        return TAG_NOT_FOUND;
    }

    rst = db_tag.subject_add_tag(subject_id, tag_id);
    if (rst != RST_OK)
    {
        logger->warn("Error adding tag to subject: {}", get_rst_txt(rst));
        return TAG_ERROR;
    }

    return RST_OK;
}

rst_code_e OperationTag::subject_remove_tag(const std::shared_ptr<const User> &user, unsigned int subject_id, unsigned int tag_id)
{
    if (user == nullptr || user->get_useraccountid() == 0)
    {
        logger->error("User info error");
        return TAG_ERROR;
    }

    DB_Subject db_subject;
    std::shared_ptr<Subject> subject = nullptr;
    rst_code_e rst = db_subject.get_subject_by_id(subject_id, subject);
    if (rst != RST_OK || subject == nullptr || subject->get_user_id() != user->get_useraccountid())
    {
        logger->error("Subject not found or does not belong to user");
        return SUBJECT_NOT_FOUND;
    }

    DB_Tag db_tag;
    rst = db_tag.subject_remove_tag(subject_id, tag_id);
    if (rst != RST_OK)
    {
        logger->warn("Error removing tag from subject: {}", get_rst_txt(rst));
        return TAG_ERROR;
    }

    return RST_OK;
}

rst_code_e OperationTag::subject_get_tags(const std::shared_ptr<const User> &user, unsigned int subject_id, std::vector<std::shared_ptr<Tag>> &tags)
{
    if (user == nullptr || user->get_useraccountid() == 0)
    {
        logger->error("User info error");
        return TAG_ERROR;
    }

    DB_Subject db_subject;
    std::shared_ptr<Subject> subject = nullptr;
    rst_code_e rst = db_subject.get_subject_by_id(subject_id, subject);
    if (rst != RST_OK || subject == nullptr || subject->get_user_id() != user->get_useraccountid())
    {
        logger->error("Subject not found or does not belong to user");
        return SUBJECT_NOT_FOUND;
    }

    DB_Tag db_tag;
    rst = db_tag.get_tags_by_subject(subject_id, tags);
    if (rst != RST_OK)
    {
        logger->warn("Error retrieving tags for subject: {}", get_rst_txt(rst));
        return TAG_ERROR;
    }

    return RST_OK;
}

rst_code_e OperationTag::subject_get_all_by_tag(const std::shared_ptr<const User> &user, unsigned int tag_id, std::vector<std::shared_ptr<Subject>> &subjects)
{
    if (user == nullptr || user->get_useraccountid() == 0)
    {
        logger->error("User info error");
        return TAG_ERROR;
    }

    DB_Tag db_tag;
    std::shared_ptr<Tag> tag = nullptr;
    rst_code_e rst = db_tag.get_tag_by_id(tag_id, tag);
    if (rst != RST_OK || tag == nullptr || tag->get_user_id() != user->get_useraccountid())
    {
        logger->error("Tag not found or does not belong to user");
        return TAG_NOT_FOUND;
    }

    rst = db_tag.get_subjects_by_tag(user->get_useraccountid(), tag_id, subjects);
    if (rst != RST_OK)
    {
        logger->warn("Error retrieving subjects by tag: {}", get_rst_txt(rst));
        return TAG_ERROR;
    }

    // Populate each subject's tag list
    for (auto &sub : subjects)
    {
        std::vector<std::shared_ptr<Tag>> sub_tag_ptrs;
        if (db_tag.get_tags_by_subject(sub->get_id(), sub_tag_ptrs) == RST_OK)
        {
            std::vector<Tag> sub_tags;
            for (const auto &tp : sub_tag_ptrs)
            {
                if (tp) sub_tags.push_back(*tp);
            }
            sub->set_tags(sub_tags);
        }
    }

    return RST_OK;
}
