/**
 * @file db_tag.hpp
 * @brief Database repository for tags and subject-tag associations using SQLite.
 */

#ifndef __DB_TAG_HPP
#define __DB_TAG_HPP

#include <string>
#include <vector>
#include <memory>

#include "primitives/definitions.hpp"
#include "primitives/utils_logger.hpp"
#include "primitives/tag.hpp"
#include "primitives/subject.hpp"

/**
 * @class DB_Tag
 * @brief SQLite persistence handler for tags and many-to-many subject_tags associations.
 */
class DB_Tag
{
public:
    /**
     * @brief Constructs a new DB_Tag object.
     */
    DB_Tag(void);

    /**
     * @brief Creates `tags` and `subject_tags` tables on the database.
     * @return rst_code_e RST_OK on success, or DB_FAIL.
     */
    rst_code_e tag_tables_create(void) const;

    /**
     * @brief Checks if a tag name is already registered for a specific user.
     * @param user_id Owning user ID.
     * @param name Tag name.
     * @param exists Output boolean flag set to true if tag exists.
     * @return rst_code_e RST_OK on success, or DB_FAIL.
     */
    rst_code_e is_tag_already_present(unsigned int user_id, const std::string &name, bool &exists) const;

    /**
     * @brief Adds a new tag to the database and updates its generated ID.
     * @param tag Tag object to insert.
     * @return rst_code_e RST_OK on success, or DB_FAIL.
     */
    rst_code_e add_new_tag(Tag &tag) const;

    /**
     * @brief Updates an existing tag record.
     * @param tag Tag object containing updated attributes.
     * @return rst_code_e RST_OK on success, or DB_FAIL.
     */
    rst_code_e update_tag(const Tag &tag) const;

    /**
     * @brief Removes a tag by ID and cleans up its subject associations.
     * @param id Unique tag ID.
     * @return rst_code_e RST_OK on success, or DB_FAIL.
     */
    rst_code_e remove_tag(unsigned int id) const;

    /**
     * @brief Removes all tags associated with a specific user.
     * @param user_id User identifier.
     * @return rst_code_e RST_OK on success, or DB_FAIL.
     */
    rst_code_e remove_all_tags_from_user(unsigned int user_id) const;

    /**
     * @brief Retrieves a tag by its ID.
     * @param id Unique tag ID.
     * @param tag Output shared pointer receiving Tag object.
     * @return rst_code_e RST_OK on success, or DB_FAIL.
     */
    rst_code_e get_tag_by_id(unsigned int id, std::shared_ptr<Tag> &tag) const;

    /**
     * @brief Retrieves a tag by user ID and tag name.
     * @param user_id User identifier.
     * @param name Tag name string.
     * @param tag Output shared pointer receiving Tag object.
     * @return rst_code_e RST_OK on success, or DB_FAIL.
     */
    rst_code_e get_tag_by_name(unsigned int user_id, const std::string &name, std::shared_ptr<Tag> &tag) const;

    /**
     * @brief Retrieves all tags belonging to a specific user.
     * @param user_id User identifier.
     * @param tags Output vector receiving matching Tag objects.
     * @return rst_code_e RST_OK on success, or DB_FAIL.
     */
    rst_code_e get_all_tags_by_user(unsigned int user_id, std::vector<std::shared_ptr<Tag>> &tags) const;

    //-------------------------------------------------------------------------------------
    // Subject - Tag Many-to-Many Associations
    //-------------------------------------------------------------------------------------

    /**
     * @brief Links a tag to a subject.
     * @param subject_id Subject ID.
     * @param tag_id Tag ID.
     * @return rst_code_e RST_OK on success, or DB_FAIL.
     */
    rst_code_e subject_add_tag(unsigned int subject_id, unsigned int tag_id) const;

    /**
     * @brief Unlinks a tag from a subject.
     * @param subject_id Subject ID.
     * @param tag_id Tag ID.
     * @return rst_code_e RST_OK on success, or DB_FAIL.
     */
    rst_code_e subject_remove_tag(unsigned int subject_id, unsigned int tag_id) const;

    /**
     * @brief Unlinks all tags from a subject.
     * @param subject_id Subject ID.
     * @return rst_code_e RST_OK on success, or DB_FAIL.
     */
    rst_code_e subject_remove_all_tags(unsigned int subject_id) const;

    /**
     * @brief Retrieves all tags attached to a subject.
     * @param subject_id Subject ID.
     * @param tags Output vector receiving attached Tag objects.
     * @return rst_code_e RST_OK on success, or DB_FAIL.
     */
    rst_code_e get_tags_by_subject(unsigned int subject_id, std::vector<std::shared_ptr<Tag>> &tags) const;

    /**
     * @brief Retrieves all subjects belonging to a user that are labeled with a specific tag.
     * @param user_id User identifier.
     * @param tag_id Tag ID.
     * @param subjects Output vector receiving matching Subject objects.
     * @return rst_code_e RST_OK on success, or DB_FAIL.
     */
    rst_code_e get_subjects_by_tag(unsigned int user_id, unsigned int tag_id, std::vector<std::shared_ptr<Subject>> &subjects) const;
};

#endif // __DB_TAG_HPP
