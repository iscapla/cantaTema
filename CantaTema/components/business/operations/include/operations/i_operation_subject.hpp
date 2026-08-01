/**
 * @file i_operation_subject.hpp
 * @brief Abstract interface for subject management operations.
 */

#ifndef __IOPERATION_SUBJECT_HPP
#define __IOPERATION_SUBJECT_HPP

#include <vector>
#include <memory>
#include <string>

#include "primitives/definitions.hpp"
#include "primitives/user.hpp"
#include "primitives/subject.hpp"

/**
 * @class IOperationSubject
 * @brief Abstract interface defining subject creation, file attachment, updating, and querying operations.
 */
class IOperationSubject
{
public:
    /**
     * @brief Virtual destructor for IOperationSubject.
     */
    virtual ~IOperationSubject() = default;

    /**
     * @brief Adds a new study subject with attached reference document.
     * @param user Pointer to authenticated user.
     * @param source_file Filepath to reference text or PDF document.
     * @param subject Reference to Subject primitive object.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e subject_add(const std::shared_ptr<const User> &user, const std::string source_file, Subject &subject) = 0;

    /**
     * @brief Updates subject metadata.
     * @param user Pointer to authenticated user.
     * @param subject Subject object with updated attributes.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e subject_update(const std::shared_ptr<const User> &user, const Subject &subject) = 0;

    /**
     * @brief Removes a subject record by ID.
     * @param user Pointer to authenticated user.
     * @param id Subject unique ID.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e subject_remove(const std::shared_ptr<const User> &user, unsigned int id) = 0;

    /**
     * @brief Retrieves a subject record by ID.
     * @param user Pointer to authenticated user.
     * @param id Subject unique ID.
     * @param subject Output pointer receiving Subject object.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e subject_get_by_id(const std::shared_ptr<const User> &user, unsigned int id, std::shared_ptr<Subject> &subject) = 0;

    /**
     * @brief Retrieves all subjects associated with a specific category ID.
     * @param user Pointer to authenticated user.
     * @param category_id Unique category ID.
     * @param subjects Output vector receiving matching Subject objects.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e subject_get_all_by_category(const std::shared_ptr<const User> &user, unsigned int category_id, std::vector<std::shared_ptr<Subject>> &subjects) = 0;

    /**
     * @brief Retrieves all subjects registered under a user.
     * @param user Pointer to authenticated user.
     * @param subjects Output vector receiving matching Subject objects.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e subject_get_all_by_user(const std::shared_ptr<const User> &user, std::vector<std::shared_ptr<Subject>> &subjects) = 0;
};

#endif //__IOPERATION_SUBJECT_HPP