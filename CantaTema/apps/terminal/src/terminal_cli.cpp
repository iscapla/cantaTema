

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
#include "database/db_main.hpp"
#include "file_handler/file_handler.hpp"

using MainScheduler = cli::StandaloneAsioScheduler;
// using CliTelnetServer = cli::StandaloneAsioCliTelnetServer;

static std::unique_ptr<TerminalSession> terminal_session;

rst_code_e terminal_init_user(std::unique_ptr<cli::Menu> &rootMenu);
rst_code_e terminal_init_category(std::unique_ptr<cli::Menu> &rootMenu);
rst_code_e terminal_init_subject(std::unique_ptr<cli::Menu> &rootMenu);



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
                DB_Main *db_main = DB_Main::getInstance();
                db_main->purge();
            },
            "Purge database");

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


        rootMenu->Insert(
            "user_identify", {"name", "password"},
            [](std::ostream &out, const std::string &name, const std::string &password)
            {
                terminal_session->user_identify(out, name, password);
            },
            "Identify a user");

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

rst_code_e terminal_init_subject(std::unique_ptr<cli::Menu> &rootMenu)
{
    try
    {
        auto subjectMenu = std::make_unique<cli::Menu>("subject", "Subject commands (MENU)");

        subjectMenu->Insert(
            "subject_add", {"category_id"},
            [](std::ostream &out, unsigned int category_id)
            {
                std::string file_path;
                rst_code_e rst = FileHandler::get_file_path_from_user_selection(file_path);
                if(rst == RST_OK){
                    logger->info("File path: {}", file_path);
                    std::filesystem::path p(file_path);
                    terminal_session->subject_add(out, (p.stem()).string(), category_id, file_path);
                }else{
                    logger->error("Error getting file path. {}", get_rst_txt(rst));
                }
            },
            "Add new subject");

        subjectMenu->Insert(
            "subject_update", {"id", "new_name", "new_category_id", "file_path_new"},
            [](std::ostream &out, unsigned int id, const std::string &new_name, const unsigned int new_category_id, const std::string &file_path_new)
            {
                terminal_session->subject_update(out, id, new_name, new_category_id, file_path_new);
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
