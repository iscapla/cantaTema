#include "database/db_subject.hpp"
#include "database/db_connection.hpp"
#include "database/db_category.hpp"
#include <sqlite3mc_amalgamation.h>

DB_Subject::DB_Subject(void) {}

rst_code_e DB_Subject::subject_tables_create(void) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    char *zErrMsg = 0;
    const char *sql = "CREATE TABLE IF NOT EXISTS subjects ("
                      "subjects_id INTEGER PRIMARY KEY CHECK(subjects_id <> 0),"
                      "user_id INTEGER NOT NULL,"
                      "category_id INTEGER,"
                      "name TEXT NOT NULL,"
                      "filepath TEXT NOT NULL,"
                      "FOREIGN KEY(user_id) REFERENCES useraccount(useraccountid) ON DELETE CASCADE,"
                      "FOREIGN KEY(category_id) REFERENCES categories(category_id) ON DELETE SET NULL);";

    int rc = sqlite3_exec(db.get(), sql, 0, 0, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        logger->error("SQL error: {}", zErrMsg);
        sqlite3_free(zErrMsg);
        return rst_code_e::DB_FAIL;
    }
    return rst_code_e::RST_OK;
}

rst_code_e DB_Subject::add_new_subject(Subject &subject) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO subjects (user_id, category_id, name, filepath) VALUES (?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, subject.get_user_id());

    if (subject.get_category())
    {
        sqlite3_bind_int(stmt, 2, subject.get_category()->get_id());
    }
    else
    {
        sqlite3_bind_null(stmt, 2);
    }

    sqlite3_bind_text(stmt, 3, subject.get_name().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, subject.get_filepath().c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        logger->error("Failed to execute statement: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return rst_code_e::DB_FAIL;
    }

    subject.set_id(static_cast<unsigned int>(sqlite3_last_insert_rowid(db.get())));
    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_Subject::update_subject(const Subject &subject) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE subjects SET user_id = ?, category_id = ?, name = ?, filepath = ? WHERE subjects_id = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, subject.get_user_id());

    if (subject.get_category())
    {
        sqlite3_bind_int(stmt, 2, subject.get_category()->get_id());
    }
    else
    {
        sqlite3_bind_null(stmt, 2);
    }

    sqlite3_bind_text(stmt, 3, subject.get_name().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, subject.get_filepath().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, subject.get_id());

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        logger->error("Failed to execute statement: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return rst_code_e::DB_FAIL;
    }

    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_Subject::remove_subject(unsigned int id) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM subjects WHERE subjects_id = ?;";

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

rst_code_e DB_Subject::remove_all_subjects_from_user(unsigned int user_id) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM subjects WHERE user_id = ?;";

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

rst_code_e DB_Subject::get_subject_by_id(unsigned int id, std::shared_ptr<Subject> &subject) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = "SELECT subjects_id, user_id, category_id, name, filepath FROM subjects WHERE subjects_id = ?;";
    rst_code_e result = rst_code_e::DB_FAIL;

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, id);

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            unsigned int sub_id = sqlite3_column_int(stmt, 0);
            unsigned int user_id = sqlite3_column_int(stmt, 1);
            
            unsigned int category_id = 0;
            bool has_category = false;
            if (sqlite3_column_type(stmt, 2) != SQLITE_NULL)
            {
                category_id = sqlite3_column_int(stmt, 2);
                has_category = true;
            }

            std::string name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            std::string filepath = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));

            subject = std::make_shared<Subject>(sub_id, name);
            subject->set_user_id(user_id);
            subject->set_filepath(filepath);

            if (has_category)
            {
                DB_Category db_category;
                std::shared_ptr<Category> category;
                if (db_category.get_category_by_id(category_id, category) == rst_code_e::RST_OK)
                {
                    subject->set_category(category);
                }
            }
            else
            {
                subject->set_category(nullptr);
            }
            
            result = rst_code_e::RST_OK;
        }
    }
    sqlite3_finalize(stmt);
    return result;
}

rst_code_e DB_Subject::get_all_subjects_by_category(unsigned int category_id, std::vector<std::shared_ptr<Subject>> &subjects) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = "SELECT s.subjects_id, s.user_id, s.name, s.filepath, "
                      "c.category_id, c.name, c.user_id "
                      "FROM subjects s "
                      "LEFT JOIN categories c ON s.category_id = c.category_id "
                      "WHERE s.category_id = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, category_id);

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            unsigned int sub_id = sqlite3_column_int(stmt, 0);
            unsigned int user_id = sqlite3_column_int(stmt, 1);
            std::string name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            std::string filepath = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));

            auto subject = std::make_shared<Subject>(sub_id, name);
            subject->set_user_id(user_id);
            subject->set_filepath(filepath);

            if (sqlite3_column_type(stmt, 4) != SQLITE_NULL)
            {
                unsigned int cat_id = sqlite3_column_int(stmt, 4);
                std::string cat_name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
                unsigned int cat_user_id = sqlite3_column_int(stmt, 6);

                auto category = std::make_shared<Category>(cat_id, cat_name);
                category->set_user_id(cat_user_id);
                subject->set_category(category);
            }
            else
            {
                subject->set_category(nullptr);
            }

            subjects.push_back(subject);
        }
    }
    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_Subject::get_all_subjects_by_user(unsigned int user_id, std::vector<std::shared_ptr<Subject>> &subjects) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = "SELECT s.subjects_id, s.user_id, s.name, s.filepath, "
                      "c.category_id, c.name, c.user_id "
                      "FROM subjects s "
                      "LEFT JOIN categories c ON s.category_id = c.category_id "
                      "WHERE s.user_id = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, user_id);

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            unsigned int sub_id = sqlite3_column_int(stmt, 0);
            unsigned int u_id = sqlite3_column_int(stmt, 1);
            std::string name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            std::string filepath = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));

            auto subject = std::make_shared<Subject>(sub_id, name);
            subject->set_user_id(u_id);
            subject->set_filepath(filepath);

            if (sqlite3_column_type(stmt, 4) != SQLITE_NULL)
            {
                unsigned int cat_id = sqlite3_column_int(stmt, 4);
                std::string cat_name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
                unsigned int cat_user_id = sqlite3_column_int(stmt, 6);

                auto category = std::make_shared<Category>(cat_id, cat_name);
                category->set_user_id(cat_user_id);
                subject->set_category(category);
            }
            else
            {
                subject->set_category(nullptr);
            }

            subjects.push_back(subject);
        }
    }
    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}