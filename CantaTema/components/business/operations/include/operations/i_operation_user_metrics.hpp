

#ifndef __IOPERATION_USER_METRICS_HPP
#define __IOPERATION_USER_METRICS_HPP

#include <string>
#include <vector>
#include <memory>

#include "primitives/definitions.hpp"
#include "primitives/user.hpp"
#include "primitives/user_metrics.hpp"

class IOperationUserMetrics
{

public:
    virtual ~IOperationUserMetrics() = default;

    virtual rst_code_e user_metrics_add(const std::shared_ptr<const User> &user, UserMetrics &metrics) = 0;

    virtual rst_code_e user_metrics_update(const std::shared_ptr<const User> &user, UserMetrics &metrics) = 0;

    virtual rst_code_e user_metrics_remove(const std::shared_ptr<const User> &user) = 0;

    virtual rst_code_e user_metrics_get(const std::shared_ptr<const User> &user, std::shared_ptr<UserMetrics> &metrics) = 0;

    virtual rst_code_e user_metrics_can_accept_file_size(const std::shared_ptr<const User> &user, unsigned int size_in_kb) = 0;

};

#endif //__IOPERATION_USER_METRICS_HPP