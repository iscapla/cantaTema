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

const std::vector<Tag>& Subject::get_tags(void) const { return tags; }
void Subject::set_tags(const std::vector<Tag> &new_tags) { tags = new_tags; }

void Subject::add_tag(const Tag &tag)
{
    for (const auto &existing : tags)
    {
        if (existing.get_id() == tag.get_id())
        {
            return;
        }
    }
    tags.push_back(tag);
}

void Subject::remove_tag(unsigned int tag_id)
{
    std::erase_if(tags, [tag_id](const Tag &t) {
        return t.get_id() == tag_id;
    });
}

bool Subject::has_tag(unsigned int tag_id) const
{
    for (const auto &t : tags)
    {
        if (t.get_id() == tag_id)
        {
            return true;
        }
    }
    return false;
}
