#ifndef __OPERATION_CATEGORY_LOGIC_HPP
#define __OPERATION_CATEGORY_LOGIC_HPP

#include "operations/i_operation_category.hpp"

class OperationCategory : public IOperationCategory
{
public:
    // Default constructor
    OperationCategory();
    ~OperationCategory();

    rst_code_e category_add(const std::shared_ptr<const User> &user, Category &category) override;

    rst_code_e category_update(const std::shared_ptr<const User> &user, Category &category) override;

    rst_code_e category_remove(unsigned int id) override;

    rst_code_e category_get_by_id(unsigned int id, std::shared_ptr<Category> &category) override;

    rst_code_e category_get_all_by_user(const std::shared_ptr<const User> &user, std::vector<std::shared_ptr<Category>> &categories) override;
};

#endif //__OPERATION_CATEGORY_LOGIC_HPP