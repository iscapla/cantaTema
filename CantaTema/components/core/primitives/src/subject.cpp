#include "primitives/subject.hpp"

Subject::Subject(unsigned int new_id, std::string new_name) : id(new_id),
                                                            user_id(0),
                                                            name(new_name),
                                                            filepath(""),
                                                            category(nullptr)
{
}

Subject::~Subject(void) {}

unsigned int Subject::get_id(void) const { return id; }
void Subject::set_id(unsigned int new_id) { id = new_id; }

unsigned int Subject::get_user_id(void) const { return user_id; }
void Subject::set_user_id(unsigned int new_user_id) { user_id = new_user_id; }

std::string Subject::get_name(void) const { return name; }
void Subject::set_name(std::string new_name) { name = new_name; }

std::string Subject::get_filepath(void) const { return filepath; }
void Subject::set_filepath(std::string new_filepath) { filepath = new_filepath; }

std::shared_ptr<Category> Subject::get_category(void) const { return category; }
void Subject::set_category(std::shared_ptr<Category> new_category) { category = new_category; }

void Subject::print(void) const
{
    logger->debug("| {:>6d} | {:>6d} | {:>20s} | {:>30s} | {:>20s} |", id, user_id, name, filepath, (category ? category->get_name() : "No Category"));
}
