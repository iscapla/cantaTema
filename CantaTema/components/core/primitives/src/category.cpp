#include "primitives/category.hpp"

Category::Category(unsigned int new_id, std::string new_name) : id(new_id),
                                                                user_id(0),
                                                                name(new_name)
{
}

Category::~Category(void) {}

unsigned int Category::get_id(void) const { return id; }
void Category::set_id(unsigned int new_id) { id = new_id; }

unsigned int Category::get_user_id(void) const { return user_id; }
void Category::set_user_id(unsigned int new_user_id) { user_id = new_user_id; }

std::string Category::get_name(void) const { return name; }
void Category::set_name(std::string new_name) { name = new_name; }
