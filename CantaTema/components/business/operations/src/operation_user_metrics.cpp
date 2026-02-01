#include "operations/operation_user_metrics.hpp"

#include "database/db_main.hpp"
#include "database/db_user_metrics.hpp"


OperationUserMetrics::OperationUserMetrics()
{
    DB_Main *db_main = DB_Main::getInstance();
}

OperationUserMetrics::~OperationUserMetrics()
{
}

rst_code_e OperationUserMetrics::user_metrics_add(const std::shared_ptr<const User> &user, UserMetrics &metrics)
{
    DB_UserMetrics db_user_metrics;
    rst_code_e rst;

    if(user == nullptr || user->get_useraccountid() == 0){
        logger->error("User info error");
        return USER_METRICS_ERROR;
    }

    metrics.set_useraccountid(user->get_useraccountid());

    rst = db_user_metrics.update_user_metrics(metrics);

    if (rst)
    {
        logger->warn("Error when adding user metrics: {}", get_rst_txt(rst));
        return USER_METRICS_ERROR;
    }

    return RST_OK;

}

rst_code_e OperationUserMetrics::user_metrics_update(const std::shared_ptr<const User> &user, UserMetrics &metrics)
{
    DB_UserMetrics db_user_metrics;
    rst_code_e rst;

    if(user == nullptr || user->get_useraccountid() == 0){
        logger->error("User info error");
        return USER_METRICS_ERROR;
    }

    metrics.set_useraccountid(user->get_useraccountid());

    rst = db_user_metrics.update_user_metrics(metrics);

    if (rst)
    {
        logger->warn("Error when adding user metrics: {}", get_rst_txt(rst));
        return USER_METRICS_ERROR;
    }

    return RST_OK;

}

rst_code_e OperationUserMetrics::user_metrics_remove(const std::shared_ptr<const User> &user)
{
    DB_UserMetrics db_user_metrics;
    rst_code_e rst;

    if(user == nullptr || user->get_useraccountid() == 0){
        logger->error("User info error");
        return USER_METRICS_ERROR;
    }

    rst = db_user_metrics.remove_user_metrics(user->get_useraccountid());

    if (rst)
    {
        logger->warn("Error when removing user metrics: {}", get_rst_txt(rst));
        return USER_METRICS_ERROR;
    }

    return RST_OK;
}

rst_code_e OperationUserMetrics::user_metrics_get(const std::shared_ptr<const User> &user, std::shared_ptr<UserMetrics> &metrics)
{
    DB_UserMetrics db_user_metrics;

    if(user == nullptr || user->get_useraccountid() == 0){
        logger->error("User info error");
        return USER_METRICS_ERROR;
    }

    auto temp_metrics = std::make_shared<UserMetrics>(user->get_useraccountid());
    rst_code_e rst = db_user_metrics.get_user_metrics(temp_metrics);
    if (rst)
    {
        logger->warn("Get metrics by user error ({})", get_rst_txt(rst));
        return USER_METRICS_ERROR;
    }

    metrics = temp_metrics;

    return RST_OK;
}

rst_code_e OperationUserMetrics::user_metrics_can_accept_file_size(const std::shared_ptr<const User> &user, unsigned int size_in_kb)
{
    std::shared_ptr<UserMetrics> metrics = nullptr;
    rst_code_e rst = user_metrics_get(user, metrics);
    if (rst)
    {
        return rst;
    }

    unsigned int new_size = metrics->get_space_used_kb() + size_in_kb;
    if(new_size > user->get_max_space_size_in_kb())
    {
        return USER_METRICS_NOT_ENOUGH_SPACE;
    }

    return RST_OK;
}
