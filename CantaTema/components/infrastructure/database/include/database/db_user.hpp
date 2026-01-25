#ifndef __DB_USER_HPP
#define __DB_USER_HPP

#include "primitives/definitions.hpp"
#include "primitives/utils_logger.hpp"

#include "primitives/user.hpp"

class DB_User
{

public:
    /**
     * @brief Construct a new db user object
     *
     */
    DB_User(void);

    /**
     * @brief Create User table on database
     */
    rst_code_e user_tables_create(void) const;

    /**
     * @brief Check if a user already exists
     *
     * @param name
     * @return rst_code_e
     */
    rst_code_e is_user_already_present(const std::string &name, bool &already_exists);

    /**
     * @brief Add new user to the DB with given information. Set the user object with its identifier
     *
     * @param user
     * @return rst_code_e
     */
    rst_code_e add_new_user(User &user) const;

    /**
     * @brief Update user to the DB with given information. Set the user object with its identifier
     *
     * @param user
     * @return rst_code_e
     */
    rst_code_e update_user(User &user) const;

    /**
     * @brief Remove user who match the name
     *
     * @param user_name user name to be removed
     * @return rst_code_e
     */
    rst_code_e remove_user(const std::string &user_name) const;

    /**
     * @brief Return user who match the name
     *
     * @param user
     * @return rst_code_e
     */
    rst_code_e get_user(User &user) const;

    /**
     * @brief Return user who match the name
     *
     * @param user_id
     * @param user
     * @return rst_code_e
     */
    rst_code_e get_user_by_id(unsigned int user_id, User &user) const;
};

#endif //__DB_USER_HPP