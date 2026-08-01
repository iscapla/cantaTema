

/**
 * @file i_operation_user.hpp
 * @brief Abstract interface for user authentication and user profile management operations.
 */

#ifndef __IOPERATION_USER_HPP
#define __IOPERATION_USER_HPP

#include <string>
#include <vector>
#include <memory>

#include "primitives/definitions.hpp"
#include "primitives/user.hpp"

/**
 * @class IOperationUser
 * @brief Abstract interface defining user account creation, identification, authentication, and state queries.
 */
class IOperationUser
{
public:
    /**
     * @brief Virtual destructor for IOperationUser.
     */
    virtual ~IOperationUser() = default;

    /**
     * @brief Registers a new user account.
     * @param name Username string.
     * @param password Raw password string.
     * @return rst_code_e RST_OK on success, USER_DUPLICATED if user exists, or USER_ERROR.
     */
    virtual rst_code_e user_add(const std::string &name, const std::string &password) = 0;

    /**
     * @brief Retrieves the currently authenticated user profile.
     * @param user Output pointer receiving User object.
     * @return rst_code_e RST_OK on success, or USER_NOT_FOUND.
     */
    virtual rst_code_e user_get(std::shared_ptr<const User> &user) = 0;

    /**
     * @brief Retrieves a user profile by account username.
     * @param user_name Username to query.
     * @param user Output pointer receiving User object.
     * @return rst_code_e RST_OK on success, or USER_NOT_FOUND.
     */
    virtual rst_code_e user_get_by_name(std::string user_name, std::shared_ptr<const User> &user) = 0;

    /**
     * @brief Updates user account parameters.
     * @param user User object containing updated details.
     * @return rst_code_e RST_OK on success, or USER_ERROR.
     */
    virtual rst_code_e user_update(std::shared_ptr<const User> &user) = 0;

    /**
     * @brief Removes the currently authenticated user account.
     * @return rst_code_e RST_OK on success, or USER_ERROR.
     */
    virtual rst_code_e user_remove(void) = 0;

    /**
     * @brief Checks if a user is currently logged in / authenticated in this session.
     * @return true if user session is authenticated, false otherwise.
     */
    virtual bool user_is_authenticated(void) = 0;

    /**
     * @brief Authenticates user credentials and sets active user session context.
     * @param name Username credential.
     * @param password Password credential.
     * @return rst_code_e RST_OK on successful authentication, USER_NO_AUTH on bad credentials, or USER_NOT_FOUND.
     */
    virtual rst_code_e user_identify(const std::string &name, const std::string &password) = 0;
};

#endif //__IOPERATION_USER_HPP