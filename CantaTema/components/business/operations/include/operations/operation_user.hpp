
#ifndef __OPERATION_USER_LOGIC_HPP
#define __OPERATION_USER_LOGIC_HPP

#include "operations/i_operation_user.hpp"

class OperationUser : public IOperationUser
{
public:
    // Default constructor
    OperationUser();
    ~OperationUser();

    rst_code_e user_add(const std::string &name, const std::string &password);

    rst_code_e user_get(std::shared_ptr<const User> &user);
    rst_code_e user_get_by_name(std::string user_name, std::shared_ptr<const User> &user);

    rst_code_e user_update(std::shared_ptr<const User> &user);

    rst_code_e user_remove(void);

    bool user_is_authenticated(void);

    rst_code_e user_identify(const std::string &name, const std::string &password);

private:
    std::shared_ptr<const User> local_user;
};

#endif //__OPERATION_USER_LOGIC_HPP