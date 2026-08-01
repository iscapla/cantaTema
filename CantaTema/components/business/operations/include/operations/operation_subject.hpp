/**
 * @file operation_subject.hpp
 * @brief Concrete implementation of study subject business logic and file management.
 */

#ifndef __OPERATION_SUBJECT_LOGIC_HPP
#define __OPERATION_SUBJECT_LOGIC_HPP

#include "operations/i_operation_subject.hpp"
#include "operations/i_operation_category.hpp"
#include "operations/i_operation_user_metrics.hpp"

/**
 * @class OperationSubject
 * @brief Coordinates study subject creation, file validation, storage metrics tracking, and category association.
 */
class OperationSubject : public IOperationSubject
{
public:
    /**
     * @brief Constructs an OperationSubject with injected metrics and category operations.
     * @param _user_metrics_op Rvalue reference to user metrics operation instance.
     * @param _category_op Rvalue reference to category operation instance.
     */
    OperationSubject(std::shared_ptr<IOperationUserMetrics> &&_user_metrics_op, std::shared_ptr<IOperationCategory> &&_category_op);

    /**
     * @brief Destructor for OperationSubject.
     */
    ~OperationSubject();

    /**
     * @brief Validates, copies attached source file, and adds subject to database.
     * @param user Pointer to authenticated user.
     * @param source_file Path to input PDF or text document.
     * @param subject Subject object populated with metadata.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e subject_add(const std::shared_ptr<const User> &user, const std::string source_file, Subject &subject) override;

    /**
     * @brief Updates subject metadata record.
     * @param user Pointer to authenticated user.
     * @param subject Subject object with updated attributes.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e subject_update(const std::shared_ptr<const User> &user, const Subject &subject) override;

    /**
     * @brief Removes a subject record and its attached file from storage.
     * @param user Pointer to authenticated user.
     * @param id Unique subject ID.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e subject_remove(const std::shared_ptr<const User> &user, unsigned int id) override;

    /**
     * @brief Retrieves a subject record by ID.
     * @param user Pointer to authenticated user.
     * @param id Unique subject ID.
     * @param subject Output pointer receiving Subject object.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e subject_get_by_id(const std::shared_ptr<const User> &user, unsigned int id, std::shared_ptr<Subject> &subject) override;

    /**
     * @brief Retrieves all subjects matching a category ID.
     * @param user Pointer to authenticated user.
     * @param category_id Unique category ID.
     * @param subjects Output vector receiving Subject objects.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e subject_get_all_by_category(const std::shared_ptr<const User> &user, unsigned int category_id, std::vector<std::shared_ptr<Subject>> &subjects) override;

    /**
     * @brief Retrieves all subjects registered for a user.
     * @param user Pointer to authenticated user.
     * @param subjects Output vector receiving Subject objects.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e subject_get_all_by_user(const std::shared_ptr<const User> &user, std::vector<std::shared_ptr<Subject>> &subjects) override;

private:
    std::shared_ptr<IOperationUserMetrics> user_metrics_op{nullptr};
    std::shared_ptr<IOperationCategory> category_op{nullptr};
};

#endif //__OPERATION_SUBJECT_LOGIC_HPP