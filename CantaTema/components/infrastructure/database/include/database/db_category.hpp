#ifndef __DB_CATEGORY_HPP
#define __DB_CATEGORY_HPP

#include <string>
#include <vector>
#include <memory>

#include "primitives/definitions.hpp"
#include "primitives/utils_logger.hpp"
#include "primitives/category.hpp"

class DB_Category
{
public:
    /**
     * @brief Construct a new DB_Category object
     *
     */
    DB_Category(void);

    /**
     * @brief Create Category table on database
     *
     * @return rst_code_e
     */
    rst_code_e category_tables_create(void) const;

    /**
     * @brief Check if a category already exists for a specific user.
     *
     * @param user_id
     * @param name
     * @param exists
     * @return rst_code_e
     */
    rst_code_e is_category_already_present(unsigned int user_id, const std::string &name, bool &exists) const;

    /**
     * @brief Add new category to the DB. Sets the category object with its new identifier.
     *
     * @param category
     * @return rst_code_e
     */
    rst_code_e add_new_category(Category &category) const;

    /**
     * @brief Update existing category in the DB.
     *
     * @param category
     * @return rst_code_e
     */
    rst_code_e update_category(const Category &category) const;

    /**
     * @brief Remove category by ID.
     *
     * @param id
     * @return rst_code_e
     */
    rst_code_e remove_category(unsigned int id) const;

    /**
     * @brief Remove all categories associated with a specific user.
     *
     * @param user_id
     * @return rst_code_e
     */
    rst_code_e remove_all_categories_from_user(unsigned int user_id) const;

    

    /**
     * @brief Retrieve a category by its ID.
     *
     * @param id
     * @param category Output shared pointer to the category
     * @return rst_code_e
     */
    rst_code_e get_category_by_id(unsigned int id, std::shared_ptr<Category> &category) const;

    rst_code_e get_all_categories_by_user(unsigned int user_id, std::vector<std::shared_ptr<Category>> &categories) const;
};

#endif //__DB_CATEGORY_HPP