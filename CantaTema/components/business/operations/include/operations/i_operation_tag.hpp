/**
 * @file i_operation_tag.hpp
 * @brief Abstract interface for tag management operations and subject-tag associations.
 */

#ifndef __IOPERATION_TAG_HPP
#define __IOPERATION_TAG_HPP

#include <vector>
#include <memory>
#include <string>

#include "primitives/definitions.hpp"
#include "primitives/tag.hpp"
#include "primitives/subject.hpp"
#include "primitives/user.hpp"

/**
 * @class IOperationTag
 * @brief Abstract interface defining tag creation, retrieval, update, removal, and subject association operations.
 */
class IOperationTag
{
public:
    /**
     * @brief Virtual destructor for IOperationTag.
     */
    virtual ~IOperationTag() = default;

    /**
     * @brief Adds a new tag for the authenticated user after checking for duplicates.
     * @param user Pointer to authenticated user.
     * @param tag Reference to Tag object containing tag name and receiving new ID.
     * @return rst_code_e RST_OK on success, TAG_DUPLICATED, USER_NO_AUTH, or TAG_ERROR.
     */
    virtual rst_code_e tag_add(const std::shared_ptr<const User> &user, Tag &tag) = 0;

    /**
     * @brief Updates an existing tag record.
     * @param user Pointer to authenticated user.
     * @param tag Tag object with updated name and attributes.
     * @return rst_code_e RST_OK on success, TAG_NOT_FOUND, TAG_DUPLICATED, or TAG_ERROR.
     */
    virtual rst_code_e tag_update(const std::shared_ptr<const User> &user, const Tag &tag) = 0;

    /**
     * @brief Removes a tag by ID.
     * @param user Pointer to authenticated user.
     * @param id Unique tag identifier.
     * @return rst_code_e RST_OK on success, TAG_NOT_FOUND, or TAG_ERROR.
     */
    virtual rst_code_e tag_remove(const std::shared_ptr<const User> &user, unsigned int id) = 0;

    /**
     * @brief Retrieves a tag record by ID.
     * @param user Pointer to authenticated user.
     * @param id Unique tag identifier.
     * @param tag Output shared pointer receiving Tag object.
     * @return rst_code_e RST_OK on success, TAG_NOT_FOUND, or TAG_ERROR.
     */
    virtual rst_code_e tag_get_by_id(const std::shared_ptr<const User> &user, unsigned int id, std::shared_ptr<Tag> &tag) = 0;

    /**
     * @brief Retrieves a tag by name for the authenticated user.
     * @param user Pointer to authenticated user.
     * @param name Tag name string.
     * @param tag Output shared pointer receiving Tag object.
     * @return rst_code_e RST_OK on success, TAG_NOT_FOUND, or TAG_ERROR.
     */
    virtual rst_code_e tag_get_by_name(const std::shared_ptr<const User> &user, const std::string &name, std::shared_ptr<Tag> &tag) = 0;

    /**
     * @brief Retrieves all tags owned by the authenticated user.
     * @param user Pointer to authenticated user.
     * @param tags Output vector receiving matching Tag objects.
     * @return rst_code_e RST_OK on success, or USER_NO_AUTH.
     */
    virtual rst_code_e tag_get_all_by_user(const std::shared_ptr<const User> &user, std::vector<std::shared_ptr<Tag>> &tags) = 0;

    //-------------------------------------------------------------------------------------
    // Subject Tag Associations
    //-------------------------------------------------------------------------------------

    /**
     * @brief Attaches a tag to a subject.
     * @param user Pointer to authenticated user.
     * @param subject_id Subject identifier.
     * @param tag_id Tag identifier.
     * @return rst_code_e RST_OK on success, error code otherwise.
     */
    virtual rst_code_e subject_add_tag(const std::shared_ptr<const User> &user, unsigned int subject_id, unsigned int tag_id) = 0;

    /**
     * @brief Detaches a tag from a subject.
     * @param user Pointer to authenticated user.
     * @param subject_id Subject identifier.
     * @param tag_id Tag identifier.
     * @return rst_code_e RST_OK on success, error code otherwise.
     */
    virtual rst_code_e subject_remove_tag(const std::shared_ptr<const User> &user, unsigned int subject_id, unsigned int tag_id) = 0;

    /**
     * @brief Retrieves all tags attached to a subject.
     * @param user Pointer to authenticated user.
     * @param subject_id Subject identifier.
     * @param tags Output vector receiving attached Tag objects.
     * @return rst_code_e RST_OK on success, error code otherwise.
     */
    virtual rst_code_e subject_get_tags(const std::shared_ptr<const User> &user, unsigned int subject_id, std::vector<std::shared_ptr<Tag>> &tags) = 0;

    /**
     * @brief Retrieves all subjects owned by the user that have the given tag.
     * @param user Pointer to authenticated user.
     * @param tag_id Tag identifier.
     * @param subjects Output vector receiving matching Subject objects.
     * @return rst_code_e RST_OK on success, error code otherwise.
     */
    virtual rst_code_e subject_get_all_by_tag(const std::shared_ptr<const User> &user, unsigned int tag_id, std::vector<std::shared_ptr<Subject>> &subjects) = 0;
};

#endif // __IOPERATION_TAG_HPP
