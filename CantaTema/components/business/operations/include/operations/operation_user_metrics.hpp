#ifndef __IOPERATION_USER_METRICS_LOGIC_HPP
#define __IOPERATION_USER_METRICS_LOGIC_HPP

#include "operations/i_operation_user_metrics.hpp"


class OperationUserMetrics : public IOperationUserMetrics
{
public:
    // Default constructor
    OperationUserMetrics();
    ~OperationUserMetrics();

    rst_code_e user_metrics_add(const std::shared_ptr<const User> &user, UserMetrics &metrics) override;

    rst_code_e user_metrics_update(const std::shared_ptr<const User> &user, UserMetrics &metrics) override;

    rst_code_e user_metrics_remove(const std::shared_ptr<const User> &user) override;

    rst_code_e user_metrics_get(const std::shared_ptr<const User> &user, std::shared_ptr<UserMetrics> &metrics) override;

    rst_code_e user_metrics_can_accept_file_size(const std::shared_ptr<const User> &user, unsigned int size_in_kb) override;

};
#endif //__IOPERATION_USER_METRICS_LOGIC_HPP
