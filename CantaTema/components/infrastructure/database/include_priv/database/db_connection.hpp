#pragma once

#include <sqlite3mc_amalgamation.h>
#include <mutex>
#include <memory>

class DB_Connection {
public:
    // Delete copy constructor and assignment operator
    DB_Connection(const DB_Connection&) = delete;
    DB_Connection& operator=(const DB_Connection&) = delete;

    // Unique public method to get the database connection
    static std::shared_ptr<sqlite3> getConn();

    /**
     * @brief Resets the database connection by destroying the current instance.
     */
    static void reset_connection();

private:
    DB_Connection();
    ~DB_Connection();

    // The SQLite database connection handle
    sqlite3* db = nullptr;
    // Mutex to ensure thread safety
    static std::recursive_mutex mtx;
    // Static instance pointer to allow manual destruction
    static DB_Connection* instance;

};
