#include "database/db_practice_event.hpp"
#include "database/db_connection.hpp"
#include "database/db_subject.hpp"
#include <sqlite3mc_amalgamation.h>

DB_PracticeEvent::DB_PracticeEvent(void) {}

rst_code_e DB_PracticeEvent::practice_event_tables_create(void) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    char *zErrMsg = 0;
    const char *sql = "CREATE TABLE IF NOT EXISTS practice_events ("
                      "practice_event_id INTEGER PRIMARY KEY CHECK(practice_event_id <> 0),"
                      "user_id INTEGER NOT NULL,"
                      "subject_id INTEGER NOT NULL,"
                      "date INTEGER NOT NULL,"
                      "recorded_date INTEGER,"
                      "duration INTEGER,"
                      "filepath TEXT,"
                      "description TEXT,"
                      "analysis_execution_id TEXT,"
                      "status TEXT NOT NULL,"
                      "FOREIGN KEY(user_id) REFERENCES useraccount(useraccountid) ON DELETE CASCADE,"
                      "FOREIGN KEY(subject_id) REFERENCES subjects(subjects_id) ON DELETE CASCADE);";

    int rc = sqlite3_exec(db.get(), sql, 0, 0, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        logger->error("SQL error: {}", zErrMsg);
        sqlite3_free(zErrMsg);
        return rst_code_e::DB_FAIL;
    }

    const char *alter_sql = "ALTER TABLE practice_events ADD COLUMN analysis_execution_id TEXT;";
    sqlite3_exec(db.get(), alter_sql, 0, 0, nullptr);

    return rst_code_e::RST_OK;
}

rst_code_e DB_PracticeEvent::add_new_practice_event(PracticeEvent &event) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO practice_events (user_id, subject_id, date, recorded_date, duration, filepath, description, analysis_execution_id, status) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, event.get_user_id());

    if (event.get_subject_id() != 0)
    {
        sqlite3_bind_int(stmt, 2, event.get_subject_id());
    }
    else
    {
        sqlite3_bind_null(stmt, 2);
    }

    sqlite3_bind_int64(stmt, 3, event.get_date());
    sqlite3_bind_int64(stmt, 4, event.get_recorded_date());
    sqlite3_bind_int(stmt, 5, event.get_duration());
    sqlite3_bind_text(stmt, 6, event.get_filepath().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, event.get_description().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, event.get_analysis_execution_id().c_str(), -1, SQLITE_TRANSIENT);

    std::string status_str = PracticeEvent::get_status_as_string(event.get_status());
    sqlite3_bind_text(stmt, 9, status_str.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        logger->error("Failed to execute statement: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return rst_code_e::DB_FAIL;
    }

    event.set_id(static_cast<unsigned int>(sqlite3_last_insert_rowid(db.get())));
    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_PracticeEvent::update_practice_event(const PracticeEvent &event) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE practice_events SET user_id = ?, subject_id = ?, date = ?, recorded_date = ?, duration = ?, filepath = ?, description = ?, analysis_execution_id = ?, status = ? WHERE practice_event_id = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, event.get_user_id());

    if (event.get_subject_id() != 0)
    {
        sqlite3_bind_int(stmt, 2, event.get_subject_id());
    }
    else
    {
        sqlite3_bind_null(stmt, 2);
    }

    sqlite3_bind_int64(stmt, 3, event.get_date());
    sqlite3_bind_int64(stmt, 4, event.get_recorded_date());
    sqlite3_bind_int(stmt, 5, event.get_duration());
    sqlite3_bind_text(stmt, 6, event.get_filepath().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, event.get_description().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, event.get_analysis_execution_id().c_str(), -1, SQLITE_TRANSIENT);

    std::string status_str = PracticeEvent::get_status_as_string(event.get_status());
    sqlite3_bind_text(stmt, 9, status_str.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_int(stmt, 10, event.get_id());

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        logger->error("Failed to execute statement: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return rst_code_e::DB_FAIL;
    }

    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_PracticeEvent::remove_practice_event(unsigned int id) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM practice_events WHERE practice_event_id = ?;";

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

rst_code_e DB_PracticeEvent::remove_all_practice_events_by_user(unsigned int user_id) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM practice_events WHERE user_id = ?;";

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

rst_code_e DB_PracticeEvent::remove_all_practice_events_by_subject(unsigned int subject_id) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM practice_events WHERE subject_id = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) != SQLITE_OK)
    {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, subject_id);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        logger->error("Failed to execute statement: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return rst_code_e::DB_FAIL;
    }

    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_PracticeEvent::get_practice_event_by_id(unsigned int id, std::shared_ptr<PracticeEvent> &event) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = "SELECT practice_event_id, user_id, subject_id, date, recorded_date, duration, filepath, description, analysis_execution_id, status FROM practice_events WHERE practice_event_id = ?;";
    rst_code_e result = rst_code_e::DB_FAIL;

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, id);

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            unsigned int p_id = sqlite3_column_int(stmt, 0);
            unsigned int user_id = sqlite3_column_int(stmt, 1);
            
            unsigned int subject_id = 0;
            if (sqlite3_column_type(stmt, 2) != SQLITE_NULL)
            {
                subject_id = sqlite3_column_int(stmt, 2);
            }

            unsigned int date = static_cast<unsigned int>(sqlite3_column_int64(stmt, 3));
            unsigned int recorded_date = static_cast<unsigned int>(sqlite3_column_int64(stmt, 4));

            unsigned int duration = 0;
            if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
                duration = sqlite3_column_int(stmt, 5);
            }
            
            std::string filepath = "";
            if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
                filepath = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
            }

            std::string description = "";
            if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
                description = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7));
            }

            std::string analysis_exec_id = "";
            if (sqlite3_column_type(stmt, 8) != SQLITE_NULL) {
                analysis_exec_id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 8));
            }

            std::string status_str = "";
            if (sqlite3_column_type(stmt, 9) != SQLITE_NULL) {
                status_str = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 9));
            }

            event = std::make_shared<PracticeEvent>();
            event->set_id(p_id);
            event->set_user_id(user_id);
            event->set_subject_id(subject_id);
            event->set_status(PracticeEvent::parse_status_from_string(status_str));
            event->set_date(date);
            event->set_recorded_date(recorded_date);
            event->set_duration(duration);
            event->set_filepath(filepath);
            event->set_description(description);
            event->set_analysis_execution_id(analysis_exec_id);
            
            result = rst_code_e::RST_OK;
        }
    }
    sqlite3_finalize(stmt);
    return result;
}

rst_code_e DB_PracticeEvent::get_all_practice_events_by_user(unsigned int user_id, std::vector<std::shared_ptr<PracticeEvent>> &events) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = "SELECT practice_event_id, user_id, subject_id, date, recorded_date, duration, filepath, description, analysis_execution_id, status FROM practice_events WHERE user_id = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, user_id);

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            unsigned int p_id = sqlite3_column_int(stmt, 0);
            unsigned int u_id = sqlite3_column_int(stmt, 1);
            
            unsigned int subject_id = 0;
            if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
                subject_id = sqlite3_column_int(stmt, 2);
            }

            unsigned int date = static_cast<unsigned int>(sqlite3_column_int64(stmt, 3));
            unsigned int recorded_date = static_cast<unsigned int>(sqlite3_column_int64(stmt, 4));

            unsigned int duration = 0;
            if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
                duration = sqlite3_column_int(stmt, 5);
            }

            std::string filepath = "";
            if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
                filepath = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
            }

            std::string description = "";
            if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
                description = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7));
            }

            std::string analysis_exec_id = "";
            if (sqlite3_column_type(stmt, 8) != SQLITE_NULL) {
                analysis_exec_id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 8));
            }

            std::string status_str = "";
            if (sqlite3_column_type(stmt, 9) != SQLITE_NULL) {
                status_str = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 9));
            }

            auto event = std::make_shared<PracticeEvent>();
            event->set_id(p_id);
            event->set_user_id(u_id);
            event->set_subject_id(subject_id);
            event->set_status(PracticeEvent::parse_status_from_string(status_str));
            event->set_date(date);
            event->set_duration(duration);
            event->set_recorded_date(recorded_date);
            event->set_filepath(filepath);
            event->set_description(description);
            event->set_analysis_execution_id(analysis_exec_id);

            events.push_back(event);
        }
    }
    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_PracticeEvent::get_all_practice_events_by_subject(unsigned int subject_id, std::vector<std::shared_ptr<PracticeEvent>> &events) const
{
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = "SELECT practice_event_id, user_id, subject_id, date, recorded_date, duration, filepath, description, analysis_execution_id, status FROM practice_events WHERE subject_id = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, subject_id);

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            unsigned int p_id = sqlite3_column_int(stmt, 0);
            unsigned int u_id = sqlite3_column_int(stmt, 1);
            
            unsigned int subject_id_db = 0;
            if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
                subject_id_db = sqlite3_column_int(stmt, 2);
            }

            unsigned int date = static_cast<unsigned int>(sqlite3_column_int64(stmt, 3));
            unsigned int recorded_date = static_cast<unsigned int>(sqlite3_column_int64(stmt, 4));

            unsigned int duration = 0;
            if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
                duration = sqlite3_column_int(stmt, 5);
            }

            std::string filepath = "";
            if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
                filepath = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
            }

            std::string description = "";
            if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
                description = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7));
            }

            std::string analysis_exec_id = "";
            if (sqlite3_column_type(stmt, 8) != SQLITE_NULL) {
                analysis_exec_id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 8));
            }

            std::string status_str = "";
            if (sqlite3_column_type(stmt, 9) != SQLITE_NULL) {
                status_str = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 9));
            }

            auto event = std::make_shared<PracticeEvent>();
            event->set_id(p_id);
            event->set_user_id(u_id);
            event->set_subject_id(subject_id_db);
            event->set_status(PracticeEvent::parse_status_from_string(status_str));
            event->set_date(date);
            event->set_duration(duration);
            event->set_recorded_date(recorded_date);
            event->set_filepath(filepath);
            event->set_description(description);
            event->set_analysis_execution_id(analysis_exec_id);

            events.push_back(event);
        }
    }
    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}