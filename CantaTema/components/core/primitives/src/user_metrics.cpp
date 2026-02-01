#include "primitives/user_metrics.hpp"

UserMetrics::UserMetrics(unsigned int useraccountid) : useraccountid(useraccountid),
                                                       space_used_kb(0)
{
}

UserMetrics::~UserMetrics(void){}

unsigned int UserMetrics::get_useraccountid(void) const { return useraccountid; }
void UserMetrics::set_useraccountid(unsigned int new_useraccountid) { useraccountid = new_useraccountid; }

unsigned int UserMetrics::get_space_used_kb(void) const { return space_used_kb; }
void UserMetrics::set_space_used_kb(unsigned int new_space_used_kb) { space_used_kb = new_space_used_kb; }

void UserMetrics::print(void) const
{
    logger->debug("| {:>6d} | {:>10d} |", useraccountid, space_used_kb);
}
