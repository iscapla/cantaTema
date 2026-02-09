#include <fstream>
#include <filesystem>
#include <ctime>

#include "terminal/terminal_cli.hpp"
#include "terminal/terminal_session.hpp"
#include "database/db_main.hpp"

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
    std::string username = "user";
    std::string password = "pass";
    user_add(out, username, password);

    // 3. Identify the user with the given credentials
    user_identify(out, username, password);

    // 4. Create a category
    category_add(out, "Test Category");
    unsigned int category_id = 1; // Assuming ID 1 after purge

    // Use example data file
    std::string file_path = "example_data/subject_es_1.pdf";
    file_path = std::filesystem::absolute(file_path).string();
    std::filesystem::path p(file_path);
    std::string file_name = p.stem().string();

    // Use example audio file
    std::string audio_file_path = "example_data/subject_es_1_p_1.opus";
    audio_file_path = std::filesystem::absolute(audio_file_path).string();

    // 5. Create 2 subjects with a file. Set the category to match the previosly created
    
    subject_add_from_path(out, category_id, file_name + "_1", file_path);
    subject_add_from_path(out, category_id, file_name + "_2", file_path);

    // 6. Create 2 subjects without category
    subject_add_from_path(out, 0, file_name + "_3", file_path);
    subject_add_from_path(out, 0, file_name + "_4", file_path);

    // 7. Add practice events
    std::vector<std::shared_ptr<Subject>> subjects;
    if (op->subject_get_by_user(subjects) == RST_OK)
    {
        int idx = 0;
        for (const auto &sub : subjects)
        {
            if (idx == 0) {
                practice_event_add_planned(out, sub->get_id(), "2025-02-08", "Scales");
                
                PracticeEvent practice;
                practice.set_subject_id(sub->get_id());
                practice.set_duration(60);
                practice.set_description("Recorded Scales");
                practice.set_date(std::time(nullptr));
                op->practice_event_add_recorded(audio_file_path, practice);

            } else if (idx == 1) {
                practice_event_add_planned(out, sub->get_id(), "2025-02-09", "Repertoire A");
                practice_event_add_planned(out, sub->get_id(), "2025-02-10", "Repertoire B");
            } else if (idx == 2) {
                PracticeEvent practice;
                practice.set_subject_id(sub->get_id());
                practice.set_duration(120);
                practice.set_description("Recorded Improvisation");
                practice.set_date(std::time(nullptr));
                op->practice_event_add_recorded(audio_file_path, practice);
            }
            idx++;
        }
    }

    // 8. Print all the information
    out << std::endl << std::endl;
    user_get(out);
    out << std::endl << std::endl;
    user_metrics_get(out);
    out << std::endl << std::endl;
    category_get_by_user(out);
    out << std::endl << std::endl;
    subject_get_by_user(out);
    out << std::endl << std::endl;
    practice_event_get_by_user(out);
    out << std::endl << std::endl;
}