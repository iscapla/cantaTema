#include "database/db_coverage.hpp"
#include "database/db_connection.hpp"
#include <sqlite3mc_amalgamation.h>
#include <fmt/format.h>

DB_Coverage::DB_Coverage(void) {}
DB_Coverage::~DB_Coverage(void) {}

rst_code_e DB_Coverage::create_coverage_tables() {
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    char *zErrMsg = 0;
    const char *sql = 
        "CREATE TABLE IF NOT EXISTS analysis_executions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "practice_id INTEGER NOT NULL,"
        "analysis_execution_id TEXT NOT NULL UNIQUE,"
        "coverage_percentage REAL,"
        "speed_score REAL,"
        "clarity_score REAL,"
        "pacing_score REAL,"
        "whisper_model TEXT,"
        "llama_model TEXT,"
        "language TEXT,"
        "similarity_threshold REAL,"
        "config_snapshot_json TEXT,"
        "report_json TEXT,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY(practice_id) REFERENCES practices(practice_id) ON DELETE CASCADE);";

    int rc = sqlite3_exec(db.get(), sql, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        logger->error("SQL error creating analysis_executions: {}", zErrMsg);
        sqlite3_free(zErrMsg);
        return rst_code_e::DB_FAIL;
    }
    return rst_code_e::RST_OK;
}

rst_code_e DB_Coverage::save_coverage_analysis_execution(
    int practice_id,
    const std::string& analysis_execution_id,
    double coverage_percentage,
    double speed_score,
    double clarity_score,
    double pacing_score,
    const std::string& whisper_model,
    const std::string& llama_model,
    const std::string& language,
    double similarity_threshold,
    const std::string& config_snapshot_json,
    const std::string& report_json
) {
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = 
        "INSERT INTO analysis_executions ("
        "practice_id, analysis_execution_id, coverage_percentage, speed_score, clarity_score, pacing_score, "
        "whisper_model, llama_model, language, similarity_threshold, config_snapshot_json, report_json) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) != SQLITE_OK) {
        logger->error("Failed to prepare statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, practice_id);
    sqlite3_bind_text(stmt, 2, analysis_execution_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, coverage_percentage);
    sqlite3_bind_double(stmt, 4, speed_score);
    sqlite3_bind_double(stmt, 5, clarity_score);
    sqlite3_bind_double(stmt, 6, pacing_score);
    sqlite3_bind_text(stmt, 7, whisper_model.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, llama_model.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, language.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 10, similarity_threshold);
    sqlite3_bind_text(stmt, 11, config_snapshot_json.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 12, report_json.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        logger->error("Failed to execute save statement: {}", sqlite3_errmsg(db.get()));
        sqlite3_finalize(stmt);
        return rst_code_e::DB_FAIL;
    }

    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_Coverage::get_analysis_executions_for_practice(
    int practice_id,
    std::string& out_executions_list_json
) {
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = 
        "SELECT analysis_execution_id, coverage_percentage, speed_score, clarity_score, pacing_score, "
        "whisper_model, llama_model, language, similarity_threshold, datetime(created_at, 'localtime') "
        "FROM analysis_executions WHERE practice_id = ? ORDER BY created_at DESC;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) != SQLITE_OK) {
        logger->error("Failed to prepare query statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_int(stmt, 1, practice_id);

    std::string json = "[";
    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) {
            json += ",";
        }
        first = false;

        std::string exec_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        double cov = sqlite3_column_double(stmt, 1);
        double speed = sqlite3_column_double(stmt, 2);
        double clarity = sqlite3_column_double(stmt, 3);
        double pacing = sqlite3_column_double(stmt, 4);
        std::string whisper = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        std::string llama = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        std::string lang = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        double thresh = sqlite3_column_double(stmt, 8);
        std::string date_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));

        json += fmt::format(
            "{{"
            "\"analysis_execution_id\":\"{}\","
            "\"coverage_percentage\":{:.2f},"
            "\"speed_score\":{:.2f},"
            "\"clarity_score\":{:.2f},"
            "\"pacing_score\":{:.2f},"
            "\"whisper_model\":\"{}\","
            "\"llama_model\":\"{}\","
            "\"language\":\"{}\","
            "\"similarity_threshold\":{:.2f},"
            "\"created_at\":\"{}\""
            "}}",
            exec_id, cov, speed, clarity, pacing, whisper, llama, lang, thresh, date_str
        );
    }

    json += "]";
    out_executions_list_json = json;

    sqlite3_finalize(stmt);
    return rst_code_e::RST_OK;
}

rst_code_e DB_Coverage::get_analysis_execution_details(
    const std::string& execution_id,
    std::string& out_report_json,
    std::string& out_config_json
) {
    std::shared_ptr<sqlite3> db = DB_Connection::getConn();
    sqlite3_stmt *stmt;
    const char *sql = 
        "SELECT report_json, config_snapshot_json FROM analysis_executions WHERE analysis_execution_id = ?;";

    if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, 0) != SQLITE_OK) {
        logger->error("Failed to prepare details statement: {}", sqlite3_errmsg(db.get()));
        return rst_code_e::DB_FAIL;
    }

    sqlite3_bind_text(stmt, 1, execution_id.c_str(), -1, SQLITE_TRANSIENT);

    rst_code_e ret = rst_code_e::DB_NOT_FOUND;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        out_report_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        out_config_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        ret = rst_code_e::RST_OK;
    }

    sqlite3_finalize(stmt);
    return ret;
}
