#ifndef __SUBJECT_HPP
#define __SUBJECT_HPP

#include <string>

#include "primitives/definitions.hpp"
#include "primitives/category.hpp"
#include <memory>

class Subject
{
public:
    Subject(unsigned int new_id, std::string new_name);
    ~Subject(void);

    unsigned int get_id(void) const;
    void set_id(unsigned int new_id);

    unsigned int get_user_id(void) const;
    void set_user_id(unsigned int new_user_id);

    unsigned int get_category_id(void) const;
    void set_category_id(unsigned int new_category_id);

    std::string get_name(void) const;
    void set_name(std::string new_name);

    std::string get_filepath(void) const;
    void set_filepath(std::string new_filepath);

    std::string get_language(void) const;
    void set_language(std::string new_language);

private:
    unsigned int id;
    unsigned int user_id;
    unsigned int category_id;
    std::string name;
    std::string filepath;
    std::string language;
};

#endif //__SUBJECT_HPP
