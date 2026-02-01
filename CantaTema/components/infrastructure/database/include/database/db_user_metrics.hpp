#ifndef __DB_USER_METRICS_HPP
#define __DB_USER_METRICS_HPP

#include <memory>
#include "primitives/definitions.hpp"
#include "primitives/utils_logger.hpp"
#include "primitives/user_metrics.hpp"

class DB_UserMetrics
{
public:
    /**
     * @brief Construct a new DB_UserMetrics object
     *
     */
    DB_UserMetrics(void);

    /**
     * @brief Create User Metrics table on database
     *
     * @return rst_code_e
     */
    rst_code_e user_metrics_tables_create(void) const;

    /**
     * @brief Update or Insert user metrics into the DB.
     *
     * @param metrics
     * @return rst_code_e
     */
    rst_code_e update_user_metrics(const UserMetrics &metrics) const;

    /**
     * @brief Remove metrics for a specific user
     *
     * @param user_id
     * @return rst_code_e
     */
    rst_code_e remove_user_metrics(unsigned int user_id) const;

    /**
     * @brief Get the user metrics object
     *
     * @param metrics Object containing the user_id to search for, and where data will be filled
     * @return rst_code_e
     */
    rst_code_e get_user_metrics(std::shared_ptr<UserMetrics> metrics) const;
};

#endif //__DB_USER_METRICS_HPP