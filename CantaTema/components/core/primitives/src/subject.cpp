#include "primitives/subject.hpp"

Subject::Subject(unsigned int new_id, std::string new_name) : id(new_id),
                                                            user_id(0),
                                                            category_id(0),
                                                            name(new_name),
                                                            filepath(""),
                                                            language("es")
{
}

Subject::~Subject(void) {}

unsigned int Subject::get_id(void) const { return id; }
void Subject::set_id(unsigned int new_id) { id = new_id; }

unsigned int Subject::get_user_id(void) const { return user_id; }
void Subject::set_user_id(unsigned int new_user_id) { user_id = new_user_id; }

unsigned int Subject::get_category_id(void) const { return category_id; }
void Subject::set_category_id(unsigned int new_category_id) { category_id = new_category_id; }

std::string Subject::get_name(void) const { return name; }
void Subject::set_name(std::string new_name) { name = new_name; }

std::string Subject::get_filepath(void) const { return filepath; }
void Subject::set_filepath(std::string new_filepath) { filepath = new_filepath; }

std::string Subject::get_language(void) const { return language; }
void Subject::set_language(std::string new_language) { language = new_language; }
