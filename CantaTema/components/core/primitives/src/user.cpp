

#include "primitives/utils_functions.hpp"
#include "primitives/user.hpp"

User::User(std::string new_name) : name(new_name),
                                             is_authenticated(false),
                                             useraccountid(0),
                                             passwordkey(""),
                                             passwordsalt(""),
                                             resettoken(""),
                                             resetexpiration(0),
                                             status(User::Account_status::UNKNOWN),
                                             creationdate(0),
                                             locknotes(""),
                                             workemail(""),
                                             recoveryemail(""),
                                             firstname(""),
                                             lastname(""),
                                             roleid(0),
                                             max_space_size_in_kb(0)
{
}
User::~User(void) {}

bool User::get_is_authenticated(void) const { return is_authenticated; }
void User::set_is_authenticated(bool new_is_authenticated) { is_authenticated = new_is_authenticated; }

unsigned int User::get_useraccountid(void) const { return useraccountid; }
void User::set_useraccountid(unsigned int new_useraccountid) { useraccountid = new_useraccountid; }

std::string User::get_name(void) const { return name; }
void User::set_name(std::string new_name) { name = new_name; }

std::string User::get_passwordkey(void) const { return passwordkey; }
void User::set_passwordkey(std::string new_passwordkey) { passwordkey = new_passwordkey; }

std::string User::get_passwordsalt(void) const { return passwordsalt; }
void User::set_passwordsalt(std::string new_passwordsalt) { passwordsalt = new_passwordsalt; }

std::string User::get_resettoken(void) const { return resettoken; }
void User::set_resettoken(std::string new_resettoken) { resettoken = new_resettoken; }

std::time_t User::get_resetexpiration(void) const { return resetexpiration; }
void User::set_resetexpiration(std::time_t new_resetexpiration) { resetexpiration = new_resetexpiration; }

User::Account_status User::get_status(void) const { return status; }
void User::set_status(User::Account_status new_status) { status = new_status; }

std::time_t User::get_creationdate(void) const { return creationdate; }
void User::set_creationdate(std::time_t new_creationdate) { creationdate = new_creationdate; }

std::string User::get_locknotes(void) const { return locknotes; }
void User::set_locknotes(std::string new_locknotes) { locknotes = new_locknotes; }

std::string User::get_workemail(void) const { return workemail; }
void User::set_workemail(std::string new_workemail) { workemail = new_workemail; }

std::string User::get_recoveryemail(void) const { return recoveryemail; }
void User::set_recoveryemail(std::string new_recoveryemail) { recoveryemail = new_recoveryemail; }

std::string User::get_firstname(void) const { return firstname; }
void User::set_firstname(std::string new_firstname) { firstname = new_firstname; }

std::string User::get_lastname(void) const { return lastname; }
void User::set_lastname(std::string new_lastname) { lastname = new_lastname; }

unsigned int User::get_roleid(void) const { return roleid; }
void User::set_roleid(unsigned int new_roleid) { roleid = new_roleid; }

unsigned int User::get_max_space_size_in_kb(void) const { return max_space_size_in_kb; }
void User::set_max_space_size_in_kb(unsigned int new_max_space_size_in_kb) { max_space_size_in_kb = new_max_space_size_in_kb; }

User::Account_status User::parse_status_to_type(std::string &status) const
{

    if (status == "PENDING_ACTIVATION")
    {
        return User::Account_status::PENDING_ACTIVATION;
    }
    else if (status == "ACTIVE")
    {
        return User::Account_status::ACTIVE;
    }
    else if (status == "LOCKED")
    {
        return User::Account_status::LOCKED;
    }
    else if (status == "DEACTIVATED")
    {
        return User::Account_status::DEACTIVATED;
    }

    return User::Account_status::UNKNOWN;
}

std::string User::parse_status_to_string(User::Account_status status) const
{

    switch (status)
    {
    case User::Account_status::PENDING_ACTIVATION:
        return std::string{"PENDING_ACTIVATION"};
        break;
    case User::Account_status::ACTIVE:
        return std::string{"ACTIVE"};
        break;
    case User::Account_status::LOCKED:
        return std::string{"LOCKED"};
        break;
    case User::Account_status::DEACTIVATED:
        return std::string{"DEACTIVATED"};
        break;
    default:
        return std::string{"UNKNOWN"};
        break;
    }
}
