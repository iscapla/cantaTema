#ifndef __USER_HPP
#define __USER_HPP

#include <string>

#include "primitives/definitions.hpp"

class User
{

public:
    enum Account_status
    {
        PENDING_ACTIVATION,
        ACTIVE,
        LOCKED,
        DEACTIVATED,
        UNKNOWN     //Error status
    };

    User(std::string new_name);
    ~User(void);

    bool get_is_authenticated(void) const;
    void set_is_authenticated(bool new_is_authenticated);

    unsigned int get_useraccountid(void) const;
    void set_useraccountid(unsigned int new_useraccountid);

    std::string get_name(void) const;
    void set_name(std::string new_name);

    std::string get_passwordkey(void) const;
    void set_passwordkey(std::string new_passwordkey);

    std::string get_passwordsalt(void) const;
    void set_passwordsalt(std::string new_passwordsalt);
    std::string get_resettoken(void) const;
    void set_resettoken(std::string new_resettoken);
    std::time_t get_resetexpiration(void) const;
    void set_resetexpiration(std::time_t new_resetexpiration);
    User::Account_status get_status(void) const;
    void set_status(User::Account_status new_status);
    std::time_t get_creationdate(void) const;
    void set_creationdate(std::time_t new_creationdate);
    std::string get_locknotes(void) const;
    void set_locknotes(std::string new_locknotes);
    std::string get_workemail(void) const;
    void set_workemail(std::string new_workemail);
    std::string get_recoveryemail(void) const;
    void set_recoveryemail(std::string new_recoveryemail);
    std::string get_firstname(void) const;
    void set_firstname(std::string new_firstname);
    std::string get_lastname(void) const;
    void set_lastname(std::string new_lastname);
    unsigned int get_roleid(void) const;
    void set_roleid(unsigned int new_roleid);
    unsigned int get_max_space_size_in_kb(void) const;
    void set_max_space_size_in_kb(unsigned int new_max_space_size_in_kb);

    User::Account_status parse_status_to_type(std::string &status) const;
    std::string parse_status_to_string(User::Account_status status) const;

    void print(void) const;

private:
    bool is_authenticated;

    unsigned int useraccountid;
    std::string name;
    std::string passwordkey;
    std::string passwordsalt;
    std::string resettoken;
    std::time_t resetexpiration;
    User::Account_status status;
    std::time_t creationdate;
    std::string locknotes;
    std::string workemail;
    std::string recoveryemail;
    std::string firstname;
    std::string lastname;
    unsigned int roleid;
    unsigned int max_space_size_in_kb;
};

#endif //__USER_HPP