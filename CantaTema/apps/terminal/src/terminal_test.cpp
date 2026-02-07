#include <fstream>
#include <filesystem>

#include "terminal/terminal_cli.hpp"
#include "terminal/terminal_session.hpp"
#include "database/db_main.hpp"

#include <thread>
#include <chrono>
#include <iomanip>

void TerminalSession::db_purge(std::ostream &out)
{
    DB_Main *db_main = DB_Main::getInstance();
    db_main->purge();
}

void TerminalSession::test_start(std::ostream &out)
{
    // 1. Purge the database
    db_purge(out);

    // 2. Create a user with a password
    std::string username = "test_user";
    std::string password = "password123";
    user_add(out, username, password);

    // 3. Identify the user with the given credentials
    user_identify(out, username, password);

    // 4. Create a category
    category_add(out, "Test Category");
    unsigned int category_id = 1; // Assuming ID 1 after purge

    // Use example data file
    std::string file_path = "example_data/subject_es_1.pdf";
    file_path = std::filesystem::absolute(file_path).string();

    // 5. Create 2 subjects with a file. Set the category to match the previosly created
    subject_add(out, "Subject 1", category_id, file_path);
    subject_add(out, "Subject 2", category_id, file_path);

    // 6. Create 2 subjects without category
    subject_add(out, "Subject 3", 0, file_path);
    subject_add(out, "Subject 4", 0, file_path);

    // 7. Add practice events
    std::vector<std::shared_ptr<const Subject>> subjects;
    if (op->subject_get_by_user(subjects) == RST_OK)
    {
        int idx = 0;
        for (const auto &sub : subjects)
        {
            if (idx == 0) {
                practice_event_add_planned(out, sub->get_id(), 30, "Scales");
                practice_event_add_recorded(out, sub->get_id(), file_path, "Improvisation");
            } else if (idx == 1) {
                practice_event_add_planned(out, sub->get_id(), 45, "Repertoire A");
                practice_event_add_planned(out, sub->get_id(), 60, "Repertoire B");
            } else if (idx == 2) {
                practice_event_add_recorded(out, sub->get_id(), file_path, "Session Recording");
            }
            idx++;
        }
    }

    // for (int seconds_elapsed = 0; seconds_elapsed < 10; ++seconds_elapsed) {
    //     int mm = seconds_elapsed / 60;
    //     int ss = seconds_elapsed % 60;

    //     // \r moves cursor to start of line
    //     // Save previous fill char to avoid affecting other outputs (like tables)
    //     char old_fill = out.fill('0');
    //     out << "\rTimer: " 
    //         << std::setw(2) << mm << ":" 
    //         << std::setw(2) << ss 
    //         << std::flush;
    //     out.fill(old_fill);

    //     std::this_thread::sleep_for(std::chrono::seconds(1));
    // }
    // out << std::endl;

    // 8. Print all the information
    user_get(out);
    category_get_by_user(out);
    subject_get_by_user(out);
    practice_event_get_by_user(out);
}