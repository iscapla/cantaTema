#include <fstream>
#include <filesystem>
#include <ctime>
#include <vector>
#include <memory>

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

    // 4. Create study categories
    category_add(out, "Derecho Constitucional");
    category_add(out, "Derecho Penal");
    category_add(out, "Derecho Civil");

    // 5. Create study tags
    tag_add(out, "Importante");
    tag_add(out, "Repasar");
    tag_add(out, "Examen");
    tag_add(out, "Dificil");

    // Resolve example reference documents
    std::string file_path_1 = "example_data/subject_es_1.pdf";
    if (std::filesystem::exists(file_path_1))
    {
        file_path_1 = std::filesystem::absolute(file_path_1).string();
    }
    std::string file_path_2 = "example_data/Don_Quijote_de_la_Mancha.pdf";
    if (std::filesystem::exists(file_path_2))
    {
        file_path_2 = std::filesystem::absolute(file_path_2).string();
    }
    else
    {
        file_path_2 = file_path_1;
    }

    // Resolve example audio recording
    std::string audio_file_path = "example_data/subject_es_1_p_1.opus";
    if (std::filesystem::exists(audio_file_path))
    {
        audio_file_path = std::filesystem::absolute(audio_file_path).string();
    }

    // 6. Create subjects with categories and reference files
    subject_add_from_path(out, 1, "Tema 1: La Corona", file_path_1);
    subject_add_from_path(out, 2, "Tema 2: Teoria del Delito", file_path_1);
    subject_add_from_path(out, 3, "Tema 3: Obligaciones y Contratos", file_path_2);
    subject_add_from_path(out, 0, "Tema 4: Literatura Clasica", file_path_2);

    // 7. Retrieve created subjects and tags to establish associations and practice events
    std::vector<std::shared_ptr<Subject>> subjects;
    std::vector<std::shared_ptr<Tag>> tags;
    if (op->subject_get_by_user(subjects) == RST_OK && op->tag_get_by_user(tags) == RST_OK)
    {
        // Assign tags to subjects
        if (subjects.size() >= 4 && tags.size() >= 4)
        {
            // Subject 1 -> Importante (tag 1), Examen (tag 3)
            subject_add_tag(out, subjects[0]->get_id(), tags[0]->get_id());
            subject_add_tag(out, subjects[0]->get_id(), tags[2]->get_id());

            // Subject 2 -> Repasar (tag 2), Dificil (tag 4)
            subject_add_tag(out, subjects[1]->get_id(), tags[1]->get_id());
            subject_add_tag(out, subjects[1]->get_id(), tags[3]->get_id());

            // Subject 3 -> Importante (tag 1), Repasar (tag 2)
            subject_add_tag(out, subjects[2]->get_id(), tags[0]->get_id());
            subject_add_tag(out, subjects[2]->get_id(), tags[1]->get_id());

            // Subject 4 -> Dificil (tag 4)
            subject_add_tag(out, subjects[3]->get_id(), tags[3]->get_id());
        }

        // Add practice events
        int idx = 0;
        for (const auto &sub : subjects)
        {
            if (idx == 0) {
                practice_event_add_planned(out, sub->get_id(), "2025-02-08", "Repaso inicial de conceptos");
                
                PracticeEvent practice;
                practice.set_subject_id(sub->get_id());
                practice.set_duration(60);
                practice.set_description("Grabacion de cante completo tema 1");
                practice.set_date(std::time(nullptr));
                op->practice_event_add_recorded(audio_file_path, practice);

            } else if (idx == 1) {
                practice_event_add_planned(out, sub->get_id(), "2025-02-09", "Simulacro de examen");
                practice_event_add_planned(out, sub->get_id(), "2025-02-10", "Repaso articulado");
            } else if (idx == 2) {
                PracticeEvent practice;
                practice.set_subject_id(sub->get_id());
                practice.set_duration(120);
                practice.set_description("Practica grabada tema 3");
                practice.set_date(std::time(nullptr));
                op->practice_event_add_recorded(audio_file_path, practice);
            }
            idx++;
        }
    }

    // 8. Print all system information
    out << std::endl << "=== USER INFO ===" << std::endl;
    user_get(out);
    out << std::endl << "=== USER METRICS ===" << std::endl;
    user_metrics_get(out);
    out << std::endl << "=== CATEGORIES ===" << std::endl;
    category_get_by_user(out);
    out << std::endl << "=== TAGS ===" << std::endl;
    tag_get_by_user(out);
    out << std::endl << "=== SUBJECTS (TEMAS) ===" << std::endl;
    subject_get_by_user(out);
    out << std::endl << "=== PRACTICE EVENTS ===" << std::endl;
    practice_event_get_by_user(out);
    out << std::endl;
}