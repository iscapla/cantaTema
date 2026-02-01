
#include "operations/operation_user.hpp"
#include "configuration/configuration_system.hpp"

#include "database/db_main.hpp"
#include "database/db_user.hpp"
#include "database/db_subject.hpp"
#include "database/db_category.hpp"
#include "file_handler/file_handler.hpp"
#include "primitives/tool_paths.hpp"

OperationUser::OperationUser(std::shared_ptr<IOperationUserMetrics> &&_user_metrics_op) : local_user(nullptr), user_metrics_op(std::move(_user_metrics_op))
{
    DB_Main *db_main = DB_Main::getInstance();
}

OperationUser::~OperationUser()
{
}

rst_code_e OperationUser::user_add(const std::string &name, const std::string &password)
{

    DB_User dbuser;
    rst_code_e rst;
    bool already_exists{true};

    rst = dbuser.is_user_already_present(name, already_exists);
    if (rst)
    {
        logger->error("Error checking for duplicate user");
        return USER_ERROR;
    }

    if (already_exists)
    {
        logger->warn("User already exists.");
        return USER_DUPLICATED;
    }

    User tmp_user{name};

    tmp_user.set_passwordkey(password);
    tmp_user.set_max_space_size_in_kb(ConfigurationSystem::getInstance().get_user_usage_limit_in_mb() * 1024);

    rst = dbuser.add_new_user(tmp_user);
    if (rst)
    {
        logger->warn("Error when adding a new user: {}", get_rst_txt(rst));
        return USER_ERROR;
    }

    UserMetrics metrics(tmp_user.get_useraccountid());
    std::shared_ptr<const User> tmp_user_ptr = std::make_shared<User>(tmp_user);
    rst = user_metrics_op->user_metrics_add(tmp_user_ptr, metrics);
    if (rst)
    {
        logger->warn("Error when adding a new user metrics: {}", get_rst_txt(rst));
        dbuser.remove_user(name);
        return USER_ERROR;
    }

    return RST_OK;
}

rst_code_e OperationUser::user_identify(const std::string &name, const std::string &password)
{
    rst_code_e rst;

    std::shared_ptr<const User> tmp_user = nullptr;

    rst = user_get_by_name(name, tmp_user);
    if (rst)
    {
        return USER_NOT_FOUND;
    }

    if (tmp_user->get_passwordkey() == password)
    {
        std::const_pointer_cast<User>(tmp_user)->set_is_authenticated(true);
        local_user = tmp_user; // Move user to local operation because it was identified correctly
        return RST_OK;
    }
    else
    {
        logger->debug("User error login.");
        return USER_NO_AUTH;
    }
}

rst_code_e OperationUser::user_get(std::shared_ptr<const User> &user)
{
    if (local_user)
    {
        user = local_user;
        return RST_OK;
    }
    else
    {
        return USER_NO_AUTH;
    }
}

rst_code_e OperationUser::user_get_by_name(std::string user_name, std::shared_ptr<const User> &user)
{

    DB_User dbuser;

    std::shared_ptr<User> tmp_user = std::make_shared<User>(user_name);

    rst_code_e rst = dbuser.get_user((*tmp_user));
    if (rst)
    {
        logger->warn("Get user error ({})", get_rst_txt(rst));
        return USER_NOT_FOUND;
    }

    user = tmp_user;
    return RST_OK;
}

rst_code_e OperationUser::user_update(std::shared_ptr<const User> &user)
{
    DB_User dbuser;

    std::shared_ptr<User> tmp_user = std::const_pointer_cast<User>(user);

    rst_code_e rst = dbuser.update_user((*tmp_user));
    if (rst)
    {
        logger->warn("Update user error ({})", get_rst_txt(rst));
        return USER_ERROR;
    }

    return RST_OK;
}

bool OperationUser::user_is_authenticated(void)
{

    if (local_user)
    {
        return local_user->get_is_authenticated();
    }

    return false;
}

rst_code_e OperationUser::user_remove(void)
{
    DB_User dbuser;
    rst_code_e rst;

    if (local_user)
    {

        unsigned int user_id = local_user->get_useraccountid();

        DB_Subject db_subject;
        std::vector<std::shared_ptr<Subject>> subjects;
        rst = db_subject.remove_all_subjects_from_user(user_id);
        if(rst){
            logger->warn("Error when deleting subjects: {}", get_rst_txt(rst));
            return USER_ERROR;
        }

        DB_Category db_category;
        rst = db_category.remove_all_categories_from_user(user_id);
        if(rst){
            logger->warn("Error when deleting categories: {}", get_rst_txt(rst));
            return USER_ERROR;
        }

        FileHandler file_handler;
        std::string user_folder = (ToolPath::get_path_for_files() / std::to_string(user_id)).string();
        rst = file_handler.remove_folder(user_folder);
        if(rst){
            logger->warn("Error when deleting user folder: {}", get_rst_txt(rst));
            return USER_ERROR;
        }

        rst = user_metrics_op->user_metrics_remove(local_user);
        if (rst)
        {
            logger->warn("Error when removing user metrics: {}", get_rst_txt(rst));
            return USER_ERROR;
        }

        rst = dbuser.remove_user(local_user->get_name());
        if (rst)
        {
            logger->warn("Error when deleting a user: {}", get_rst_txt(rst));
            return USER_ERROR;
        }

        std::const_pointer_cast<User>(local_user)->set_is_authenticated(false);
        local_user.reset(); // Remove object
        local_user = nullptr;

        return RST_OK;
    }

    return USER_NO_AUTH;
}
