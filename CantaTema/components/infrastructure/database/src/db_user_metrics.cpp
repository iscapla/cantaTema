#include "database/db_user_metrics.hpp"
#include "database/db_connection.hpp"
#include <sqlite3mc_amalgamation.h>

DB_UserMetrics::DB_UserMetrics(void)
{
}

rst_code_e DB_UserMetrics::user_metrics_tables_create(void) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    char *zErrMsg = nullptr;
    
    // Table linked to useraccount via foreign key.
    // ON DELETE CASCADE ensures metrics are removed if the user is removed.
    const char *sql_table =
        "CREATE TABLE IF NOT EXISTS user_metrics ("
        "useraccountid INTEGER PRIMARY KEY,"
        "space_used_kb INTEGER DEFAULT 0,"
        "FOREIGN KEY(useraccountid) REFERENCES useraccount(useraccountid) ON DELETE CASCADE"
        ");";

    int rc = sqlite3_exec(db.get(), sql_table, nullptr, nullptr, &zErrMsg);

    if (rc != SQLITE_OK) {
        logger->error("Error creating user_metrics table: {}", zErrMsg);
        sqlite3_free(zErrMsg);
        return DB_FAIL;
    }

    return RST_OK;
}

rst_code_e DB_UserMetrics::update_user_metrics(const UserMetrics &metrics) const
{
    if (metrics.get_useraccountid() == 0)
    {
        logger->debug("User ID must not be 0 when accessing database metrics");
        return DB_BAD_PARAM;
    }

    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt = nullptr;

    // Use SQLite UPSERT syntax
    const char *sql = "INSERT INTO user_metrics (useraccountid, space_used_kb) "
                      "VALUES (?, ?) "
                      "ON CONFLICT (useraccountid) DO UPDATE SET "
                      "space_used_kb = excluded.space_used_kb;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        logger->error("Error preparing update_user_metrics: {}", sqlite3_errmsg(db.get()));
        return DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, metrics.get_useraccountid());
    sqlite3_bind_int(stmt, 2, metrics.get_space_used_kb());

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        logger->error("Error executing update_user_metrics: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return DB_FAIL;
    }

    sqlite3_finalize(stmt);
    return RST_OK;
}

rst_code_e DB_UserMetrics::remove_user_metrics(unsigned int user_id) const
{
    if (user_id == 0)
    {
        logger->debug("User ID must not be 0 when removing metrics");
        return DB_BAD_PARAM;
    }

    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "DELETE FROM user_metrics WHERE useraccountid = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        logger->error("Error preparing remove_user_metrics: {}", sqlite3_errmsg(db.get()));
        return DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        logger->error("Error executing remove_user_metrics: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return DB_FAIL;
    }

    sqlite3_finalize(stmt);
    return RST_OK;
}

rst_code_e DB_UserMetrics::get_user_metrics(std::shared_ptr<UserMetrics> metrics) const
{
    if (metrics == nullptr || metrics->get_useraccountid() == 0)
    {
        logger->debug("User ID must not be 0 or null when getting metrics");
        return DB_BAD_PARAM;
    }

    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT space_used_kb FROM user_metrics WHERE useraccountid = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        logger->error("Error preparing get_user_metrics: {}", sqlite3_errmsg(db.get()));
        return DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, metrics->get_useraccountid());

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        metrics->set_space_used_kb(sqlite3_column_int(stmt, 0));
        sqlite3_finalize(stmt);
        return RST_OK;
    } else if (rc == SQLITE_DONE) {
        logger->debug("Metrics for User ID {} not found", metrics->get_useraccountid());
        sqlite3_finalize(stmt);
        return DB_NOT_FOUND;
    } else {
        logger->error("Error executing get_user_metrics: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return DB_FAIL;
    }
}