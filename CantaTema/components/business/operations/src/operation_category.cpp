/**
 * @file operation_category.cpp
 * @brief Implementation of OperationCategory business logic methods.
 */

#include "operations/operation_category.hpp"

#include "database/db_main.hpp"
#include "database/db_category.hpp"

OperationCategory::OperationCategory()
{
    DB_Main *db_main = DB_Main::getInstance();
}

OperationCategory::~OperationCategory()
{
}

rst_code_e OperationCategory::category_add(const std::shared_ptr<const User> &user, Category &category)
{
    DB_Category db_category;
    rst_code_e rst;
    bool already_exists = false;

    if(user == nullptr || user->get_useraccountid() == 0){
        logger->error("User info error");
        return CATEGORY_ERROR;
    }

    category.set_user_id(user->get_useraccountid());

    rst = db_category.is_category_already_present(category.get_user_id(), category.get_name(), already_exists);
    if (rst)
    {
        logger->error("Error checking for duplicate category");
        return CATEGORY_ERROR;
    }

    if (already_exists)
    {
        logger->warn("Category '{}' already exists.", category.get_name());
        return CATEGORY_DUPLICATED;
    }

    rst = db_category.add_new_category(category);

    if (rst)
    {
        logger->warn("Error when adding a new category: {}", get_rst_txt(rst));
        return CATEGORY_ERROR;
    }

    return RST_OK;
}

rst_code_e OperationCategory::category_update(const std::shared_ptr<const User> &user, Category &category)
{
    DB_Category db_category;

    if(user == nullptr || user->get_useraccountid() == 0){
        logger->error("User info error");
        return CATEGORY_ERROR;
    }

    category.set_user_id(user->get_useraccountid());

    std::vector<std::shared_ptr<Category>> categories;
    rst_code_e rst = category_get_all_by_user(user, categories);
    if (rst != RST_OK)
        return rst;

    for (const auto &existing_cat : categories)
    {
        if (existing_cat->get_name() == category.get_name() && existing_cat->get_id() != category.get_id())
        {
            return CATEGORY_DUPLICATED;
        }
    }

    rst = db_category.update_category(category);

    if (rst)
    {
        logger->warn("Update category error ({})", get_rst_txt(rst));
        return CATEGORY_ERROR;
    }

    return RST_OK;
}

rst_code_e OperationCategory::category_remove(unsigned int id)
{
    DB_Category db_category;

    rst_code_e rst = db_category.remove_category(id);

    if (rst)
    {
        logger->warn("Error when deleting a category: {}", get_rst_txt(rst));
        return CATEGORY_ERROR;
    }

    return RST_OK;
}

rst_code_e OperationCategory::category_get_by_id(unsigned int id, std::shared_ptr<Category> &category)
{
    DB_Category db_category;

    rst_code_e rst = db_category.get_category_by_id(id, category);
    if (rst)
    {
        logger->warn("Get category by id error ({})", get_rst_txt(rst));
        return CATEGORY_NOT_FOUND;
    }

    return RST_OK;
}

rst_code_e OperationCategory::category_get_all_by_user(const std::shared_ptr<const User> &user, std::vector<std::shared_ptr<Category>> &categories)
{
    DB_Category db_category;

    if(user == nullptr || user->get_useraccountid() == 0){
        logger->error("User info error");
        return CATEGORY_ERROR;
    }

    rst_code_e rst = db_category.get_all_categories_by_user(user->get_useraccountid(), categories);

    if (rst)
    {
        logger->warn("Get all categories by user error ({})", get_rst_txt(rst));
        return CATEGORY_ERROR;
    }

    return RST_OK;
}
