#ifndef __SUBJECT_HPP
#define __SUBJECT_HPP

#include <string>

#include "primitives/definitions.hpp"
#include "primitives/category.hpp"
#include "primitives/tag.hpp"
#include <memory>
#include <vector>

/**
 * @class Subject
 * @brief Primitive domain model representing a study topic / Tema.
 */
class Subject
{
public:
    /**
     * @brief Constructs a new Subject.
     * @param new_id Unique identifier.
     * @param new_name Name of the subject.
     */
    Subject(unsigned int new_id, std::string new_name);

    /**
     * @brief Destructs the Subject.
     */
    ~Subject(void);

    /**
     * @brief Gets the unique identifier of the subject.
     * @return Subject ID.
     */
    unsigned int get_id(void) const;

    /**
     * @brief Sets the unique identifier of the subject.
     * @param new_id Subject ID.
     */
    void set_id(unsigned int new_id);

    /**
     * @brief Gets the owning user ID.
     * @return Owner user ID.
     */
    unsigned int get_user_id(void) const;

    /**
     * @brief Sets the owning user ID.
     * @param new_user_id Owner user ID.
     */
    void set_user_id(unsigned int new_user_id);

    /**
     * @brief Gets the associated category ID.
     * @return Category ID (0 if uncategorized).
     */
    unsigned int get_category_id(void) const;

    /**
     * @brief Sets the associated category ID.
     * @param new_category_id Category ID.
     */
    void set_category_id(unsigned int new_category_id);

    /**
     * @brief Gets the subject name.
     * @return Subject name string.
     */
    std::string get_name(void) const;

    /**
     * @brief Sets the subject name.
     * @param new_name Subject name string.
     */
    void set_name(std::string new_name);

    /**
     * @brief Gets the reference document filepath.
     * @return Filepath string.
     */
    std::string get_filepath(void) const;

    /**
     * @brief Sets the reference document filepath.
     * @param new_filepath Filepath string.
     */
    void set_filepath(std::string new_filepath);

    /**
     * @brief Gets the language ISO code.
     * @return Language string (e.g. 'es').
     */
    std::string get_language(void) const;

    /**
     * @brief Sets the language ISO code.
     * @param new_language Language string.
     */
    void set_language(std::string new_language);

    /**
     * @brief Gets the list of attached tags.
     * @return Const reference to tag vector.
     */
    const std::vector<Tag>& get_tags(void) const;

    /**
     * @brief Sets the list of attached tags.
     * @param new_tags Vector of tags.
     */
    void set_tags(const std::vector<Tag> &new_tags);

    /**
     * @brief Adds a tag to the subject if not already present.
     * @param tag Tag object to attach.
     */
    void add_tag(const Tag &tag);

    /**
     * @brief Removes a tag from the subject by tag ID.
     * @param tag_id Identifier of the tag to remove.
     */
    void remove_tag(unsigned int tag_id);

    /**
     * @brief Checks if a tag is attached to this subject.
     * @param tag_id Identifier of the tag to query.
     * @return true if attached, false otherwise.
     */
    bool has_tag(unsigned int tag_id) const;

private:
    unsigned int id;
    unsigned int user_id;
    unsigned int category_id;
    std::string name;
    std::string filepath;
    std::string language;
    std::vector<Tag> tags;
};

#endif //__SUBJECT_HPP
