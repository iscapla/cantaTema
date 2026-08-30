/**
 * @file tag.hpp
 * @brief Domain model representing a user-scoped tag for categorizing and labeling subjects (Temas).
 */

#ifndef __TAG_HPP
#define __TAG_HPP

#include <string>
#include "primitives/definitions.hpp"

/**
 * @class Tag
 * @brief Primitive domain model representing a tag/label owned by a specific user.
 */
class Tag
{
public:
    /**
     * @brief Constructs a new Tag object.
     * @param new_id Unique identifier of the tag.
     * @param new_name Name of the tag.
     */
    Tag(unsigned int new_id, std::string new_name);

    /**
     * @brief Destructs the Tag object.
     */
    ~Tag(void);

    /**
     * @brief Gets the unique identifier of the tag.
     * @return Unique tag ID.
     */
    unsigned int get_id(void) const;

    /**
     * @brief Sets the unique identifier of the tag.
     * @param new_id Unique tag ID.
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
     * @brief Gets the name of the tag.
     * @return Tag name string.
     */
    std::string get_name(void) const;

    /**
     * @brief Sets the name of the tag.
     * @param new_name Tag name string.
     */
    void set_name(std::string new_name);

private:
    unsigned int id;       ///< Unique identifier of the tag.
    unsigned int user_id;  ///< Identifier of the owning user.
    std::string name;      ///< Name of the tag.
};

#endif // __TAG_HPP
