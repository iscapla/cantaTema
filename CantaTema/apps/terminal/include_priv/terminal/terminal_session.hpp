

#ifndef __TERMINAL_SESSION_HPP
#define __TERMINAL_SESSION_HPP

#include "session/Session.hpp"

#include "primitives/utils_thread_pool.hpp"

class TerminalSession
{
public:
    TerminalSession();
    ~TerminalSession();

    //-------------------------------------------------------------------------------------

    void db_purge(std::ostream &out);
    void test_start(std::ostream &out);

    //-------------------------------------------------------------------------------------

    void user_add(std::ostream &out, const std::string &name, const std::string &password);
    void user_get(std::ostream &out);
    void user_remove(std::ostream &out);
    void user_identify(std::ostream &out, const std::string &name, const std::string &password);

    //-------------------------------------------------------------------------------------

    void category_add(std::ostream &out, const std::string &name);
    void category_update(std::ostream &out, const unsigned int category_id, const std::string &new_name);
    void category_remove(std::ostream &out, const unsigned int category_id);
    void category_get_by_user(std::ostream &out);

    //-------------------------------------------------------------------------------------

    void subject_add(std::ostream &out, const std::string &name, unsigned int category_id, const std::string &file_path);
    void subject_update(std::ostream &out, unsigned int id, const std::string &new_name, const unsigned int new_category_id, const std::string &file_path_new);
    void subject_remove(std::ostream &out, unsigned int id);
    void subject_get_by_category(std::ostream &out, unsigned int category_id);
    void subject_get_by_user(std::ostream &out);

    //-------------------------------------------------------------------------------------

    void user_metrics_get(std::ostream &out);

    //-------------------------------------------------------------------------------------

    void practice_event_add_planned(std::ostream &out, unsigned int subject_id, unsigned int duration, const std::string &name);
    void practice_event_add_recorded(std::ostream &out, unsigned int subject_id, const std::string &source_file, const std::string &name);
    void practice_event_update(std::ostream &out, unsigned int id, unsigned int duration, const std::string &name);
    void practice_event_remove(std::ostream &out, unsigned int id);
    void practice_event_get_by_id(std::ostream &out, unsigned int id);
    void practice_event_get_by_subject(std::ostream &out, unsigned int subject_id);
    void practice_event_get_by_user(std::ostream &out);

private:
    Session *op;
    ThreadPool *session_thread_pool;

    const size_t session_thread_pool_number = 1;
};

#endif //__TERMINAL_SESSION_HPP