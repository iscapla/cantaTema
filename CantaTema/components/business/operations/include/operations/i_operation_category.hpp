/**
 * @file i_operation_category.hpp
 * @brief Abstract interface for study category operations and lifecycle management.
 */

#ifndef __IOPERATION_CATEGORY_HPP
#define __IOPERATION_CATEGORY_HPP

#include <vector>
#include <memory>

#include "primitives/definitions.hpp"
#include "primitives/category.hpp"
#include "primitives/user.hpp"

/**
 * @class IOperationCategory
 * @brief Abstract interface defining category creation, retrieval, update, and removal operations.
 */
class IOperationCategory
{
public:
    /**
     * @brief Virtual destructor for IOperationCategory.
     */
    virtual ~IOperationCategory() = default;

    /**
     * @brief Adds a new study category for the given user.
     * @param user Pointer to authenticated user requesting addition.
     * @param category Reference to Category object containing category name and details.
     * @return rst_code_e RST_OK on success, CATEGORY_DUPLICATED if category exists, or CATEGORY_ERROR.
     */
    virtual rst_code_e category_add(const std::shared_ptr<const User> &user, Category &category) = 0;

    /**
     * @brief Updates an existing study category's properties.
     * @param user Pointer to authenticated user performing update.
     * @param category Category object with updated parameters.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e category_update(const std::shared_ptr<const User> &user, Category &category) = 0;

    /**
     * @brief Removes a study category by its unique identifier.
     * @param id Category unique ID.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e category_remove(unsigned int id) = 0;

    /**
     * @brief Fetches a category record by its unique ID.
     * @param id Category unique ID.
     * @param category Pointer receiving the fetched Category object.
     * @return rst_code_e RST_OK on success, or CATEGORY_NOT_FOUND.
     */
    virtual rst_code_e category_get_by_id(unsigned int id, std::shared_ptr<Category> &category) = 0;

    /**
     * @brief Retrieves all study categories owned by a specific user.
     * @param user Pointer to authenticated user.
     * @param categories Vector receiving shared pointers to user categories.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e category_get_all_by_user(const std::shared_ptr<const User> &user, std::vector<std::shared_ptr<Category>> &categories) = 0;
};

#endif //__IOPERATION_CATEGORY_HPP