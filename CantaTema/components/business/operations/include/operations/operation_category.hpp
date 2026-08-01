/**
 * @file operation_category.hpp
 * @brief Concrete implementation of category business logic operations.
 */

#ifndef __OPERATION_CATEGORY_LOGIC_HPP
#define __OPERATION_CATEGORY_LOGIC_HPP

#include "operations/i_operation_category.hpp"

/**
 * @class OperationCategory
 * @brief Handles validation and database persistence coordination for category operations.
 */
class OperationCategory : public IOperationCategory
{
public:
    /**
     * @brief Constructs an OperationCategory instance.
     */
    OperationCategory();

    /**
     * @brief Destructs the OperationCategory instance.
     */
    ~OperationCategory();

    /**
     * @brief Adds a new study category for a user after checking for duplicates.
     * @param user Pointer to authenticated user.
     * @param category Category instance to be stored.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e category_add(const std::shared_ptr<const User> &user, Category &category) override;

    /**
     * @brief Updates an existing study category record.
     * @param user Pointer to authenticated user.
     * @param category Category instance with updated data.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e category_update(const std::shared_ptr<const User> &user, Category &category) override;

    /**
     * @brief Removes a category record by ID.
     * @param id Unique identifier of the category to remove.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e category_remove(unsigned int id) override;

    /**
     * @brief Queries a category record by ID.
     * @param id Category ID.
     * @param category Shared pointer receiving Category object.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e category_get_by_id(unsigned int id, std::shared_ptr<Category> &category) override;

    /**
     * @brief Retrieves all categories owned by a given user.
     * @param user User account pointer.
     * @param categories Vector populated with matching Category objects.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e category_get_all_by_user(const std::shared_ptr<const User> &user, std::vector<std::shared_ptr<Category>> &categories) override;
};

#endif //__OPERATION_CATEGORY_LOGIC_HPP