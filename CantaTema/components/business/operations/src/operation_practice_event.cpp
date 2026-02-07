#include "operations/operation_practice_event.hpp"

#include "database/db_main.hpp"
#include "database/db_practice_event.hpp"
#include "primitives/tool_paths.hpp"
#include "file_handler/text_handler.hpp"
#include <filesystem>

OperationPracticeEvent::OperationPracticeEvent(
    std::shared_ptr<IOperationUserMetrics> &&_user_metrics_op, std::shared_ptr<IOperationSubject> &&_subject_op
) : user_metrics_op(std::move(_user_metrics_op)), subject_op(std::move(_subject_op))
{
    DB_Main *db_main = DB_Main::getInstance();
}

OperationPracticeEvent::~OperationPracticeEvent()
{
}

rst_code_e OperationPracticeEvent::practice_event_add_planned(const std::shared_ptr<const User> &user, PracticeEvent &practice)
{
    DB_PracticeEvent db_practice;
    rst_code_e rst;

    if(user == nullptr || user->get_useraccountid() == 0){
        logger->error("User info error");
        return PRACTICE_EVENT_ERROR;
    }

    practice.set_user_id(user->get_useraccountid());
    practice.set_status(PracticeEvent::PracticeEvent_status::PLANNED);

    rst = check_class_consistency(&practice);
    if(rst != RST_OK){
        return rst;
    }

    rst = db_practice.add_new_practice_event(practice);

    if (rst != RST_OK)
    {
        logger->warn("Error when adding a new practice event: {}", get_rst_txt(rst));
        return PRACTICE_EVENT_ERROR;
    }

    return RST_OK;
}

rst_code_e OperationPracticeEvent::practice_event_add_recorded(const std::shared_ptr<const User> &user, const std::string source_file, PracticeEvent &practice)
{
    DB_PracticeEvent db_practice;
    rst_code_e rst;
    TextFileHandler text_handler(source_file);

    if(user == nullptr || user->get_useraccountid() == 0){
        logger->error("User info error");
        return PRACTICE_EVENT_ERROR;
    }

    std::uintmax_t file_size_in_kb = text_handler.get_file_size_in_bytes() / 1024;
    rst = user_metrics_op->user_metrics_can_accept_file_size(user, file_size_in_kb);
    if(rst != RST_OK){
        logger->error("User metrics operation error: {}", get_rst_txt(rst));
        return rst;
    }

    practice.set_user_id(user->get_useraccountid());
    practice.set_status(PracticeEvent::PracticeEvent_status::RECORDED);

    rst = check_class_consistency(&practice);
    if(rst != RST_OK){
        return rst;
    }

    rst = db_practice.add_new_practice_event(practice);

    if (rst != RST_OK)
    {
        logger->warn("Error when adding a new practice event: {}", get_rst_txt(rst));
        return PRACTICE_EVENT_ERROR;
    }

    std::filesystem::path p(source_file);
    std::string dst_file = (ToolPath::get_path_for_subject(user->get_useraccountid(), user->get_useraccountid()) / std::to_string(practice.get_id()) / p.filename()).string();
    
    unsigned int uploaded_bytes;
    rst = text_handler.upload_file(dst_file, uploaded_bytes);
    if(rst != RST_OK){
        logger->warn("Error uploading file: {}", get_rst_txt(rst));
        db_practice.remove_practice_event(practice.get_id());
        return rst;
    }

    std::shared_ptr<UserMetrics> current_metrics = nullptr;
    rst = user_metrics_op->user_metrics_get(user, current_metrics);
    if(rst == RST_OK){
        current_metrics->set_space_used_kb(current_metrics->get_space_used_kb() + (uploaded_bytes / 1024));
        user_metrics_op->user_metrics_update(user, *current_metrics);
    }

    practice.set_filepath(dst_file);
    rst = db_practice.update_practice_event(practice);
    if(rst != RST_OK){
        logger->warn("Error updating practice event path: {}", get_rst_txt(rst));
    }

    return RST_OK;
}

rst_code_e OperationPracticeEvent::check_updates(const PracticeEvent *from, const PracticeEvent *to) const
{
    if(
        from->get_status() == PracticeEvent::PracticeEvent_status::RECORDED &&
        to->get_status() == PracticeEvent::PracticeEvent_status::PLANNED
    ){
        logger->error("A recorded practice event cannot be updated to planned");
        return PRACTICE_EVENT_ILLEGAL_CHANGE;
    }

    if(
        from->get_status() == PracticeEvent::PracticeEvent_status::REMOVED &&
        to->get_status() == PracticeEvent::PracticeEvent_status::PLANNED &&
        from->get_filepath().empty()
    ){
        logger->error("Impossible to update from removed to planned without removing the path first");
        return PRACTICE_EVENT_ILLEGAL_CHANGE;
    }

    if(
        from->get_status() == PracticeEvent::PracticeEvent_status::REMOVED &&
        to->get_status() == PracticeEvent::PracticeEvent_status::RECORDED &&
        !to->get_filepath().empty()
    ){
        logger->error("Impossible to update from removed to recorded without having a path first");
        return PRACTICE_EVENT_ILLEGAL_CHANGE;
    }

    if(
        from->get_status() == PracticeEvent::PracticeEvent_status::PLANNED &&
        to->get_status() == PracticeEvent::PracticeEvent_status::REMOVED
    ){
        logger->error("Impossible to update from planned to removed. Remove the event.");
        return PRACTICE_EVENT_ILLEGAL_CHANGE;
    }

    rst_code_e rst = check_class_consistency(to);
    if(rst != RST_OK){
        return rst;
    }

    return RST_OK;
}

rst_code_e OperationPracticeEvent::check_class_consistency(const PracticeEvent *event) const
{
    rst_code_e rst = RST_OK;

    if(event->get_user_id() == 0){
        logger->error("Invalid user id");
        return PRACTICE_EVENT_ERROR;
    }

    if(event->get_subject_id() == 0){
        logger->error("Invalid subject id");
        return PRACTICE_EVENT_ERROR;
    }

    if(event->get_status() == PracticeEvent::PracticeEvent_status::UNKNOWN){
        logger->error("Invalid status");
        return PRACTICE_EVENT_ERROR;
    }

    if(event->get_recorded_date()){
        if(event->get_recorded_date() < event->get_date()){
            logger->error("Recorded date cannot happen before the even date");
            return PRACTICE_EVENT_DATE_MISSMATCH;
        }
    }

    std::shared_ptr<Subject> subject;
    rst = subject_op->subject_get_by_id(event->get_subject_id(), subject);
    if(rst != RST_OK){
        return rst;
    }

    if(subject->get_user_id() != event->get_user_id()){
        return PRACTICE_EVENT_NOT_FOUND;
    }

    return RST_OK;
}

rst_code_e OperationPracticeEvent::practice_event_update(const std::shared_ptr<const User> &user, const PracticeEvent &practice)
{
    DB_PracticeEvent db_practice;

    if(user == nullptr || user->get_useraccountid() == 0){
        logger->error("User info error");
        return PRACTICE_EVENT_ERROR;
    }

    std::shared_ptr<PracticeEvent> existing_practice = nullptr;
    rst_code_e rst = practice_event_get_by_id(user, practice.get_id(), existing_practice);
    if (rst != RST_OK) {
        return rst;
    }

    if(existing_practice->get_filepath() != practice.get_filepath()){
        logger->error("Impossible to change file path directly.");
        return PRACTICE_EVENT_ILLEGAL_CHANGE;
    }

    if (existing_practice->get_user_id() != user->get_useraccountid()) {
        logger->error("User ID does not match");
        return PRACTICE_EVENT_ERROR;
    }

    rst = check_updates(existing_practice.get(), &practice);
    if(rst != RST_OK){
        return rst;
    }

    rst = db_practice.update_practice_event(practice);
    if (rst != RST_OK)
    {
        logger->warn("Update practice event error ({})", get_rst_txt(rst));
        return PRACTICE_EVENT_ERROR;
    }

    return RST_OK;
}

rst_code_e OperationPracticeEvent::practice_event_remove(const std::shared_ptr<const User> &user, unsigned int id)
{
    DB_PracticeEvent db_practice;

    if(user == nullptr || user->get_useraccountid() == 0){
        logger->error("User info error");
        return PRACTICE_EVENT_ERROR;
    }

    std::shared_ptr<PracticeEvent> existing_practice = nullptr;
    rst_code_e rst = practice_event_get_by_id(user, id, existing_practice);
    if (rst != RST_OK) {
        return rst;
    }

    if (existing_practice->get_user_id() != user->get_useraccountid()) {
        logger->error("User ID does not match or invalid user");
        return PRACTICE_EVENT_ERROR;
    }

    if(!existing_practice->get_filepath().empty()){
        TextFileHandler file_handler(existing_practice->get_filepath());
        std::uintmax_t file_size = file_handler.get_file_size_in_bytes();
        
        std::filesystem::path file_path(existing_practice->get_filepath());
        std::filesystem::remove(file_path);

        std::shared_ptr<UserMetrics> current_metrics = nullptr;
        if(user_metrics_op->user_metrics_get(user, current_metrics) == RST_OK){
             std::uintmax_t used = current_metrics->get_space_used_kb();
             std::uintmax_t freed = file_size / 1024;
             if(used > freed) current_metrics->set_space_used_kb(used - freed);
             else current_metrics->set_space_used_kb(0);
             user_metrics_op->user_metrics_update(user, *current_metrics);
        }
    }

    rst = db_practice.remove_practice_event(id);

    if (rst != RST_OK)
    {
        logger->warn("Error when deleting a practice event: {}", get_rst_txt(rst));
        return PRACTICE_EVENT_ERROR;
    }

    return RST_OK;
}

rst_code_e OperationPracticeEvent::practice_event_remove_by_subject_id(const std::shared_ptr<const User> &user, unsigned int id)
{
    DB_PracticeEvent db_practice;

    if(user == nullptr || user->get_useraccountid() == 0){
        logger->error("User info error");
        return PRACTICE_EVENT_ERROR;
    }

    std::shared_ptr<Subject> subject;
    rst_code_e rst = subject_op->subject_get_by_id(id, subject);
    if (rst != RST_OK)
    {
        return rst;
    }

    if (subject->get_user_id() != user->get_useraccountid())
    {
        logger->error("Subject does not belong to user");
        return PRACTICE_EVENT_ERROR;
    }

    std::vector<std::shared_ptr<PracticeEvent>> practices;
    if (db_practice.get_all_practice_events_by_subject(id, practices) == RST_OK) {
        for (const auto& p : practices) {
            practice_event_remove(user, p->get_id());
        }
    }

    rst = db_practice.remove_all_practice_events_by_subject(id);

    if (rst != RST_OK)
    {
        logger->warn("Error when deleting practice events by subject: {}", get_rst_txt(rst));
        return PRACTICE_EVENT_ERROR;
    }

    return RST_OK;
}

rst_code_e OperationPracticeEvent::practice_event_remove_by_user_id(const std::shared_ptr<const User> &user)
{
    DB_PracticeEvent db_practice;

    if(user == nullptr || user->get_useraccountid() == 0){
        logger->error("User info error");
        return PRACTICE_EVENT_ERROR;
    }

    std::vector<std::shared_ptr<PracticeEvent>> practices;
    if (db_practice.get_all_practice_events_by_user(user->get_useraccountid(), practices) == RST_OK) {
        for (const auto& p : practices) {
            practice_event_remove(user, p->get_id());
        }
    }

    rst_code_e rst = db_practice.remove_all_practice_events_by_user(user->get_useraccountid());

    if (rst != RST_OK)
    {
        logger->warn("Error when deleting practice events by user: {}", get_rst_txt(rst));
        return PRACTICE_EVENT_ERROR;
    }

    return RST_OK;
}

rst_code_e OperationPracticeEvent::practice_event_get_by_id(const std::shared_ptr<const User> &user, unsigned int id, std::shared_ptr<PracticeEvent> &practice)
{
    DB_PracticeEvent db_practice;

    if(user == nullptr || user->get_useraccountid() == 0){
        logger->error("User info error");
        return PRACTICE_EVENT_ERROR;
    }

    rst_code_e rst = db_practice.get_practice_event_by_id(id, practice);
    if (rst != RST_OK)
    {
        logger->warn("Get practice event by id error ({})", get_rst_txt(rst));
        return PRACTICE_EVENT_NOT_FOUND;
    }

    if (practice->get_user_id() != user->get_useraccountid())
    {
        logger->error("Practice event does not belong to user");
        practice = nullptr;
        return PRACTICE_EVENT_NOT_FOUND;
    }

    return RST_OK;
}

rst_code_e OperationPracticeEvent::practice_event_get_all_by_subject(const std::shared_ptr<const User> &user, unsigned int subject_id, std::vector<std::shared_ptr<PracticeEvent>> &practices)
{
    DB_PracticeEvent db_practice;

    if(user == nullptr || user->get_useraccountid() == 0){
        logger->error("User info error");
        return PRACTICE_EVENT_ERROR;
    }

    std::shared_ptr<Subject> subject;
    rst_code_e rst = subject_op->subject_get_by_id(subject_id, subject);
    if (rst != RST_OK)
    {
        return rst;
    }

    if (subject->get_user_id() != user->get_useraccountid())
    {
        logger->error("Subject does not belong to user");
        return PRACTICE_EVENT_ERROR;
    }

    rst = db_practice.get_all_practice_events_by_subject(subject_id, practices);

    if (rst != RST_OK)
    {
        logger->warn("Get all practice events by subject error ({})", get_rst_txt(rst));
        return PRACTICE_EVENT_ERROR;
    }

    return RST_OK;
}

rst_code_e OperationPracticeEvent::practice_event_get_all_by_user(const std::shared_ptr<const User> &user, std::vector<std::shared_ptr<PracticeEvent>> &practices)
{
    DB_PracticeEvent db_practice;

    if(user == nullptr || user->get_useraccountid() == 0){
        logger->error("User info error");
        return PRACTICE_EVENT_ERROR;
    }

    rst_code_e rst = db_practice.get_all_practice_events_by_user(user->get_useraccountid(), practices);

    if (rst != RST_OK)
    {
        logger->warn("Get all practice events by user error ({})", get_rst_txt(rst));
        return PRACTICE_EVENT_ERROR;
    }

    return RST_OK;
}