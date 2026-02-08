
#include <string>

#include "primitives/tool_paths.hpp"
#include "file_handler/text_handler.hpp"
#include "operations/operation_subject.hpp"

#include "database/db_main.hpp"
#include "database/db_subject.hpp"

OperationSubject::OperationSubject(std::shared_ptr<IOperationUserMetrics> &&_user_metrics_op, std::shared_ptr<IOperationCategory> &&_category_op) :
    user_metrics_op(std::move(_user_metrics_op)),
    category_op(std::move(_category_op))
{
    DB_Main *db_main = DB_Main::getInstance();
}

OperationSubject::~OperationSubject()
{
}

rst_code_e OperationSubject::subject_add(const std::shared_ptr<const User> &user, const std::string source_file, Subject &subject)
{
    DB_Subject db_subject;
    TextFileHandler text_handler(source_file);
    rst_code_e rst = RST_OK;

    if(user == nullptr || user->get_useraccountid() == 0){
        logger->error("User info error");
        return SUBJECT_ERROR;
    }

    std::uintmax_t file_size_in_kb = text_handler.get_file_size_in_bytes() / 1024;
    rst = user_metrics_op->user_metrics_can_accept_file_size(user, file_size_in_kb);
    if(rst != RST_OK){
        logger->error("User metrics operation error: {}", get_rst_txt(rst));
        return rst;
    }

    subject.set_user_id(user->get_useraccountid());
    
    if (subject.get_category_id() != 0)
    {
        if (category_op == nullptr)
        {
            logger->error("Category operation error");
            return SUBJECT_ERROR;
        }

        std::shared_ptr<Category> category_check;
        rst = category_op->category_get_by_id(subject.get_category_id(), category_check);
        if (rst != RST_OK)
        {
            logger->error("Category not found");
            return SUBJECT_ERROR;
        }

        if (category_check->get_user_id() != user->get_useraccountid())
        {
            logger->error("Category does not belong to user");
            return SUBJECT_ERROR;
        }
    }

    std::vector<std::shared_ptr<Subject>> subjects;
    rst = db_subject.get_all_subjects_by_user(user->get_useraccountid(), subjects);
    if (rst != RST_OK)
    {
        logger->error("Error checking for duplicate subject");
        return SUBJECT_ERROR;
    }

    for (const auto &existing : subjects)
    {
        if (existing->get_name() == subject.get_name())
        {
            logger->warn("Subject '{}' already exists.", subject.get_name());
            return SUBJECT_DUPLICATED;
        }
    }

    rst = db_subject.add_new_subject(subject);
    if (rst != RST_OK)
    {
        logger->warn("Error when adding a new subject: {}", get_rst_txt(rst));
        return SUBJECT_ERROR;
    }

    std::filesystem::path p(source_file);
    std::string dst_file = (ToolPath::get_path_for_subject(user->get_useraccountid(), subject.get_id()) / p.filename()).string();
    unsigned int uploaded_bytes;
    rst = text_handler.upload_file(dst_file, uploaded_bytes);
    if(rst != RST_OK){
        logger->warn("Error uploading file: {}", get_rst_txt(rst));
        if (db_subject.remove_subject(subject.get_id()) != RST_OK)
        {
            logger->warn("Undo database change");
        }
        return rst;
    }

    // Update user metrics with the new file size
    std::shared_ptr<UserMetrics> current_metrics = nullptr;
    rst = user_metrics_op->user_metrics_get(user, current_metrics);
    if(rst != RST_OK){
        logger->error("User metrics operation error: {}", get_rst_txt(rst));
        return rst;
    }
   
    current_metrics->set_space_used_kb(current_metrics->get_space_used_kb() + (uploaded_bytes / 1024));
    rst = user_metrics_op->user_metrics_update(user, *current_metrics);
    if(rst != RST_OK){
        logger->warn("User metrics update error: {}", get_rst_txt(rst));
    }

    //Update database with new destination path
    subject.set_filepath(dst_file);
    rst = db_subject.update_subject(subject);

    if(rst != RST_OK){
        logger->warn("Error updating subject: {}", get_rst_txt(rst));
        return subject_remove(user, subject.get_id());
    }

    return RST_OK;
}

rst_code_e OperationSubject::subject_update(const std::shared_ptr<const User> &user, const Subject &subject)
{
    DB_Subject db_subject;

    if(user == nullptr || user->get_useraccountid() == 0){
        logger->error("User info error");
        return SUBJECT_ERROR;
    }

    std::vector<std::shared_ptr<Subject>> subjects;
    rst_code_e rst = subject_get_all_by_user(user, subjects);
    if (rst != RST_OK)
        return rst;

    std::shared_ptr<Subject> tmp_subject = nullptr;

    // Check that the name is not duplicated
    for (const auto &existing : subjects)
    {
        if(existing->get_id() != subject.get_id()){
            tmp_subject = existing;  // Found operation
        }

        if (existing->get_name() == subject.get_name() && existing->get_id() != subject.get_id())
        {
            return SUBJECT_DUPLICATED;
        }
    }

    //Check that the update does not change some important values
    if(tmp_subject == nullptr){
        logger->error("Subject not found in user");
        return SUBJECT_ERROR;
    }

    if(tmp_subject->get_user_id() != user->get_useraccountid()){
        logger->error("User ID does not match");
        return SUBJECT_ERROR;
    }

    if(tmp_subject->get_filepath() != subject.get_filepath()){
        logger->error("Impossible to change file path. Remove the subject and add it again.");
        return SUBJECT_ERROR;
    }

    if (subject.get_category_id() != 0)
    {
        if (category_op == nullptr)
        {
            logger->error("Category operation error");
            return SUBJECT_ERROR;
        }

        std::shared_ptr<Category> category_check;
        rst = category_op->category_get_by_id(subject.get_category_id(), category_check);
        if (rst != RST_OK)
        {
            logger->error("Category not found");
            return SUBJECT_ERROR;
        }

        if (category_check->get_user_id() != user->get_useraccountid())
        {
            logger->error("Category does not belong to user");
            return SUBJECT_ERROR;
        }
    }

    rst = db_subject.update_subject(subject);
    if (rst != RST_OK)
    {
        logger->warn("Update subject error ({})", get_rst_txt(rst));
        return SUBJECT_ERROR;
    }

    return RST_OK;
}

rst_code_e OperationSubject::subject_remove(const std::shared_ptr<const User> &user, unsigned int id)
{
    DB_Subject db_subject;

    std::shared_ptr<Subject> subject = nullptr;
    rst_code_e rst = subject_get_by_id(user,id, subject);
    if (rst != RST_OK)
        return rst;
    
    logger->debug("Removing subject: {}", subject->get_name());

    rst = db_subject.remove_subject(id);
    if (rst != RST_OK)
    {
        logger->warn("Error when deleting a subject: {}", get_rst_txt(rst));
        return SUBJECT_ERROR;
    }

    TextFileHandler file_handler{subject->get_filepath()};
    std::uintmax_t file_size_in_bytes{};
    if(subject->get_filepath() != ""){
        std::filesystem::path file_path(subject->get_filepath());
        std::filesystem::path parent_path = file_path.parent_path();
        file_size_in_bytes = file_handler.get_file_size_in_bytes();
        rst = file_handler.remove_folder(parent_path.string());
        if(rst != RST_OK){
            logger->warn("Error when deleting subject folder: {}", get_rst_txt(rst));
            return SUBJECT_ERROR;
        }
    }

    // Update user metrics with the new file size
    std::shared_ptr<UserMetrics> current_metrics = nullptr;
    rst = user_metrics_op->user_metrics_get(user, current_metrics);
    if(rst != RST_OK){
        logger->error("User metrics operation error: {}", get_rst_txt(rst));
        return rst;
    }

    std::uintmax_t file_size_new = 0;
    if (current_metrics->get_space_used_kb() > (file_size_in_bytes / 1024)){
        file_size_new = current_metrics->get_space_used_kb() - (file_size_in_bytes / 1024);
    }
    current_metrics->set_space_used_kb(file_size_new);
    rst = user_metrics_op->user_metrics_update(user, *current_metrics);
    if(rst != RST_OK){
        logger->warn("User metrics update error: {}", get_rst_txt(rst));
    }

    return RST_OK;
}

rst_code_e OperationSubject::subject_get_by_id(const std::shared_ptr<const User> &user, unsigned int id, std::shared_ptr<Subject> &subject)
{
    DB_Subject db_subject;

    rst_code_e rst = db_subject.get_subject_by_id(id, subject);
    if (rst != RST_OK)
    {
        logger->warn("Get subject by id error ({})", get_rst_txt(rst));
        return SUBJECT_NOT_FOUND;
    }

    if(subject->get_user_id() != user->get_useraccountid())
    {
        logger->error("User ID does not match");
        subject = nullptr;
        return SUBJECT_NOT_FOUND;
    }

    return RST_OK;
}

rst_code_e OperationSubject::subject_get_all_by_category(const std::shared_ptr<const User> &user, unsigned int category_id, std::vector<std::shared_ptr<Subject>> &subjects)
{
    DB_Subject db_subject;

    rst_code_e rst = RST_OK;

    std::shared_ptr<Category> category = nullptr;
    rst = category_op->category_get_by_id(category_id, category);
    if (rst != RST_OK)
    {
        logger->warn("Category not found");
        return CATEGORY_NOT_FOUND;
    }

    if(category->get_user_id() != user->get_useraccountid()){
        logger->error("User ID does not match");
        return CATEGORY_NOT_FOUND;
    }

    rst = db_subject.get_all_subjects_by_category(category_id, subjects);
    if (rst != RST_OK)
    {
        logger->warn("Get all subjects by category error ({})", get_rst_txt(rst));
        return SUBJECT_ERROR;
    }

    return RST_OK;
}

rst_code_e OperationSubject::subject_get_all_by_user(const std::shared_ptr<const User> &user, std::vector<std::shared_ptr<Subject>> &subjects)
{
    DB_Subject db_subject;

    if(user == nullptr || user->get_useraccountid() == 0){
        logger->error("User info error");
        return SUBJECT_ERROR;
    }

    rst_code_e rst = db_subject.get_all_subjects_by_user(user->get_useraccountid(), subjects);

    if (rst != RST_OK)
    {
        logger->warn("Get all subjects by user error ({})", get_rst_txt(rst));
        return SUBJECT_ERROR;
    }

    return RST_OK;
}