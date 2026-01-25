#ifndef __OPERATION_CATEGORY_LOGIC_HPP
#define __OPERATION_CATEGORY_LOGIC_HPP

#include "operations/IOperation_Category.hpp"

class Operation_Category : public IOperation_Category
{
public:
    // Default constructor
    Operation_Category();
    ~Operation_Category();

    rst_code_e category_add(Category &category) override;

    rst_code_e category_update(const Category &category) override;

    rst_code_e category_remove(unsigned int id) override;

    rst_code_e category_get_by_id(unsigned int id, std::shared_ptr<Category> &category) override;

    rst_code_e category_get_all_by_user(unsigned int user_id, std::vector<std::shared_ptr<Category>> &categories) override;
};

#endif //__OPERATION_CATEGORY_LOGIC_HPP