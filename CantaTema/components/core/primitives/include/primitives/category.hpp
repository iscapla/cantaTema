#ifndef __CATEGORY_HPP
#define __CATEGORY_HPP

#include <string>

#include "primitives/definitions.hpp"

class Category
{
public:
    Category(unsigned int new_id, std::string new_name);
    ~Category(void);

    unsigned int get_id(void) const;
    void set_id(unsigned int new_id);

    unsigned int get_user_id(void) const;
    void set_user_id(unsigned int new_user_id);

    std::string get_name(void) const;
    void set_name(std::string new_name);

    void print(void) const;

private:
    unsigned int id;
    unsigned int user_id;
    std::string name;
};

#endif //__CATEGORY_HPP
