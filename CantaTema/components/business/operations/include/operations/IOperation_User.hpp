

#ifndef __IOPERATION_USER_HPP
#define __IOPERATION_USER_HPP

#include <string>
#include <vector>
#include <memory>

#include "primitives/definitions.hpp"
#include "primitives/user.hpp"

class IOperation_User
{

public:
    virtual rst_code_e user_add(const std::string &name, const std::string &password) = 0;

    virtual rst_code_e user_get(std::shared_ptr<const User> &user) = 0;
    virtual rst_code_e user_get_by_name(std::string user_name, std::shared_ptr<const User> &user) = 0;

    virtual rst_code_e user_update(std::shared_ptr<const User> &user) = 0;

    virtual rst_code_e user_remove(void) = 0;

    virtual bool user_is_authenticated(void) = 0;

    virtual rst_code_e user_identify(const std::string &name, const std::string &password) = 0;
};

#endif //__IOPERATION_USER_HPP