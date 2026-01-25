
#ifndef __SESSION_HPP
#define __SESSION_HPP

#include "operations/IOperation_User.hpp"
#include "operations/IOperation_Category.hpp"
#include "operations/IOperation_Subject.hpp"


class Session : public IOperation_User
{

public:
    Session(std::shared_ptr<IOperation_User> &&_user_op, std::shared_ptr<IOperation_Category> &&_category_op, std::shared_ptr<IOperation_Subject> &&_subject_op);
    Session(void);
    ~Session(void);

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
    rst_code_e subject_get_by_category(unsigned int category_id, std::vector<std::shared_ptr<const Subject>> &subjects);
    rst_code_e subject_get_by_user(std::vector<std::shared_ptr<const Subject>> &subjects);

    //-------------------------------------------------------------------------------------

    

private:
    std::shared_ptr<IOperation_User> user_op{nullptr};
    std::shared_ptr<IOperation_Category> category_op{nullptr};
    std::shared_ptr<IOperation_Subject> subject_op{nullptr};

    //-------------------------------------------------------------------------------------

    std::shared_ptr<const User> session_user{nullptr};
};

#endif //__SESSION_HPP