/**
 * @file tag.cpp
 * @brief Implementation of the Tag domain primitive model.
 */

#include "primitives/tag.hpp"

Tag::Tag(unsigned int new_id, std::string new_name) : id(new_id),
                                                      user_id(0),
                                                      name(new_name)
{
}

Tag::~Tag(void) {}

unsigned int Tag::get_id(void) const { return id; }
void Tag::set_id(unsigned int new_id) { id = new_id; }

unsigned int Tag::get_user_id(void) const { return user_id; }
void Tag::set_user_id(unsigned int new_user_id) { user_id = new_user_id; }

std::string Tag::get_name(void) const { return name; }
void Tag::set_name(std::string new_name) { name = new_name; }
