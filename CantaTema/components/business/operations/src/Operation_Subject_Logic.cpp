
#include <string>

#include "primitives/tool_paths.hpp"
#include "file_handler/text_handler.hpp"
#include "operations/Operation_Subject_Logic.hpp"

#include "database/db_main.hpp"
#include "database/db_subject.hpp"

Operation_Subject::Operation_Subject()
{
    DB_Main *db_main = DB_Main::getInstance();
}

Operation_Subject::~Operation_Subject()
{
}

rst_code_e Operation_Subject::subject_add(const std::string source_file, Subject &subject)
{
    DB_Subject db_subject;
    
    std::vector<std::shared_ptr<Subject>> subjects;
    rst_code_e rst = db_subject.get_all_subjects_by_user(subject.get_user_id(), subjects);
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
    std::string dst_file = (ToolPath::get_path_for_files() / std::to_string(subject.get_user_id()) / std::to_string(subject.get_id()) / p.filename()).string();

    TextFileHandler text_handler(source_file);
    rst = text_handler.upload_file(dst_file);
    if(rst != RST_OK){
        logger->warn("Error uploading file: {}", get_rst_txt(rst));
        if (db_subject.remove_subject(subject.get_id()) != RST_OK)
        {
            logger->warn("Undo database change");
        }
        return rst;
    }

    //Update database with new destination path
    subject.set_filepath(dst_file);
    rst = db_subject.update_subject(subject);

    if(rst != RST_OK){
        logger->warn("Error updating subject: {}", get_rst_txt(rst));
        return subject_remove(subject.get_id());
    }

    return RST_OK;
}

rst_code_e Operation_Subject::subject_update(const Subject &subject)
{
    DB_Subject db_subject;

    std::vector<std::shared_ptr<Subject>> subjects;
    rst_code_e rst = subject_get_all_by_user(subject.get_user_id(), subjects);
    if (rst != RST_OK)
        return rst;

    for (const auto &existing : subjects)
    {
        if (existing->get_name() == subject.get_name() && existing->get_id() != subject.get_id())
        {
            return SUBJECT_DUPLICATED;
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

rst_code_e Operation_Subject::subject_remove(unsigned int id)
{
    DB_Subject db_subject;
    FileHandler file_handler;

    std::shared_ptr<Subject> subject = nullptr;
    rst_code_e rst = subject_get_by_id(id, subject);
    if (rst != RST_OK)
        return rst;
    
    logger->debug("Removing subject: {}", subject->get_name());

    rst = db_subject.remove_subject(id);

    if (rst != RST_OK)
    {
        logger->warn("Error when deleting a subject: {}", get_rst_txt(rst));
        return SUBJECT_ERROR;
    }

    if(subject->get_filepath() != ""){
        std::filesystem::path file_path(subject->get_filepath());
        std::filesystem::path parent_path = file_path.parent_path();
        rst = file_handler.remove_folder(parent_path.string());
        if(rst != RST_OK){
            logger->warn("Error when deleting subject folder: {}", get_rst_txt(rst));
            return SUBJECT_ERROR;
        }
    }

    return RST_OK;
}

rst_code_e Operation_Subject::subject_get_by_id(unsigned int id, std::shared_ptr<Subject> &subject)
{
    DB_Subject db_subject;

    rst_code_e rst = db_subject.get_subject_by_id(id, subject);
    if (rst != RST_OK)
    {
        logger->warn("Get subject by id error ({})", get_rst_txt(rst));
        return SUBJECT_NOT_FOUND;
    }

    return RST_OK;
}

rst_code_e Operation_Subject::subject_get_all_by_category(unsigned int category_id, std::vector<std::shared_ptr<Subject>> &subjects)
{
    DB_Subject db_subject;

    rst_code_e rst = db_subject.get_all_subjects_by_category(category_id, subjects);

    if (rst != RST_OK)
    {
        logger->warn("Get all subjects by category error ({})", get_rst_txt(rst));
        return SUBJECT_ERROR;
    }

    return RST_OK;
}

rst_code_e Operation_Subject::subject_get_all_by_user(unsigned int user_id, std::vector<std::shared_ptr<Subject>> &subjects)
{
    DB_Subject db_subject;

    rst_code_e rst = db_subject.get_all_subjects_by_user(user_id, subjects);

    if (rst != RST_OK)
    {
        logger->warn("Get all subjects by user error ({})", get_rst_txt(rst));
        return SUBJECT_ERROR;
    }

    return RST_OK;
}