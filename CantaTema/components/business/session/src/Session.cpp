
#include "session/Session.hpp"

#include "configuration/configuration_system.hpp"

#include "operations/operation_user.hpp"
#include "operations/operation_category.hpp"
#include "operations/operation_subject.hpp"
#include "operations/operation_user_metrics.hpp"
#include "operations/operation_practice_event.hpp"
#include "operations/operation_coverage.hpp"
#include "database/db_coverage.hpp"

Session::Session(
    std::shared_ptr<IOperationUser> &&_user_op,
    std::shared_ptr<IOperationCategory> &&_category_op,
    std::shared_ptr<IOperationSubject> &&_subject_op,
    std::shared_ptr<IOperationUserMetrics> &&_user_metrics_op,
    std::shared_ptr<IOperationPracticeEvent> &&_practice_event_op,
    std::shared_ptr<IOperationCoverage> &&_coverage_op,
    std::shared_ptr<IDatabase> &&_db_op
) : user_op(std::move(_user_op)),
    category_op(std::move(_category_op)),
    subject_op(std::move(_subject_op)),
    user_metrics_op(std::move(_user_metrics_op)),
    practice_event_op(std::move(_practice_event_op)),
    coverage_op(std::move(_coverage_op)),
    db_coverage_op(std::move(_db_op))
{
    if (user_op == nullptr || category_op == nullptr || subject_op == nullptr || user_metrics_op == nullptr || practice_event_op == nullptr)
    {
        throw std::runtime_error("Operation session received wrong operation instances.");
    }
    if (db_coverage_op == nullptr)
    {
        db_coverage_op = std::make_shared<DB_Coverage>();
    }
    if (coverage_op == nullptr)
    {
        coverage_op = std::make_shared<OperationCoverage>(db_coverage_op, subject_op, practice_event_op);
    }
    initialize();
}

Session::Session(void)
{
    user_metrics_op = std::make_shared<OperationUserMetrics>();
    category_op = std::make_shared<OperationCategory>();
    user_op = std::make_shared<OperationUser>(std::shared_ptr<IOperationUserMetrics>(user_metrics_op));
    subject_op = std::make_shared<OperationSubject>(std::shared_ptr<IOperationUserMetrics>(user_metrics_op), std::shared_ptr<IOperationCategory>(category_op));
    practice_event_op = std::make_shared<OperationPracticeEvent>(std::shared_ptr<IOperationUserMetrics>(user_metrics_op), std::shared_ptr<IOperationSubject>(subject_op));
    db_coverage_op = std::make_shared<DB_Coverage>();
    coverage_op = std::make_shared<OperationCoverage>(db_coverage_op, subject_op, practice_event_op);

    if (user_op == nullptr || category_op == nullptr || subject_op == nullptr || user_metrics_op == nullptr || practice_event_op == nullptr || coverage_op == nullptr || db_coverage_op == nullptr)
    {
        throw std::runtime_error("Operation session received wrong operation instances. (2)");
    }
    initialize();
}

Session::~Session(void)
{
    session_user = nullptr;
}

rst_code_e Session::initialize(void){

    ConfigurationSystem &config = ConfigurationSystem::getInstance();

    session_user = nullptr;
    return RST_OK;
}

rst_code_e Session::user_add(const std::string &name, const std::string &password)
{
    return user_op->user_add(name, password);
}

rst_code_e Session::user_get(std::shared_ptr<const User> &user)
{

    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    user = std::const_pointer_cast<const User>(session_user);

    return RST_OK;
}

rst_code_e Session::user_get_by_name(std::string user_name, std::shared_ptr<const User> &user)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return user_op->user_get_by_name(user_name, user);
}

rst_code_e Session::user_update(std::shared_ptr<const User> &user)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    if (session_user->get_name() != user->get_name())
        return USER_ERROR;

    return user_op->user_update(user);
}

rst_code_e Session::user_remove(void)
{

    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    rst_code_e rst = user_op->user_remove();
    if (rst)
        return rst;

    // Clear local user variable due to we remove the current user
    session_user = nullptr;

    return RST_OK;
}

bool Session::user_is_authenticated(void)
{
    if (session_user == nullptr)
        return false;

    return user_op->user_is_authenticated();
}

rst_code_e Session::user_identify(const std::string &name, const std::string &password)
{
    // Remove session information
    session_user = nullptr;

    rst_code_e rst = user_op->user_identify(name, password);
    if (rst)
        return rst;

    rst = user_op->user_get(session_user);
    if (rst)
        return rst;

    return RST_OK;
}

rst_code_e Session::category_add(const std::string &name)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    Category category(0, name);
    category.set_user_id(session_user->get_useraccountid());

    return category_op->category_add(session_user, category);
}

rst_code_e Session::category_update(const unsigned int category_id, const std::string &new_name)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    std::vector<std::shared_ptr<Category>> categories;
    rst_code_e rst = category_op->category_get_all_by_user(session_user, categories);
    if (rst != RST_OK)
        return rst;

    for (auto &cat : categories)
    {
        if (cat->get_id() == category_id)
        {
            cat->set_name(new_name);
            return category_op->category_update(session_user, *cat);
        }
    }
    return CATEGORY_NOT_FOUND;
}

rst_code_e Session::category_remove(const unsigned int category_id)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    std::vector<std::shared_ptr<Category>> categories;
    rst_code_e rst = category_op->category_get_all_by_user(session_user, categories);
    if (rst != RST_OK)
        return rst;

    for (auto &cat : categories)
    {
        if (cat->get_id() == category_id)
        {
            return category_op->category_remove(category_id);
        }
    }
    return CATEGORY_NOT_FOUND;
}

rst_code_e Session::category_get_by_user(std::vector<std::shared_ptr<const Category>> &categories)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    std::vector<std::shared_ptr<Category>> user_categories;
    rst_code_e rst = category_op->category_get_all_by_user(session_user, user_categories);
    if (rst != RST_OK)
        return rst;

    categories.clear();
    for (const auto &cat : user_categories)
    {
        categories.push_back(cat);
    }

    return RST_OK;
}

rst_code_e Session::subject_add(const std::string &name, unsigned int category_id, const std::string &file_path)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    Subject subject(0, name);
    subject.set_user_id(session_user->get_useraccountid());
    subject.set_category_id(category_id);

    return subject_op->subject_add(session_user, file_path, subject);
}

rst_code_e Session::subject_update(unsigned int id, const std::string &new_name, const unsigned int new_category_id, const std::string &file_path_new)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    std::shared_ptr<Subject> subject;
    rst_code_e rst = subject_op->subject_get_by_id(session_user, id, subject);
    if (rst != RST_OK)
        return rst;

    subject->set_name(new_name);
    subject->set_category_id(new_category_id);
    subject->set_filepath(file_path_new);

    return subject_op->subject_update(session_user, *subject);
}

rst_code_e Session::subject_remove(unsigned int id)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return subject_op->subject_remove(session_user, id);
}

rst_code_e Session::subject_get_by_id(unsigned int subject_id, std::shared_ptr<Subject> &subject)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return subject_op->subject_get_by_id(session_user, subject_id, subject);
}

rst_code_e Session::subject_get_by_category(unsigned int category_id, std::vector<std::shared_ptr<Subject>> &subjects)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    std::vector<std::shared_ptr<Category>> categories;
    rst_code_e rst = category_op->category_get_all_by_user(session_user, categories);
    if (rst != RST_OK)
        return rst;

    bool found = false;
    for (const auto &cat : categories)
    {
        if (cat->get_id() == category_id)
        {
            found = true;
            break;
        }
    }

    if (!found)
        return CATEGORY_NOT_FOUND;

    std::vector<std::shared_ptr<Subject>> user_subjects;
    rst = subject_op->subject_get_all_by_category(session_user, category_id, user_subjects);
    if (rst != RST_OK)
        return rst;

    subjects.clear();
    for (const auto &sub : user_subjects)
    {
        subjects.push_back(sub);
    }

    return RST_OK;
}

rst_code_e Session::subject_get_by_user(std::vector<std::shared_ptr<Subject>> &subjects)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    std::vector<std::shared_ptr<Subject>> user_subjects;
    rst_code_e rst = subject_op->subject_get_all_by_user(session_user, user_subjects);
    if (rst != RST_OK)
        return rst;

    subjects.clear();
    for (const auto &sub : user_subjects)
    {
        subjects.push_back(sub);
    }

    return RST_OK;
}

rst_code_e Session::set_subject_language(int subject_id, const std::string &language)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    std::shared_ptr<Subject> subject;
    rst_code_e rst = subject_op->subject_get_by_id(session_user, subject_id, subject);
    if (rst != RST_OK)
        return rst;

    subject->set_language(language);
    return subject_op->subject_update(session_user, *subject);
}

rst_code_e Session::user_metrics_get(std::shared_ptr<const UserMetrics> &user_metrics)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    std::shared_ptr<UserMetrics> temp_metrics;
    rst_code_e rst = user_metrics_op->user_metrics_get(session_user, temp_metrics);
    user_metrics = temp_metrics;

    return rst;
}

rst_code_e Session::practice_event_add_planned(PracticeEvent &practice)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return practice_event_op->practice_event_add_planned(session_user, practice);
}

rst_code_e Session::practice_event_add_recorded(const std::string &source_file, PracticeEvent &practice)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return practice_event_op->practice_event_add_recorded(session_user, source_file, practice);
}

rst_code_e Session::practice_event_update(const PracticeEvent &practice)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return practice_event_op->practice_event_update(session_user, practice);
}

rst_code_e Session::practice_event_remove(unsigned int id)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return practice_event_op->practice_event_remove(session_user, id);
}

rst_code_e Session::practice_event_get_by_id(unsigned int id, std::shared_ptr<PracticeEvent> &practice)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return practice_event_op->practice_event_get_by_id(session_user, id, practice);
}

rst_code_e Session::practice_event_get_by_subject(unsigned int subject_id, std::vector<std::shared_ptr<PracticeEvent>> &practices)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return practice_event_op->practice_event_get_all_by_subject(session_user, subject_id, practices);
}

rst_code_e Session::practice_event_get_by_user(std::vector<std::shared_ptr<PracticeEvent>> &practices)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return practice_event_op->practice_event_get_all_by_user(session_user, practices);
}

rst_code_e Session::analyze_practice_coverage(
    int practice_id,
    std::string &out_execution_id,
    const std::string &whisper_model,
    const std::string &llama_model,
    float similarity_threshold,
    const std::string &language
)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    if (!coverage_op)
        return UNKNOWN;

    return coverage_op->analyze_practice_coverage(
        session_user,
        practice_id,
        whisper_model,
        llama_model,
        similarity_threshold,
        language,
        out_execution_id
    );
}

rst_code_e Session::get_analysis_executions_for_practice(int practice_id, std::string &executions_list_json)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    std::shared_ptr<PracticeEvent> practice;
    rst_code_e rst = practice_event_op->practice_event_get_by_id(session_user, practice_id, practice);
    if (rst != RST_OK)
        return rst;

    if (!db_coverage_op)
        return UNKNOWN;

    return db_coverage_op->get_analysis_executions_for_practice(practice_id, executions_list_json);
}

rst_code_e Session::get_analysis_execution_details(const std::string &execution_id, std::string &report_json, std::string &config_json)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    if (!db_coverage_op)
        return UNKNOWN;

    return db_coverage_op->get_analysis_execution_details(execution_id, report_json, config_json);
}
