
#include <exception>
#include <chrono>
#include <iostream>
#include "database/db_connection.hpp"
#include "database/db_user.hpp"

#include "primitives/utils_functions.hpp"

DB_User::DB_User(void)
{
}

/**
 * @brief Create User table on database
 *
 * @return rst_code_e
 */
rst_code_e DB_User::user_tables_create(void) const
{
    // Retrieve the SQLite connection
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();

    // SQLite adaptation:
    // 1. No "CREATE SCHEMA". Tables are in the main database.
    // 2. No "ENUM". Used TEXT instead.
    // 3. "SERIAL" -> "INTEGER PRIMARY KEY AUTOINCREMENT".
    // 4. "timestamp" -> "INTEGER" (as requested).

    const char *sql_table =
        "CREATE TABLE IF NOT EXISTS useraccount ("
        "useraccountid INTEGER PRIMARY KEY CHECK(useraccountid <> 0),"
        "name TEXT UNIQUE NOT NULL DEFAULT '',"
        "passwordkey TEXT NOT NULL,"
        "passwordsalt TEXT DEFAULT null,"
        "resettoken TEXT DEFAULT null,"
        "resetexpiration INTEGER DEFAULT null,"
        "status TEXT NOT NULL DEFAULT 'ACTIVE',"
        "creationdate INTEGER NOT NULL,"
        "locknotes TEXT DEFAULT '',"
        "workemail TEXT NOT NULL DEFAULT '',"
        "recoveryemail TEXT DEFAULT null,"
        "firstname TEXT NOT NULL DEFAULT '',"
        "lastname TEXT NOT NULL DEFAULT '',"
        "roleid INTEGER NOT NULL,"
        "max_space_size_in_kb INTEGER DEFAULT 0"
        ");";

    char *zErrMsg = nullptr;
    int rc = sqlite3_exec(db.get(), sql_table, nullptr, nullptr, &zErrMsg);

    if (rc != SQLITE_OK) {
        logger->error("Error creating user table: {}", zErrMsg);
        sqlite3_free(zErrMsg);
        return DB_FAIL;
    }

    return RST_OK;
}

rst_code_e DB_User::is_user_already_present(const std::string &name, bool &already_exists)
{

    if (name.empty())
    {
        logger->debug("User name must not be null when accessing database");
        return DB_BAD_PARAM;
    }

    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT 1 FROM useraccount WHERE name = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        logger->error("Error preparing is_user_already_present: {}", sqlite3_errmsg(db.get()));
        return DB_FAIL;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    already_exists = (rc == SQLITE_ROW);

    sqlite3_finalize(stmt);

    return RST_OK;
}

rst_code_e DB_User::add_new_user(User &user) const
{
    return update_user(user);
}

rst_code_e DB_User::update_user(User &user) const
{
    if (user.get_name().empty())
    {
        logger->debug("User name must not be null when accessing database");
        return DB_BAD_PARAM;
    }

    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt = nullptr;

    try
    {
        // Use SQLite UPSERT syntax
        const char *sql = "INSERT INTO useraccount (name, passwordkey, passwordsalt, resettoken, resetexpiration, status, creationdate, locknotes, workemail, recoveryemail, firstname, lastname, roleid, max_space_size_in_kb) "
                          "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
                          "ON CONFLICT (name) DO UPDATE SET "
                          "passwordkey = excluded.passwordkey, "
                          "passwordsalt = excluded.passwordsalt, "
                          "resettoken = excluded.resettoken, "
                          "resetexpiration = excluded.resetexpiration, "
                          "status = excluded.status, "
                          "creationdate = excluded.creationdate, "
                          "locknotes = excluded.locknotes, "
                          "workemail = excluded.workemail, "
                          "recoveryemail = excluded.recoveryemail, "
                          "firstname = excluded.firstname, "
                          "lastname = excluded.lastname, "
                          "roleid = excluded.roleid, "
                          "max_space_size_in_kb = excluded.max_space_size_in_kb "
                          "RETURNING useraccountid;";

        if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
            logger->error("Error preparing update_user: {}", sqlite3_errmsg(db.get()));
            return DB_FAIL;
        }

        int idx = 1;
        sqlite3_bind_text(stmt, idx++, user.get_name().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, idx++, user.get_passwordkey().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, idx++, user.get_passwordsalt().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, idx++, user.get_resettoken().c_str(), -1, SQLITE_TRANSIENT);

        if (user.get_resetexpiration() == 0) sqlite3_bind_null(stmt, idx++);
        else sqlite3_bind_int64(stmt, idx++, user.get_resetexpiration());

        std::string status_str = user.parse_status_to_string(user.get_status());
        sqlite3_bind_text(stmt, idx++, status_str.c_str(), -1, SQLITE_TRANSIENT);

        sqlite3_bind_int64(stmt, idx++, user.get_creationdate());
        sqlite3_bind_text(stmt, idx++, user.get_locknotes().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, idx++, user.get_workemail().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, idx++, user.get_recoveryemail().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, idx++, user.get_firstname().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, idx++, user.get_lastname().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, idx++, user.get_roleid());
        sqlite3_bind_int(stmt, idx++, user.get_max_space_size_in_kb());

        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            user.set_useraccountid(sqlite3_column_int(stmt, 0));
        } else {
            logger->warn("User update did not return an ID. RC: {}", rc);
            sqlite3_finalize(stmt);
            return DB_NOT_FOUND;
        }

        sqlite3_finalize(stmt);
    }
    catch (const std::exception &e)
    {
        if (stmt) sqlite3_finalize(stmt);
        logger->error("Error database update user: {}", e.what());
        return DB_FAIL;
    }

    return RST_OK;
}

rst_code_e DB_User::remove_user(const std::string &user_name) const
{
    if (user_name.empty())
    {
        logger->debug("User must not be null when accessing database");
        return DB_BAD_PARAM;
    }

    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt = nullptr;

    // Delete subjects associated with the user
    const char *sql_sub = "DELETE FROM subjects WHERE user_id = (SELECT useraccountid FROM useraccount WHERE name = ?);";
    if (sqlite3_prepare_v2(db.get(), sql_sub, -1, &stmt, nullptr) != SQLITE_OK) {
        logger->error("Error preparing delete subjects for remove_user: {}", sqlite3_errmsg(db.get()));
        return DB_FAIL;
    }
    sqlite3_bind_text(stmt, 1, user_name.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        logger->error("Error executing delete subjects for remove_user: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return DB_FAIL;
    }
    sqlite3_finalize(stmt);

    // Delete categories associated with the user
    const char *sql_cat = "DELETE FROM categories WHERE user_id = (SELECT useraccountid FROM useraccount WHERE name = ?);";
    if (sqlite3_prepare_v2(db.get(), sql_cat, -1, &stmt, nullptr) != SQLITE_OK) {
        logger->error("Error preparing delete categories for remove_user: {}", sqlite3_errmsg(db.get()));
        return DB_FAIL;
    }
    sqlite3_bind_text(stmt, 1, user_name.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        logger->error("Error executing delete categories for remove_user: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return DB_FAIL;
    }
    sqlite3_finalize(stmt);

    const char *sql = "DELETE FROM useraccount WHERE name = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        logger->error("Error preparing remove_user: {}", sqlite3_errmsg(db.get()));
        return DB_FAIL;
    }

    sqlite3_bind_text(stmt, 1, user_name.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        logger->error("Error executing remove_user: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return DB_FAIL;
    }

    sqlite3_finalize(stmt);

    return RST_OK;
}

void DB_User_set_user_data_from_db(sqlite3_stmt *stmt, User &user)
{
    user.set_is_authenticated(false);

    // Helper lambda to get text from column safely
    auto get_col_text = [&](int col) {
        const char* txt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
        return txt ? std::string(txt) : std::string();
    };

    user.set_useraccountid(sqlite3_column_int(stmt, 0));
    user.set_name(get_col_text(1));
    user.set_passwordkey(get_col_text(2));
    user.set_passwordsalt(get_col_text(3));
    user.set_resettoken(get_col_text(4));

    if (sqlite3_column_type(stmt, 5) == SQLITE_NULL) {
        user.set_resetexpiration(0);
    } else {
        user.set_resetexpiration(sqlite3_column_int64(stmt, 5));
    }

    std::string _status = get_col_text(6);
    user.set_status(user.parse_status_to_type(_status));

    if (sqlite3_column_type(stmt, 7) == SQLITE_NULL) {
        user.set_creationdate(0);
    } else {
        user.set_creationdate(sqlite3_column_int64(stmt, 7));
    }

    user.set_locknotes(get_col_text(8));
    user.set_workemail(get_col_text(9));
    user.set_recoveryemail(get_col_text(10));
    user.set_firstname(get_col_text(11));
    user.set_lastname(get_col_text(12));
    user.set_roleid(sqlite3_column_int(stmt, 13));
    user.set_max_space_size_in_kb(sqlite3_column_int(stmt, 14));
}

rst_code_e DB_User::get_user(User &user) const
{
    if (user.get_name().empty())
    {
        logger->debug("User name must not be null when accessing database");
        return DB_BAD_PARAM;
    }

    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT * FROM useraccount WHERE name = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        logger->error("Error preparing get_user: {}", sqlite3_errmsg(db.get()));
        return DB_FAIL;
    }

    sqlite3_bind_text(stmt, 1, user.get_name().c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        DB_User_set_user_data_from_db(stmt, user);
        sqlite3_finalize(stmt);
        return RST_OK;
    } else if (rc == SQLITE_DONE) {
        logger->debug("User name {} is not on database", user.get_name());
        sqlite3_finalize(stmt);
        return USER_NOT_FOUND;
    } else {
        logger->debug("User name {} is not on database", user.get_name());
        sqlite3_finalize(stmt);
        return USER_NOT_FOUND;
    }
}

rst_code_e DB_User::get_user_by_id(unsigned int user_id, User &user) const
{
    if (user_id == 0)
    {
        logger->debug("User ID must not be null when accessing database");
        return DB_BAD_PARAM;
    }

    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT * FROM useraccount WHERE useraccountid = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        logger->error("Error preparing get_user_by_id: {}", sqlite3_errmsg(db.get()));
        return DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        DB_User_set_user_data_from_db(stmt, user);
        sqlite3_finalize(stmt);
        return RST_OK;
    } else if (rc == SQLITE_DONE) {
        logger->debug("User ID {} is not on database", user_id);
        sqlite3_finalize(stmt);
        return USER_NOT_FOUND;
    } else {
        logger->debug("User ID {} is not on database", user_id);
        sqlite3_finalize(stmt);
        return USER_NOT_FOUND;
    }
}
