

#include "terminal/terminal_cli.hpp"

#include <vector>
#include <algorithm> // std::copy
#include <complex>

#include <cli/standaloneasioscheduler.h>
// #include <cli/standaloneasioremotecli.h>
#include <cli/clifilesession.h>
#include <cli/cli.h>
#include <cli/clilocalsession.h>
#include <cli/filehistorystorage.h>

#include "primitives/utils_logger.hpp"
#include "terminal/terminal_session.hpp"

using MainScheduler = cli::StandaloneAsioScheduler;
// using CliTelnetServer = cli::StandaloneAsioCliTelnetServer;

static std::unique_ptr<TerminalSession> terminal_session;

rst_code_e terminal_init_user(std::unique_ptr<cli::Menu> &rootMenu);
rst_code_e terminal_init_category(std::unique_ptr<cli::Menu> &rootMenu);
rst_code_e terminal_init_subject(std::unique_ptr<cli::Menu> &rootMenu);
rst_code_e terminal_init_practice(std::unique_ptr<cli::Menu> &rootMenu);
rst_code_e terminal_init_models(std::unique_ptr<cli::Menu> &rootMenu);
rst_code_e terminal_init_coverage(std::unique_ptr<cli::Menu> &rootMenu);
rst_code_e terminal_init_hardware(std::unique_ptr<cli::Menu> &rootMenu);


rst_code_e terminal_cli_start(void)
{
    try
    {

        // setup cli
        cli::SetColor();

        terminal_session = std::make_unique<TerminalSession>();

        auto rootMenu = std::make_unique<cli::Menu>("cli");

        rootMenu->Insert(
            "db_purge", {},
            [](std::ostream &out)
            {
                terminal_session->db_purge(out);
            },
            "Purge database");

        rootMenu->Insert(
            "test_start", {},
            [](std::ostream &out)
            {
                terminal_session->test_start(out);
            },
            "Purge and populate database with test data");

        // rootMenu->Insert(
        //     "test_start",
        //     [](std::ostream &out)
        //     {
        //         terminal_session->test_start(out);
        //     },
        //     "Execute default test commands to start with the real tests");

        // rootMenu->Insert(
        //     "test_populate",
        //     [](std::ostream &out)
        //     {
        //         terminal_session->test_populate(out);
        //     },
        //     "Populate database with example data");


        terminal_init_user(rootMenu);
        terminal_init_category(rootMenu);
        terminal_init_subject(rootMenu);
        terminal_init_practice(rootMenu);
        terminal_init_models(rootMenu);
        terminal_init_coverage(rootMenu);
        terminal_init_hardware(rootMenu);

        rootMenu->Insert(
            "user_identify", {"name", "password"},
            [](std::ostream &out, const std::string &name, const std::string &password)
            {
                terminal_session->user_identify(out, name, password);
            },
            "Identify a user");
        
        rootMenu->Insert(
            "metrics", {},
            [](std::ostream &out)
            {
                terminal_session->user_metrics_get(out);
            },
            "User metrics");

        rootMenu->Insert(
            "hardware", {},
            [](std::ostream &out)
            {
                terminal_session->hardware_info(out);
            },
            "Display detected CPU and GPU hardware information");

        rootMenu->Insert(
            "hardware_info", {},
            [](std::ostream &out)
            {
                terminal_session->hardware_info(out);
            },
            "Display detected CPU and GPU hardware information");

        // create a cli with the given root menu and a persistent storage
        // you must pass to FileHistoryStorage the path of the history file
        // if you don't pass the second argument, the cli will use a VolatileHistoryStorage object that keeps in memory
        // the history of all the sessions, until the cli is shut down.
        cli::Cli cli(std::move(rootMenu), std::make_unique<cli::FileHistoryStorage>(".cli"));

        // std exception custom handler
        cli.StdExceptionHandler(
            [](std::ostream &out, const std::string &cmd, const std::exception &e)
            {
                out << "Exception caught in cli handler: "
                    << e.what()
                    << " handling command: "
                    << cmd
                    << ".\n";
            });

        MainScheduler scheduler;
        cli::CliLocalTerminalSession localSession(cli, scheduler, std::cout, 200);
        localSession.ExitAction(
            [&scheduler](auto &out) // session exit action
            {
                scheduler.Stop();
            });

        // setup server to be able to open remote terminals
        // CliTelnetServer server(cli, scheduler, 5000);
        // // exit action for all the connections
        // server.ExitAction([](auto &out)
        //                   { out << "Terminating this session...\n"; });

        scheduler.Run();

        return RST_OK;
    }
    catch (const std::exception &e)
    {
        logger->error("Exception caugth in main: {}", e.what());
    }
    catch (...)
    {
        logger->critical("Unknown exception caugth.");
    }

    return CONSOLE_EXP;
}

rst_code_e terminal_init_practice(std::unique_ptr<cli::Menu> &rootMenu)
{
    try
    {
        auto practiceMenu = std::make_unique<cli::Menu>("practice", "Practice event commands (MENU)");

        practiceMenu->Insert(
            "add_planned", {"subject_id", "date", "description"},
            [](std::ostream &out, unsigned int subject_id, const std::string &date, const std::string &description)
            {
                terminal_session->practice_event_add_planned(out, subject_id, date, description);
            },
            "Add a planned practice event");

        practiceMenu->Insert(
            "add_recorded", {"subject_id", "name"},
            [](std::ostream &out, unsigned int subject_id, const std::string name)
            {
                terminal_session->practice_event_add_recorded(out, subject_id, name);
            },
            "Add a recorded practice event (user recording action)");
        
        practiceMenu->Insert(
            "add_recorded_file", {"subject_id", "date"},
            [](std::ostream &out, unsigned int subject_id, const std::string &date)
            {
                terminal_session->practice_event_add_recorded_from_file(out, subject_id, date);
            },
            "Add a recorded practice event (select file)");
        
        practiceMenu->Insert(
            "play", {"practice_id"},
            [](std::ostream &out, unsigned int practice_id)
            {
                terminal_session->practice_event_play(out, practice_id);
            },
            "Play a practice event");

        practiceMenu->Insert(
            "update", {"id", "status", "description"},
            [](std::ostream &out, unsigned int id, const std::string &status, const std::string &description)
            {
                terminal_session->practice_event_update(out, id, status, description);
            },
            "Update a practice event");

        practiceMenu->Insert(
            "remove", {"id"},
            [](std::ostream &out, unsigned int id)
            {
                terminal_session->practice_event_remove(out, id);
            },
            "Remove a practice event");

        practiceMenu->Insert(
            "get_by_id", {"id"},
            [](std::ostream &out, unsigned int id)
            {
                terminal_session->practice_event_get_by_id(out, id);
            },
            "Get practice event by ID");

        practiceMenu->Insert(
            "get_by_subject", {"subject_id"},
            [](std::ostream &out, unsigned int subject_id)
            {
                terminal_session->practice_event_get_by_subject(out, subject_id);
            },
            "Get practice events by subject ID");

        practiceMenu->Insert(
            "get_by_user", {},
            [](std::ostream &out)
            {
                terminal_session->practice_event_get_by_user(out);
            },
            "Get all practice events for current user");

        rootMenu->Insert(std::move(practiceMenu));
        return RST_OK;
    }
    catch (const std::exception &e)
    {
        logger->error("Exception caught in practice menu: {}", e.what());
    }
    catch (...)
    {
        logger->critical("Unknown exception caught in practice menu.");
    }

    return CONSOLE_EXP;
}

rst_code_e terminal_init_subject(std::unique_ptr<cli::Menu> &rootMenu)
{
    try
    {
        auto subjectMenu = std::make_unique<cli::Menu>("subject", "Subject commands (MENU)");

        subjectMenu->Insert(
            "subject_add", {"category_id"},
            [](std::ostream &out, unsigned int category_id, const std::string &name)
            {
                terminal_session->subject_add(out, category_id, name);
            },
            "Add new subject");

        subjectMenu->Insert(
            "subject_update", {"id", "new_category_id", "new_name"},
            [](std::ostream &out, unsigned int id, const unsigned int new_category_id, const std::string &new_name)
            {
                terminal_session->subject_update(out, id, new_category_id, new_name);
            },
            "Update subject");

        subjectMenu->Insert(
            "subject_remove", {"id"},
            [](std::ostream &out, unsigned int id)
            {
                terminal_session->subject_remove(out, id);
            },
            "Remove subject");

        subjectMenu->Insert(
            "subject_get_by_category", {"category_id"},
            [](std::ostream &out, unsigned int category_id)
            {
                terminal_session->subject_get_by_category(out, category_id);
            },
            "Get subjects by category");

        subjectMenu->Insert(
            "subject_get_by_user", {},
            [](std::ostream &out)
            {
                terminal_session->subject_get_by_user(out);
            },
            "Get all subjects for current user");

        subjectMenu->Insert(
            "set_language", {"subject_id", "language"},
            [](std::ostream &out, unsigned int subject_id, const std::string &language)
            {
                terminal_session->subject_set_language(out, subject_id, language);
            },
            "Set language for subject (e.g. es, en)");

        rootMenu->Insert(std::move(subjectMenu));
        return RST_OK;
    }
    catch (const std::exception &e)
    {
        logger->error("Exception caught in subject menu: {}", e.what());
    }
    catch (...)
    {
        logger->critical("Unknown exception caught in subject menu.");
    }

    return CONSOLE_EXP;
}

rst_code_e terminal_init_user(std::unique_ptr<cli::Menu> &rootMenu)
{
    try
    {

        auto userMenu = std::make_unique<cli::Menu>("user", "User commands (MENU)");

                userMenu->Insert(
            "user_add", {"name", "password"},
            [](std::ostream &out, const std::string &name, const std::string &password)
            {
                terminal_session->user_add(out, name, password);
            },
            "Add new user to the system");

        userMenu->Insert(
            "user_get", {},
            [](std::ostream &out)
            {
                terminal_session->user_get(out);
            },
            "Prints user information if registered");

        userMenu->Insert(
            "user_remove", {},
            [](std::ostream &out)
            {
                terminal_session->user_remove(out);
            },
            "Remove user if registered");

        rootMenu->Insert(std::move(userMenu));
    }
    catch (const std::exception &e)
    {
        logger->error("Exception caugth in user menu: {}", e.what());
    }
    catch (...)
    {
        logger->critical("Unknown exception caugth in user menu.");
    }

    return CONSOLE_EXP;
}

rst_code_e terminal_init_category(std::unique_ptr<cli::Menu> &rootMenu)
{
    try
    {
        auto categoryMenu = std::make_unique<cli::Menu>("category", "Category commands (MENU)");

        categoryMenu->Insert(
            "category_add", {"name"},
            [](std::ostream &out, const std::string &name)
            {
                terminal_session->category_add(out, name);
            },
            "Add new category");

        categoryMenu->Insert(
            "category_update", {"id", "new_name"},
            [](std::ostream &out, const unsigned int id, const std::string &new_name)
            {
                terminal_session->category_update(out, id, new_name);
            },
            "Update category name");

        categoryMenu->Insert(
            "category_remove", {"id"},
            [](std::ostream &out, const unsigned int id)
            {
                terminal_session->category_remove(out, id);
            },
            "Remove category");

        categoryMenu->Insert(
            "category_get", {},
            [](std::ostream &out)
            {
                terminal_session->category_get_by_user(out);
            },
            "Get all categories for current user");

        rootMenu->Insert(std::move(categoryMenu));
        return RST_OK;
    }
    catch (const std::exception &e)
    {
        logger->error("Exception caught in category menu: {}", e.what());
    }
    catch (...)
    {
        logger->critical("Unknown exception caught in category menu.");
    }

    return CONSOLE_EXP;
}

rst_code_e terminal_init_models(std::unique_ptr<cli::Menu> &rootMenu)
{
    try
    {
        auto modelsMenu = std::make_unique<cli::Menu>("models", "Model management commands (MENU)");

        modelsMenu->Insert(
            "list", {},
            [](std::ostream &out)
            {
                terminal_session->models_list(out);
            },
            "List all available Whisper and Llama models");

        modelsMenu->Insert(
            "download_whisper", {"model_name"},
            [](std::ostream &out, const std::string &model_name)
            {
                terminal_session->models_download_whisper(out, model_name);
            },
            "Download a Whisper speech recognition model");

        modelsMenu->Insert(
            "download_llama", {"model_name"},
            [](std::ostream &out, const std::string &model_name)
            {
                terminal_session->models_download_llama(out, model_name);
            },
            "Download a Llama embedding model");

        modelsMenu->Insert(
            "remove_whisper", {"model_name"},
            [](std::ostream &out, const std::string &model_name)
            {
                terminal_session->models_remove_whisper(out, model_name);
            },
            "Remove a local Whisper model file from disk");

        modelsMenu->Insert(
            "remove_llama", {"model_name"},
            [](std::ostream &out, const std::string &model_name)
            {
                terminal_session->models_remove_llama(out, model_name);
            },
            "Remove a local Llama model file from disk");

        rootMenu->Insert(std::move(modelsMenu));
        return RST_OK;
    }
    catch (const std::exception &e)
    {
        logger->error("Exception caught in models menu: {}", e.what());
    }
    catch (...)
    {
        logger->critical("Unknown exception caught in models menu.");
    }

    return CONSOLE_EXP;
}

rst_code_e terminal_init_coverage(std::unique_ptr<cli::Menu> &rootMenu)
{
    try
    {
        auto coverageMenu = std::make_unique<cli::Menu>("coverage", "Coverage analysis commands (MENU)");

        coverageMenu->Insert(
            "analyze", {"practice_id"},
            [](std::ostream &out, unsigned int practice_id)
            {
                terminal_session->coverage_analyze(out, practice_id);
            },
            "Analyze coverage for practice session using default configuration");

        coverageMenu->Insert(
            "analyze_with_models", {"practice_id", "whisper_model", "llama_model"},
            [](std::ostream &out, unsigned int practice_id, const std::string &whisper_model, const std::string &llama_model)
            {
                terminal_session->coverage_analyze(out, practice_id, whisper_model, llama_model);
            },
            "Analyze coverage for practice session using specified Whisper and Llama models");

        coverageMenu->Insert(
            "report", {"execution_id"},
            [](std::ostream &out, const std::string &execution_id)
            {
                terminal_session->coverage_report(out, execution_id);
            },
            "View detailed coverage analysis report by execution ID");

        coverageMenu->Insert(
            "task_submit", {"practice_id"},
            [](std::ostream &out, unsigned int practice_id)
            {
                terminal_session->coverage_task_submit(out, practice_id);
            },
            "Submit asynchronous analysis task to background scheduler");

        coverageMenu->Insert(
            "status", {"task_id"},
            [](std::ostream &out, const std::string &task_id)
            {
                terminal_session->coverage_task_status(out, task_id);
            },
            "Check status and progress percentage of an analysis task");

        coverageMenu->Insert(
            "cancel", {"task_id"},
            [](std::ostream &out, const std::string &task_id)
            {
                terminal_session->coverage_task_cancel(out, task_id);
            },
            "Cancel a queued or currently executing analysis task");

        coverageMenu->Insert(
            "list", {},
            [](std::ostream &out)
            {
                terminal_session->coverage_task_list(out);
            },
            "List all analysis tasks submitted by current user");

        coverageMenu->Insert(
            "task_status", {"task_id"},
            [](std::ostream &out, const std::string &task_id)
            {
                terminal_session->coverage_task_status(out, task_id);
            },
            "Check status and progress percentage of an analysis task");

        coverageMenu->Insert(
            "task_cancel", {"task_id"},
            [](std::ostream &out, const std::string &task_id)
            {
                terminal_session->coverage_task_cancel(out, task_id);
            },
            "Cancel a queued or currently executing analysis task");

        coverageMenu->Insert(
            "task_list", {},
            [](std::ostream &out)
            {
                terminal_session->coverage_task_list(out);
            },
            "List all analysis tasks submitted by current user");

        coverageMenu->Insert(
            "report_practice", {"practice_id"},
            [](std::ostream &out, unsigned int practice_id)
            {
                terminal_session->coverage_report_by_practice(out, practice_id);
            },
            "View coverage analysis report for a practice event");

        coverageMenu->Insert(
            "admin_tasks", {},
            [](std::ostream &out)
            {
                terminal_session->coverage_admin_tasks(out);
            },
            "Admin: list all analysis tasks across all users in system");

        rootMenu->Insert(std::move(coverageMenu));
        return RST_OK;
    }
    catch (const std::exception &e)
    {
        logger->error("Exception caught in coverage menu: {}", e.what());
    }
    catch (...)
    {
        logger->critical("Unknown exception caught in coverage menu.");
    }

    return CONSOLE_EXP;
}

rst_code_e terminal_init_hardware(std::unique_ptr<cli::Menu> &rootMenu)
{
    try
    {
        auto hardwareMenu = std::make_unique<cli::Menu>("hardware", "Hardware detection commands (MENU)");

        hardwareMenu->Insert(
            "info", {},
            [](std::ostream &out)
            {
                terminal_session->hardware_info(out);
            },
            "Display complete host CPU and GPU hardware information");

        hardwareMenu->Insert(
            "cpu", {},
            [](std::ostream &out)
            {
                terminal_session->hardware_cpu(out);
            },
            "Display host CPU specifications and cores");

        hardwareMenu->Insert(
            "gpu", {},
            [](std::ostream &out)
            {
                terminal_session->hardware_gpu(out);
            },
            "Display detected GPU devices and acceleration status");

        rootMenu->Insert(std::move(hardwareMenu));
        return RST_OK;
    }
    catch (const std::exception &e)
    {
        logger->error("Exception caught in hardware menu: {}", e.what());
    }
    catch (...)
    {
        logger->critical("Unknown exception caught in hardware menu.");
    }

    return CONSOLE_EXP;
}

