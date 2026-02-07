#include <iostream>

#include "database/db_connection.hpp"
#include "database/db_main.hpp"
#include "database/db_user.hpp"
#include "database/db_category.hpp"
#include "database/db_subject.hpp"
#include "database/db_user_metrics.hpp"
#include "database/db_practice_event.hpp"


// initializing instancePtr with NULL
DB_Main *DB_Main::instancePtr{nullptr};
bool DB_Main::initialized{false};

DB_Main::DB_Main(void)
{
    if(!DB_Main::initialized){
        DB_User user;
        user.user_tables_create();
        DB_Category category;
        category.category_tables_create();
        DB_Subject subject;
        subject.subject_tables_create();
        DB_UserMetrics user_metrics;
        user_metrics.user_metrics_tables_create();
        DB_PracticeEvent practice_event;
        practice_event.practice_event_tables_create();
        DB_Main::initialized = true;
        logger->info("Database initialized");
    }
}

DB_Main::~DB_Main(void) {}

void DB_Main::purge(void)
{
    DB_Connection::reset_connection();
    DB_Main::initialized = false;
    logger->info("Database purged");

    if (instancePtr) {
        delete instancePtr;
        instancePtr = nullptr;
    }

    // Call DB_Main constructor again to re-initialize the database
    DB_Main::getInstance();
}