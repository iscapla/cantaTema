#include "database/db_subject.hpp"
#include "database/db_connection.hpp"
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
                      "language TEXT NOT NULL DEFAULT 'es',"
                      "FOREIGN KEY(user_id) REFERENCES useraccount(useraccountid) ON DELETE CASCADE,"
                      "FOREIGN KEY(category_id) REFERENCES categories(category_id) ON DELETE SET NULL);";

    int rc = sqlite3_exec(db.get(), sql, 0, 0, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        logger->error("SQL error: {}", zErrMsg);
        sqlite3_free(zErrMsg);
        return rst_code_e::DB_FAIL;
    }

    // Dynamic schema migration: add 'language' column if it's missing from existing DB
    sqlite3_stmt *stmt;
    bool has_language = false;
    if (sqlite3_prepare_v2(db.get(), "PRAGMA table_info(subjects);", -1, &stmt, 0) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const char *col_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            if (col_name && std::string(col_name) == "language")
            {
                has_language = true;
                break;
            }
        }
        sqlite3_finalize(stmt);
    }

    if (!has_language)
    {
        rc = sqlite3_exec(db.get(), "ALTER TABLE subjects ADD COLUMN language TEXT NOT NULL DEFAULT 'es';", 0, 0, &zErrMsg);
        if (rc != SQLITE_OK)
        {
            logger->error("SQL error migrating subjects table: {}", zErrMsg);
            sqlite3_free(zErrMsg);
            return rst_code_e::DB_FAIL;
        }
        logger->info("Migrated 'subjects' table with 'language' column.");
    }

    return rst_code_e::RST_OK;
}

rst_code_e DB_Subject::add_new_subject(Subject &subject) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO subjects (user_id, category_id, name, filepath, language) VALUES (?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, subject.get_user_id());

    if (subject.get_category_id() != 0)
    {
        sqlite3_bind_int(stmt, 2, subject.get_category_id());
    }
    else
    {
        sqlite3_bind_null(stmt, 2);
    }

    sqlite3_bind_text(stmt, 3, subject.get_name().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, subject.get_filepath().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, subject.get_language().c_str(), -1, SQLITE_TRANSIENT);

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
    const char *sql = "UPDATE subjects SET user_id = ?, category_id = ?, name = ?, filepath = ?, language = ? WHERE subjects_id = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, subject.get_user_id());

    if (subject.get_category_id() != 0)
    {
        sqlite3_bind_int(stmt, 2, subject.get_category_id());
    }
    else
    {
        sqlite3_bind_null(stmt, 2);
    }

    sqlite3_bind_text(stmt, 3, subject.get_name().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, subject.get_filepath().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, subject.get_language().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, subject.get_id());

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
    const char *sql = "SELECT subjects_id, user_id, category_id, name, filepath, language FROM subjects WHERE subjects_id = ?;";
    rst_code_e result = rst_code_e::DB_FAIL;

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, id);

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            unsigned int sub_id = sqlite3_column_int(stmt, 0);
            unsigned int user_id = sqlite3_column_int(stmt, 1);
            
            unsigned int category_id = 0;
            if (sqlite3_column_type(stmt, 2) != SQLITE_NULL)
            {
                category_id = sqlite3_column_int(stmt, 2);
            }

            std::string name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            std::string filepath = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
            std::string language = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));

            subject = std::make_shared<Subject>(sub_id, name);
            subject->set_user_id(user_id);
            subject->set_filepath(filepath);
            subject->set_category_id(category_id);
            subject->set_language(language);
            load_tags_for_subject(db.get(), *subject);
            
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
    const char *sql = "SELECT subjects_id, user_id, name, filepath, language "
                      "FROM subjects "
                      "WHERE category_id = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, category_id);

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            unsigned int sub_id = sqlite3_column_int(stmt, 0);
            unsigned int user_id = sqlite3_column_int(stmt, 1);
            std::string name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            std::string filepath = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            std::string language = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));

            auto subject = std::make_shared<Subject>(sub_id, name);
            subject->set_user_id(user_id);
            subject->set_filepath(filepath);
            subject->set_category_id(category_id);
            subject->set_language(language);
            load_tags_for_subject(db.get(), *subject);

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
    const char *sql = "SELECT subjects_id, user_id, category_id, name, filepath, language "
                      "FROM subjects "
                      "WHERE user_id = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, user_id);

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            unsigned int sub_id = sqlite3_column_int(stmt, 0);
            unsigned int u_id = sqlite3_column_int(stmt, 1);
            
            unsigned int category_id = 0;
            if (sqlite3_column_type(stmt, 2) != SQLITE_NULL)
            {
                category_id = sqlite3_column_int(stmt, 2);
            }

            std::string name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            std::string filepath = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
            std::string language = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));

            auto subject = std::make_shared<Subject>(sub_id, name);
            subject->set_user_id(u_id);
            subject->set_filepath(filepath);
            subject->set_category_id(category_id);
            subject->set_language(language);
            load_tags_for_subject(db.get(), *subject);

            subjects.push_back(subject);
        }
    }
    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

void DB_Subject::load_tags_for_subject(sqlite3 *db, Subject &subject) const
{
    if (db == nullptr)
    {
        return;
    }

    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT t.tag_id, t.user_id, t.name FROM tags t "
                      "INNER JOIN subject_tags st ON t.tag_id = st.tag_id "
                      "WHERE st.subject_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, static_cast<int>(subject.get_id()));
        std::vector<Tag> tags;
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            unsigned int t_id = static_cast<unsigned int>(sqlite3_column_int(stmt, 0));
            unsigned int u_id = static_cast<unsigned int>(sqlite3_column_int(stmt, 1));
            const unsigned char *t_name_raw = sqlite3_column_text(stmt, 2);
            std::string t_name = t_name_raw ? reinterpret_cast<const char *>(t_name_raw) : "";

            Tag tag(t_id, t_name);
            tag.set_user_id(u_id);
            tags.push_back(tag);
        }
        subject.set_tags(tags);
        sqlite3_finalize(stmt);
    }
}