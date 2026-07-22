

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

    void subject_add(std::ostream &out, unsigned int category_id, const std::string &name);
    void subject_add_from_path(std::ostream &out, unsigned int category_id, const std::string &name, const std::string &file_path);
    void subject_update(std::ostream &out, unsigned int id, const unsigned int new_category_id, const std::string &new_name);
    void subject_remove(std::ostream &out, unsigned int id);
    void subject_get_by_category(std::ostream &out, unsigned int category_id);
    void subject_get_by_user(std::ostream &out);
    void subject_set_language(std::ostream &out, unsigned int subject_id, const std::string &language);

    //-------------------------------------------------------------------------------------

    void user_metrics_get(std::ostream &out);

    //-------------------------------------------------------------------------------------

    void practice_event_add_planned(std::ostream &out, unsigned int subject_id, const std::string &date, const std::string &description);
    void practice_event_add_recorded(std::ostream &out, unsigned int subject_id, const std::string name);
    void practice_event_add_recorded_from_file(std::ostream &out, unsigned int subject_id, const std::string &date);
    void practice_event_play(std::ostream &out, unsigned int id);
    void practice_event_update(std::ostream &out, unsigned int id, const std::string &new_status, const std::string &description);
    void practice_event_remove(std::ostream &out, unsigned int id);
    void practice_event_get_by_id(std::ostream &out, unsigned int id);
    void practice_event_get_by_subject(std::ostream &out, unsigned int subject_id);
    void practice_event_get_by_user(std::ostream &out);

    //-------------------------------------------------------------------------------------

    void whisper_get_available_models(std::ostream &out);
    void whisper_download_model(std::ostream &out, const std::string &model_name);
    void models_get_available(std::ostream &out);
    void models_download(std::ostream &out, const std::string &model_type_str, const std::string &model_name);

    //-------------------------------------------------------------------------------------

    void coverage_analyze(std::ostream &out, unsigned int practice_id, const std::string &whisper_model = "", const std::string &llama_model = "", float similarity_threshold = 0.0f, const std::string &language = "");
    void coverage_history(std::ostream &out, unsigned int practice_id);
    void coverage_report(std::ostream &out, const std::string &execution_id);
    

private:
    Session *op;
    ThreadPool *session_thread_pool;

    const size_t session_thread_pool_number = 1;
};

#endif //__TERMINAL_SESSION_HPP