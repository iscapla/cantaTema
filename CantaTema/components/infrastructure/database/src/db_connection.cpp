#include <iostream>
#include <cstring>
#include <filesystem>
#include <thread>
#include <chrono>

#include "primitives/tool_paths.hpp"
#include "primitives/utils_logger.hpp"
#include "database/db_connection.hpp"

// Define the static mutex member
std::recursive_mutex DB_Connection::mtx;
DB_Connection* DB_Connection::instance = nullptr;

DB_Connection::DB_Connection() {

    // Construct the full path to the database file
    std::filesystem::path db_path = ToolPath::get_path_for_database() / "data.db";
    logger->debug("Database path: {}", db_path.string());

    // Open the database connection.
    // The filename is hardcoded here, but could be moved to a config file.
    int rc = sqlite3_open(db_path.string().c_str(), &db);

    if (rc) {
        // Log error if connection fails.
        // Note: sqlite3_open usually returns a handle even on error to allow retrieving the error message.
        logger->error("Error opening SQLite3 database: {}", sqlite3_errmsg(db));
        
    }

    // Set the encryption key for the database
    const char *encryption_key = "MySecretPassword"; //TODO: Is there a way to hide this secret?
    rc = sqlite3_key(db, encryption_key, std::strlen(encryption_key));
    if (rc != SQLITE_OK) {
        logger->error("Error setting encryption key: {}", sqlite3_errmsg(db));
        // Handle error, perhaps close the database and exit
        sqlite3_close_v2(db);
        db = nullptr;
        //TODO close app with an error
    }

}

DB_Connection::~DB_Connection() {
    if (db) {
        logger->info("Closing database connection");
        sqlite3_close(db);
        db = nullptr;
    }else{
        logger->info("No database connection to close");
    }
}

std::shared_ptr<sqlite3> DB_Connection::getConn() {
    // Lock the mutex to ensure exclusive access to the connection
    mtx.lock();

    if (instance == nullptr) {
        try {
            instance = new DB_Connection();
        } catch (...) {
            mtx.unlock();
            throw;
        }
    }

    // Return a shared_ptr with a custom deleter.
    // The deleter simply unlocks the mutex when the shared_ptr goes out of scope.
    return std::shared_ptr<sqlite3>(instance->db, [](sqlite3*) {
        mtx.unlock();
    });
}

void DB_Connection::reset_connection() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;

        std::filesystem::path db_dir = ToolPath::get_path_for_database();
        std::filesystem::path db_file = db_dir / "data.db";
        std::vector<std::filesystem::path> files_to_remove = {
            db_file,
            db_dir / "data.db-journal",
            db_dir / "data.db-wal",
            db_dir / "data.db-shm"
        };

        for (const auto& file : files_to_remove) {
            if (std::filesystem::exists(file)) {
                std::error_code ec;
                for (int attempt = 0; attempt < 25; ++attempt) {
                    ec.clear();
                    std::filesystem::remove(file, ec);
                    if (!ec || !std::filesystem::exists(file)) {
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(25));
                }
                if (ec && std::filesystem::exists(file)) {
                    logger->error("Failed to delete database file {}: {}", file.string(), ec.message());
                    throw std::filesystem::filesystem_error("cannot remove database file", file, ec);
                }
            }
        }
        logger->info("Database reset completed for: {}", db_dir.string());
    }
}
