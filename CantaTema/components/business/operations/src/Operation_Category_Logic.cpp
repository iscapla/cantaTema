#include "operations/Operation_Category_Logic.hpp"

#include "database/db_main.hpp"
#include "database/db_category.hpp"

Operation_Category::Operation_Category()
{
    DB_Main *db_main = DB_Main::getInstance();
}

Operation_Category::~Operation_Category()
{
}

rst_code_e Operation_Category::category_add(Category &category)
{
    DB_Category db_category;
    rst_code_e rst;
    bool already_exists = false;

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

rst_code_e Operation_Category::category_update(const Category &category)
{
    DB_Category db_category;

    std::vector<std::shared_ptr<Category>> categories;
    rst_code_e rst = category_get_all_by_user(category.get_user_id(), categories);
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

rst_code_e Operation_Category::category_remove(unsigned int id)
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

rst_code_e Operation_Category::category_get_by_id(unsigned int id, std::shared_ptr<Category> &category)
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

rst_code_e Operation_Category::category_get_all_by_user(unsigned int user_id, std::vector<std::shared_ptr<Category>> &categories)
{
    DB_Category db_category;

    rst_code_e rst = db_category.get_all_categories_by_user(user_id, categories);

    if (rst)
    {
        logger->warn("Get all categories by user error ({})", get_rst_txt(rst));
        return CATEGORY_ERROR;
    }

    return RST_OK;
}
