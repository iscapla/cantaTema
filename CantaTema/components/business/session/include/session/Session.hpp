
#ifndef __SESSION_HPP
#define __SESSION_HPP

#include "operations/i_operation_user.hpp"
#include "operations/i_operation_category.hpp"
#include "operations/i_operation_subject.hpp"
#include "operations/i_operation_user_metrics.hpp"
#include "operations/i_operation_practice_event.hpp"
#include "operations/i_operation_coverage.hpp"
#include "database/i_database.hpp"


class Session : public IOperationUser
{

public:
    Session(
        std::shared_ptr<IOperationUser> &&_user_op,
        std::shared_ptr<IOperationCategory> &&_category_op,
        std::shared_ptr<IOperationSubject> &&_subject_op,
        std::shared_ptr<IOperationUserMetrics> &&_user_metrics_op,
        std::shared_ptr<IOperationPracticeEvent> &&_practice_event_op,
        std::shared_ptr<IOperationCoverage> &&_coverage_op = nullptr,
        std::shared_ptr<IDatabase> &&_db_op = nullptr
    );
    Session(void);
    ~Session(void);

    rst_code_e initialize(void);

    //-------------------------------------------------------------------------------------

    rst_code_e user_add(const std::string &name, const std::string &password);
    rst_code_e user_get(std::shared_ptr<const User> &user);
    rst_code_e user_get_by_name(std::string user_name, std::shared_ptr<const User> &user);
    rst_code_e user_update(std::shared_ptr<const User> &user);
    rst_code_e user_remove(void);
    bool user_is_authenticated(void);
    rst_code_e user_identify(const std::string &name, const std::string &password);

    //-------------------------------------------------------------------------------------

    rst_code_e category_add(const std::string &name);
    rst_code_e category_update(const unsigned int category_id, const std::string &new_name);
    rst_code_e category_remove(const unsigned int category_id);
    rst_code_e category_get_by_user(std::vector<std::shared_ptr<const Category>> &categories);

    //-------------------------------------------------------------------------------------

    rst_code_e subject_add(const std::string &name, unsigned int category_id, const std::string &file_path);
    rst_code_e subject_update(unsigned int id, const std::string &new_name, const unsigned int new_category_id, const std::string &file_path_new);
    rst_code_e subject_remove(unsigned int id);
    rst_code_e subject_get_by_id(unsigned int subject_id, std::shared_ptr<Subject> &subject);
    rst_code_e subject_get_by_category(unsigned int category_id, std::vector<std::shared_ptr<Subject>> &subjects);
    rst_code_e subject_get_by_user(std::vector<std::shared_ptr<Subject>> &subjects);
    rst_code_e set_subject_language(int subject_id, const std::string &language);

    //-------------------------------------------------------------------------------------

    rst_code_e user_metrics_get(std::shared_ptr<const UserMetrics> &user_metrics);

    //-------------------------------------------------------------------------------------

    rst_code_e practice_event_add_planned(PracticeEvent &practice);
    rst_code_e practice_event_add_recorded(const std::string &source_file, PracticeEvent &practice);
    rst_code_e practice_event_update(const PracticeEvent &practice);
    rst_code_e practice_event_remove(unsigned int id);
    rst_code_e practice_event_get_by_id(unsigned int id, std::shared_ptr<PracticeEvent> &practice);
    rst_code_e practice_event_get_by_subject(unsigned int subject_id, std::vector<std::shared_ptr<PracticeEvent>> &practices);
    rst_code_e practice_event_get_by_user(std::vector<std::shared_ptr<PracticeEvent>> &practices);

    //-------------------------------------------------------------------------------------

    rst_code_e analyze_practice_coverage(
        int practice_id,
        std::string &out_execution_id,
        const std::string &whisper_model = "",
        const std::string &llama_model = "",
        float similarity_threshold = 0.0f,
        const std::string &language = ""
    );
    rst_code_e get_analysis_executions_for_practice(int practice_id, std::string &executions_list_json);
    rst_code_e get_analysis_execution_details(const std::string &execution_id, std::string &report_json, std::string &config_json);

private:
    std::shared_ptr<IOperationUser> user_op{nullptr};
    std::shared_ptr<IOperationCategory> category_op{nullptr};
    std::shared_ptr<IOperationSubject> subject_op{nullptr};
    std::shared_ptr<IOperationUserMetrics> user_metrics_op{nullptr};
    std::shared_ptr<IOperationPracticeEvent> practice_event_op{nullptr};
    std::shared_ptr<IOperationCoverage> coverage_op{nullptr};
    std::shared_ptr<IDatabase> db_coverage_op{nullptr};

    //-------------------------------------------------------------------------------------

    std::shared_ptr<const User> session_user{nullptr};
};

#endif //__SESSION_HPP