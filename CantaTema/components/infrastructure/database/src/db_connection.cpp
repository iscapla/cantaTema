#include <iostream>
#include <cstring>
#include <filesystem>

#include "primitives/utils_logger.hpp"
#include "database/db_connection.hpp"

// Define the static mutex member
std::recursive_mutex DB_Connection::mtx;
DB_Connection* DB_Connection::instance = nullptr;

DB_Connection::DB_Connection() {

    // Create the "data" folder if it doesn't exist
    const std::string data_folder = "data";
    if (!std::filesystem::exists(data_folder)) {
        if (!std::filesystem::create_directory(data_folder)) {
            logger->error("Error creating data directory: {}", data_folder);
            //TODO close app with an error
        }
    }

    // Construct the full path to the database file
    std::filesystem::path db_path = std::filesystem::path(data_folder) / "canta_tema.db";


    // Open the database connection.
    // The filename is hardcoded here, but could be moved to a config file.
    int rc = sqlite3_open(db_path.string().c_str(), &db);

    if (rc) {
        // Log error if connection fails.
        // Note: sqlite3_open usually returns a handle even on error to allow retrieving the error message.
        logger->error("Error opening SQLite3 database: {}", sqlite3_errmsg(db));
        
    }

    // Set the encryption key for the database
    const char *encryption_key = "MySecretPassword";
    rc = sqlite3_key(db, encryption_key, std::strlen(encryption_key));
    if (rc != SQLITE_OK) {
        logger->error("Error setting encryption key: {}", sqlite3_errmsg(db));
        // Handle error, perhaps close the database and exit
        sqlite3_close(db);
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

        // Construct the full path to the database file
        const std::string data_folder = "data";
        std::filesystem::path db_path = std::filesystem::path(data_folder) / "canta_tema.db";

        // Delete the database file
        if (std::filesystem::exists(db_path)) {
            std::filesystem::remove(db_path);
            logger->info("Database file deleted: {}", db_path.string());
        }

    }
}
