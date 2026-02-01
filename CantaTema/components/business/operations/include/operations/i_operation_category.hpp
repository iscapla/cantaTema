#ifndef __IOPERATION_CATEGORY_HPP
#define __IOPERATION_CATEGORY_HPP

#include <vector>
#include <memory>

#include "primitives/definitions.hpp"
#include "primitives/category.hpp"
#include "primitives/user.hpp"

class IOperationCategory
{
public:
    virtual ~IOperationCategory() = default;

    virtual rst_code_e category_add(const std::shared_ptr<const User> &user, Category &category) = 0;

    virtual rst_code_e category_update(const std::shared_ptr<const User> &user, Category &category) = 0;

    virtual rst_code_e category_remove(unsigned int id) = 0;

    virtual rst_code_e category_get_by_id(unsigned int id, std::shared_ptr<Category> &category) = 0;

    virtual rst_code_e category_get_all_by_user(const std::shared_ptr<const User> &user, std::vector<std::shared_ptr<Category>> &categories) = 0;
};

#endif //__IOPERATION_CATEGORY_HPP