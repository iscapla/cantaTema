#include "terminal/terminal_session.hpp"
#include <iomanip>

TerminalSession::TerminalSession()
{
    op = new Session();
    session_thread_pool = new ThreadPool(session_thread_pool_number);

    session_thread_pool->threads_initialize();
}
TerminalSession::~TerminalSession()
{
    session_thread_pool->threads_destroy();
    delete session_thread_pool;

    delete op;
}

void TerminalSession::user_add(std::ostream &out, const std::string &name, const std::string &password)
{
    rst_code_e rst{};

    rst = op->user_add(name, password);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("User added");
    }
}

void TerminalSession::user_get(std::ostream &out)
{

    rst_code_e rst{};

    std::shared_ptr<const User> current_user{nullptr};

    rst = op->user_get(current_user);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        current_user->print();
    }
}

void TerminalSession::user_remove(std::ostream &out)
{

    rst_code_e rst{};

    rst = op->user_remove();
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("User removed");
    }
}

void TerminalSession::user_identify(std::ostream &out, const std::string &name, const std::string &password)
{
    rst_code_e rst{};

    rst = op->user_identify(name, password);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("User identified");
    }
}

void TerminalSession::category_add(std::ostream &out, const std::string &name)
{
    rst_code_e rst = op->category_add(name);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Category added");
    }
}

void TerminalSession::category_update(std::ostream &out, const unsigned int category_id, const std::string &new_name)
{
    rst_code_e rst = op->category_update(category_id, new_name);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Category updated");
    }
}

void TerminalSession::category_remove(std::ostream &out, const unsigned int category_id)
{
    rst_code_e rst = op->category_remove(category_id);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Category removed");
    }
}

void TerminalSession::category_get_by_user(std::ostream &out)
{
    std::vector<std::shared_ptr<const Category>> categories;
    rst_code_e rst = op->category_get_by_user(categories);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        out << std::left << std::setw(10) << "ID" << std::setw(30) << "Name" << std::endl;
        out << std::string(40, '-') << std::endl;

        for (const auto &cat : categories)
        {
            out << std::left << std::setw(10) << cat->get_id() << std::setw(30) << cat->get_name() << std::endl;
        }
    }
}

void TerminalSession::subject_add(std::ostream &out, const std::string &name, unsigned int category_id, const std::string &file_path)
{
    rst_code_e rst = op->subject_add(name, category_id, file_path);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Subject added");
    }
}

void TerminalSession::subject_update(std::ostream &out, unsigned int id, const std::string &new_name, const unsigned int new_category_id, const std::string &file_path_new)
{
    rst_code_e rst = op->subject_update(id, new_name, new_category_id, file_path_new);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Subject updated");
    }
}

void TerminalSession::subject_remove(std::ostream &out, unsigned int id)
{
    rst_code_e rst = op->subject_remove(id);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Subject removed");
    }
}

void TerminalSession::subject_get_by_category(std::ostream &out, unsigned int category_id)
{
    std::vector<std::shared_ptr<const Subject>> subjects;
    rst_code_e rst = op->subject_get_by_category(category_id, subjects);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        out << std::left << std::setw(10) << "ID" << std::setw(30) << "Name" << std::setw(30) << "Category" << std::setw(50) << "File Path" << std::endl;
        out << std::string(120, '-') << std::endl;

        for (const auto &sub : subjects)
        {
            std::string category_info = "None";
            if (sub->get_category())
            {
                category_info = std::to_string(sub->get_category()->get_id()) + "-" + sub->get_category()->get_name();
            }
            out << std::left << std::setw(10) << sub->get_id() << std::setw(30) << sub->get_name() << std::setw(30) << category_info << std::setw(50) << sub->get_filepath() << std::endl;
        }
    }
}

void TerminalSession::subject_get_by_user(std::ostream &out)
{
    std::vector<std::shared_ptr<const Subject>> subjects;
    rst_code_e rst = op->subject_get_by_user(subjects);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        out << std::left << std::setw(10) << "ID" << std::setw(30) << "Name" << std::setw(30) << "Category" << std::setw(50) << "File Path" << std::endl;
        out << std::string(120, '-') << std::endl;

        for (const auto &sub : subjects)
        {
            std::string category_info = "None";
            if (sub->get_category())
            {
                category_info = std::to_string(sub->get_category()->get_id()) + "-" + sub->get_category()->get_name();
            }
            out << std::left << std::setw(10) << sub->get_id() << std::setw(30) << sub->get_name() << std::setw(30) << category_info << std::setw(50) << sub->get_filepath() << std::endl;
        }
    }
}
