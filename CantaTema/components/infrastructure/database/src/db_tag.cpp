/**
 * @file db_tag.cpp
 * @brief Implementation of SQLite database persistence for tags and subject_tags junction table.
 */

#include "database/db_tag.hpp"
#include "database/db_connection.hpp"
#include <sqlite3mc_amalgamation.h>

DB_Tag::DB_Tag(void) {}

rst_code_e DB_Tag::tag_tables_create(void) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    char *zErrMsg = nullptr;

    const char *sql_tags = "CREATE TABLE IF NOT EXISTS tags ("
                           "tag_id INTEGER PRIMARY KEY CHECK(tag_id <> 0),"
                           "user_id INTEGER NOT NULL,"
                           "name TEXT NOT NULL,"
                           "FOREIGN KEY(user_id) REFERENCES useraccount(useraccountid) ON DELETE CASCADE,"
                           "UNIQUE(user_id, name));";

    int rc = sqlite3_exec(db.get(), sql_tags, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        logger->error("SQL error creating tags table: {}", zErrMsg ? zErrMsg : "unknown");
        sqlite3_free(zErrMsg);
        return rst_code_e::DB_FAIL;
    }

    const char *sql_subject_tags = "CREATE TABLE IF NOT EXISTS subject_tags ("
                                   "subject_id INTEGER NOT NULL,"
                                   "tag_id INTEGER NOT NULL,"
                                   "PRIMARY KEY (subject_id, tag_id),"
                                   "FOREIGN KEY(subject_id) REFERENCES subjects(subjects_id) ON DELETE CASCADE,"
                                   "FOREIGN KEY(tag_id) REFERENCES tags(tag_id) ON DELETE CASCADE);";

    rc = sqlite3_exec(db.get(), sql_subject_tags, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        logger->error("SQL error creating subject_tags table: {}", zErrMsg ? zErrMsg : "unknown");
        sqlite3_free(zErrMsg);
        return rst_code_e::DB_FAIL;
    }

    return rst_code_e::RST_OK;
}

rst_code_e DB_Tag::is_tag_already_present(unsigned int user_id, const std::string &name, bool &exists) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT 1 FROM tags WHERE user_id = ? AND name = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, static_cast<int>(user_id));
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

rst_code_e DB_Tag::add_new_tag(Tag &tag) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "INSERT INTO tags (user_id, name) VALUES (?, ?);";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, static_cast<int>(tag.get_user_id()));
    sqlite3_bind_text(stmt, 2, tag.get_name().c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        logger->error("Failed to execute statement: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return rst_code_e::DB_FAIL;
    }

    tag.set_id(static_cast<unsigned int>(sqlite3_last_insert_rowid(db.get())));
    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_Tag::update_tag(const Tag &tag) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "UPDATE tags SET name = ?, user_id = ? WHERE tag_id = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_text(stmt, 1, tag.get_name().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, static_cast<int>(tag.get_user_id()));
    sqlite3_bind_int(stmt, 3, static_cast<int>(tag.get_id()));

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        logger->error("Failed to execute statement: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return rst_code_e::DB_FAIL;
    }

    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_Tag::remove_tag(unsigned int id) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt = nullptr;

    // Remove junction entries first
    const char *sql_junction = "DELETE FROM subject_tags WHERE tag_id = ?;";
    if (sqlite3_prepare_v2(db.get(), sql_junction, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, static_cast<int>(id));
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    const char *sql = "DELETE FROM tags WHERE tag_id = ?;";
    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, static_cast<int>(id));

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        logger->error("Failed to execute statement: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return rst_code_e::DB_FAIL;
    }

    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_Tag::remove_all_tags_from_user(unsigned int user_id) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt = nullptr;

    const char *sql_junction = "DELETE FROM subject_tags WHERE tag_id IN (SELECT tag_id FROM tags WHERE user_id = ?);";
    if (sqlite3_prepare_v2(db.get(), sql_junction, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, static_cast<int>(user_id));
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    const char *sql = "DELETE FROM tags WHERE user_id = ?;";
    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, static_cast<int>(user_id));

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        logger->error("Failed to execute statement: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return rst_code_e::DB_FAIL;
    }

    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_Tag::get_tag_by_id(unsigned int id, std::shared_ptr<Tag> &tag) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT tag_id, user_id, name FROM tags WHERE tag_id = ?;";
    rst_code_e result = rst_code_e::DB_FAIL;

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, static_cast<int>(id));

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            unsigned int t_id = static_cast<unsigned int>(sqlite3_column_int(stmt, 0));
            unsigned int u_id = static_cast<unsigned int>(sqlite3_column_int(stmt, 1));
            std::string name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));

            tag = std::make_shared<Tag>(t_id, name);
            tag->set_user_id(u_id);
            result = rst_code_e::RST_OK;
        }
    }
    sqlite3_finalize(stmt);
    return result;
}

rst_code_e DB_Tag::get_tag_by_name(unsigned int user_id, const std::string &name, std::shared_ptr<Tag> &tag) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT tag_id, user_id, name FROM tags WHERE user_id = ? AND name = ?;";
    rst_code_e result = rst_code_e::DB_FAIL;

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, static_cast<int>(user_id));
        sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            unsigned int t_id = static_cast<unsigned int>(sqlite3_column_int(stmt, 0));
            unsigned int u_id = static_cast<unsigned int>(sqlite3_column_int(stmt, 1));
            std::string tag_name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));

            tag = std::make_shared<Tag>(t_id, tag_name);
            tag->set_user_id(u_id);
            result = rst_code_e::RST_OK;
        }
    }
    sqlite3_finalize(stmt);
    return result;
}

rst_code_e DB_Tag::get_all_tags_by_user(unsigned int user_id, std::vector<std::shared_ptr<Tag>> &tags) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT tag_id, user_id, name FROM tags WHERE user_id = ? ORDER BY tag_id ASC;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, static_cast<int>(user_id));

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            unsigned int t_id = static_cast<unsigned int>(sqlite3_column_int(stmt, 0));
            unsigned int u_id = static_cast<unsigned int>(sqlite3_column_int(stmt, 1));
            std::string name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));

            auto tag = std::make_shared<Tag>(t_id, name);
            tag->set_user_id(u_id);
            tags.push_back(tag);
        }
    }
    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_Tag::subject_add_tag(unsigned int subject_id, unsigned int tag_id) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "INSERT OR IGNORE INTO subject_tags (subject_id, tag_id) VALUES (?, ?);";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, static_cast<int>(subject_id));
    sqlite3_bind_int(stmt, 2, static_cast<int>(tag_id));

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        logger->error("Failed to execute statement: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return rst_code_e::DB_FAIL;
    }

    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_Tag::subject_remove_tag(unsigned int subject_id, unsigned int tag_id) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "DELETE FROM subject_tags WHERE subject_id = ? AND tag_id = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, static_cast<int>(subject_id));
    sqlite3_bind_int(stmt, 2, static_cast<int>(tag_id));

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        logger->error("Failed to execute statement: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return rst_code_e::DB_FAIL;
    }

    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_Tag::subject_remove_all_tags(unsigned int subject_id) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "DELETE FROM subject_tags WHERE subject_id = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, static_cast<int>(subject_id));

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        logger->error("Failed to execute statement: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return rst_code_e::DB_FAIL;
    }

    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_Tag::get_tags_by_subject(unsigned int subject_id, std::vector<std::shared_ptr<Tag>> &tags) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT t.tag_id, t.user_id, t.name "
                      "FROM tags t "
                      "JOIN subject_tags st ON t.tag_id = st.tag_id "
                      "WHERE st.subject_id = ? "
                      "ORDER BY t.tag_id ASC;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, static_cast<int>(subject_id));

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            unsigned int t_id = static_cast<unsigned int>(sqlite3_column_int(stmt, 0));
            unsigned int u_id = static_cast<unsigned int>(sqlite3_column_int(stmt, 1));
            std::string name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));

            auto tag = std::make_shared<Tag>(t_id, name);
            tag->set_user_id(u_id);
            tags.push_back(tag);
        }
    }
    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_Tag::get_subjects_by_tag(unsigned int user_id, unsigned int tag_id, std::vector<std::shared_ptr<Subject>> &subjects) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT s.subjects_id, s.user_id, s.category_id, s.name, s.filepath, s.language "
                      "FROM subjects s "
                      "JOIN subject_tags st ON s.subjects_id = st.subject_id "
                      "WHERE st.tag_id = ? AND s.user_id = ? "
                      "ORDER BY s.subjects_id ASC;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, static_cast<int>(tag_id));
        sqlite3_bind_int(stmt, 2, static_cast<int>(user_id));

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            unsigned int sub_id = static_cast<unsigned int>(sqlite3_column_int(stmt, 0));
            unsigned int u_id = static_cast<unsigned int>(sqlite3_column_int(stmt, 1));
            unsigned int cat_id = 0;
            if (sqlite3_column_type(stmt, 2) != SQLITE_NULL)
            {
                cat_id = static_cast<unsigned int>(sqlite3_column_int(stmt, 2));
            }
            std::string name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            std::string filepath = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
            std::string language = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));

            auto subject = std::make_shared<Subject>(sub_id, name);
            subject->set_user_id(u_id);
            subject->set_category_id(cat_id);
            subject->set_filepath(filepath);
            subject->set_language(language);

            subjects.push_back(subject);
        }
    }
    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}
