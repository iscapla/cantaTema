/**
 * @file operation_tag.hpp
 * @brief Concrete implementation of tag business logic and subject-tag associations.
 */

#ifndef __OPERATION_TAG_HPP
#define __OPERATION_TAG_HPP

#include "operations/i_operation_tag.hpp"

/**
 * @class OperationTag
 * @brief Coordinates tag creation, unique validations, user authorization, and subject tagging.
 */
class OperationTag : public IOperationTag
{
public:
    /**
     * @brief Constructs an OperationTag instance.
     */
    OperationTag();

    /**
     * @brief Destructs the OperationTag instance.
     */
    ~OperationTag() override;

    /**
     * @brief Adds a new tag for the authenticated user.
     * @param user Pointer to authenticated user.
     * @param tag Tag object to insert.
     * @return rst_code_e RST_OK on success, TAG_DUPLICATED if present, or error code.
     */
    rst_code_e tag_add(const std::shared_ptr<const User> &user, Tag &tag) override;

    /**
     * @brief Updates an existing tag.
     * @param user Pointer to authenticated user.
     * @param tag Tag object with updated parameters.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e tag_update(const std::shared_ptr<const User> &user, const Tag &tag) override;

    /**
     * @brief Removes a tag by ID.
     * @param user Pointer to authenticated user.
     * @param id Unique tag identifier.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e tag_remove(const std::shared_ptr<const User> &user, unsigned int id) override;

    /**
     * @brief Retrieves a tag by ID for the authenticated user.
     * @param user Pointer to authenticated user.
     * @param id Unique tag identifier.
     * @param tag Output shared pointer receiving Tag.
     * @return rst_code_e RST_OK on success, TAG_NOT_FOUND, or error code.
     */
    rst_code_e tag_get_by_id(const std::shared_ptr<const User> &user, unsigned int id, std::shared_ptr<Tag> &tag) override;

    /**
     * @brief Retrieves a tag by name for the authenticated user.
     * @param user Pointer to authenticated user.
     * @param name Tag name string.
     * @param tag Output shared pointer receiving Tag.
     * @return rst_code_e RST_OK on success, TAG_NOT_FOUND, or error code.
     */
    rst_code_e tag_get_by_name(const std::shared_ptr<const User> &user, const std::string &name, std::shared_ptr<Tag> &tag) override;

    /**
     * @brief Retrieves all tags belonging to the authenticated user.
     * @param user Pointer to authenticated user.
     * @param tags Output vector receiving matching Tag objects.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e tag_get_all_by_user(const std::shared_ptr<const User> &user, std::vector<std::shared_ptr<Tag>> &tags) override;

    /**
     * @brief Attaches a tag to a subject owned by the authenticated user.
     * @param user Pointer to authenticated user.
     * @param subject_id Subject identifier.
     * @param tag_id Tag identifier.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e subject_add_tag(const std::shared_ptr<const User> &user, unsigned int subject_id, unsigned int tag_id) override;

    /**
     * @brief Detaches a tag from a subject owned by the authenticated user.
     * @param user Pointer to authenticated user.
     * @param subject_id Subject identifier.
     * @param tag_id Tag identifier.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e subject_remove_tag(const std::shared_ptr<const User> &user, unsigned int subject_id, unsigned int tag_id) override;

    /**
     * @brief Retrieves all tags attached to a subject owned by the authenticated user.
     * @param user Pointer to authenticated user.
     * @param subject_id Subject identifier.
     * @param tags Output vector receiving attached Tag objects.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e subject_get_tags(const std::shared_ptr<const User> &user, unsigned int subject_id, std::vector<std::shared_ptr<Tag>> &tags) override;

    /**
     * @brief Retrieves all subjects belonging to the authenticated user labeled with a specific tag.
     * @param user Pointer to authenticated user.
     * @param tag_id Tag identifier.
     * @param subjects Output vector receiving matching Subject objects with their tags populated.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e subject_get_all_by_tag(const std::shared_ptr<const User> &user, unsigned int tag_id, std::vector<std::shared_ptr<Subject>> &subjects) override;
};

#endif // __OPERATION_TAG_HPP
