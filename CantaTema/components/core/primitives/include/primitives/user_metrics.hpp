#ifndef __USER_METRICS_HPP
#define __USER_METRICS_HPP

#include <string>

#include "primitives/definitions.hpp"

class UserMetrics
{

public:
    UserMetrics(unsigned int useraccountid);
    ~UserMetrics(void);

    unsigned int get_useraccountid(void) const;
    void set_useraccountid(unsigned int new_useraccountid);

    unsigned int get_space_used_kb(void) const;
    void set_space_used_kb(unsigned int new_space_used_kb);

private:
    unsigned int useraccountid;
    unsigned int space_used_kb;
};

#endif //__USER_METRICS_HPP