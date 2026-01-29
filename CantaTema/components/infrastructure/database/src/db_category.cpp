#include "database/db_category.hpp"
#include "database/db_connection.hpp"
#include <sqlite3mc_amalgamation.h>

DB_Category::DB_Category(void) {}

rst_code_e DB_Category::category_tables_create(void) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    char *zErrMsg = 0;
    const char *sql = "CREATE TABLE IF NOT EXISTS categories ("
                      "category_id INTEGER PRIMARY KEY CHECK(category_id <> 0),"
                      "user_id INTEGER NOT NULL,"
                      "name TEXT NOT NULL,"
                      "FOREIGN KEY(user_id) REFERENCES useraccount(useraccountid) ON DELETE CASCADE);";

    int rc = sqlite3_exec(db.get(), sql, 0, 0, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        logger->error("SQL error: {}", zErrMsg);
        sqlite3_free(zErrMsg);
        return rst_code_e::DB_FAIL;
    }
    return rst_code_e::RST_OK;
}

rst_code_e DB_Category::is_category_already_present(unsigned int user_id, const std::string &name, bool &exists) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = "SELECT 1 FROM categories WHERE user_id = ? AND name = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        exists = true;
    }
    else if (rc == SQLITE_DONE)
    {
        exists = false;
    }
    else
    {
        logger->error("Failed to execute statement: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return rst_code_e::DB_FAIL;
    }

    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_Category::add_new_category(Category &category) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO categories (user_id, name) VALUES (?, ?);";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, category.get_user_id());
    sqlite3_bind_text(stmt, 2, category.get_name().c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        logger->error("Failed to execute statement: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return rst_code_e::DB_FAIL;
    }

    category.set_id(static_cast<unsigned int>(sqlite3_last_insert_rowid(db.get())));
    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_Category::update_category(const Category &category) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE categories SET name = ?, user_id = ? WHERE category_id = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_text(stmt, 1, category.get_name().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, category.get_user_id());
    sqlite3_bind_int(stmt, 3, category.get_id());

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        logger->error("Failed to execute statement: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return rst_code_e::DB_FAIL;
    }

    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_Category::remove_category(unsigned int id) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;

    // First, set category_id to NULL for all subjects associated with this category
    const char *sql_update = "UPDATE subjects SET category_id = NULL WHERE category_id = ?;";
    if (sqlite3_prepare_v2(db.get(), sql_update, -1, &stmt, 0) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        logger->error("Failed to execute statement: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return rst_code_e::DB_FAIL;
    }
    sqlite3_finalize(stmt);

    const char *sql = "DELETE FROM categories WHERE category_id = ?;";
    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        logger->error("Failed to execute statement: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return rst_code_e::DB_FAIL;
    }

    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_Category::remove_all_categories_from_user(unsigned int user_id) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;

    // First, set category_id to NULL for all subjects associated with categories of this user
    const char *sql_update = "UPDATE subjects SET category_id = NULL WHERE category_id IN (SELECT category_id FROM categories WHERE user_id = ?);";
    if (sqlite3_prepare_v2(db.get(), sql_update, -1, &stmt, 0) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }
    sqlite3_bind_int(stmt, 1, user_id);
    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        logger->error("Failed to execute statement: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return rst_code_e::DB_FAIL;
    }
    sqlite3_finalize(stmt);

    // Now delete the categories
    const char *sql = "DELETE FROM categories WHERE user_id = ?;";
    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        logger->error("Failed to execute statement: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return rst_code_e::DB_FAIL;
    }

    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_Category::get_category_by_id(unsigned int id, std::shared_ptr<Category> &category) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = "SELECT category_id, user_id, name FROM categories WHERE category_id = ?;";
    rst_code_e result = rst_code_e::DB_FAIL;

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, id);

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            unsigned int cat_id = sqlite3_column_int(stmt, 0);
            unsigned int user_id = sqlite3_column_int(stmt, 1);
            std::string name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));

            category = std::make_shared<Category>(cat_id, name);
            category->set_user_id(user_id);
            result = rst_code_e::RST_OK;
        }
    }
    sqlite3_finalize(stmt);
    return result;
}

rst_code_e DB_Category::get_all_categories_by_user(unsigned int user_id, std::vector<std::shared_ptr<Category>> &categories) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = "SELECT category_id, user_id, name FROM categories WHERE user_id = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, user_id);

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            unsigned int cat_id = sqlite3_column_int(stmt, 0);
            unsigned int u_id = sqlite3_column_int(stmt, 1);
            std::string name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));

            auto category = std::make_shared<Category>(cat_id, name);
            category->set_user_id(u_id);
            categories.push_back(category);
        }
    }
    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}