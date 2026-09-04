
/**
 * @file operation_user.hpp
 * @brief Concrete implementation of user account authentication and business operations.
 */

#ifndef __OPERATION_USER_LOGIC_HPP
#define __OPERATION_USER_LOGIC_HPP

#include "operations/i_operation_user.hpp"
#include "operations/i_operation_user_metrics.hpp"

/**
 * @class OperationUser
 * @brief Manages user registration, credential hashing/authentication, and active session user context state.
 */
class OperationUser : public IOperationUser
{
public:
    /**
     * @brief Constructs an OperationUser with injected user metrics operation.
     * @param _user_metrics_op Shared pointer to user metrics operation handler.
     */
    OperationUser(std::shared_ptr<IOperationUserMetrics> &&_user_metrics_op);

    /**
     * @brief Destructor for OperationUser.
     */
    ~OperationUser();

    /**
     * @brief Adds a new user account to the system.
     * @param name Desired username.
     * @param password Password credential.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e user_add(const std::string &name, const std::string &password) override;

    /**
     * @brief Gets the currently authenticated user profile.
     * @param user Pointer receiving User instance.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e user_get(std::shared_ptr<const User> &user) override;

    /**
     * @brief Queries user account profile by username.
     * @param user_name Target username.
     * @param user Pointer receiving User instance.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e user_get_by_name(std::string user_name, std::shared_ptr<const User> &user) override;

    /**
     * @brief Updates active user settings in database.
     * @param user User object containing updated fields.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e user_update(std::shared_ptr<const User> &user) override;

    /**
     * @brief Removes the authenticated user account and associated metrics.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e user_remove(void) override;

    /**
     * @brief Checks if a user is currently authenticated in the local session.
     * @return true if authenticated, false otherwise.
     */
    bool user_is_authenticated(void) override;

    /**
     * @brief Authenticates credentials and sets current authenticated user state.
     * @param name Account username.
     * @param password Account password.
     * @return rst_code_e RST_OK on successful login, or error code.
     */
    rst_code_e user_identify(const std::string &name, const std::string &password) override;

    /**
     * @brief Persists UserConfiguration for a specific user ID.
     * @param user_id User identifier.
     * @param config Configuration parameters struct.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e save_user_configuration(unsigned int user_id, const UserConfiguration& config) override;

    /**
     * @brief Loads UserConfiguration for a specific user ID.
     * @param user_id User identifier.
     * @param out_config Output struct receiving configuration parameters.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e get_user_configuration(unsigned int user_id, UserConfiguration& out_config) override;

private:
    std::shared_ptr<IOperationUserMetrics> user_metrics_op{nullptr};
    std::shared_ptr<const User> local_user;
};

#endif //__OPERATION_USER_LOGIC_HPP