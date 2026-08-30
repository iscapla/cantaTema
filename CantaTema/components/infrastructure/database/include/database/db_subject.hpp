#ifndef __DB_SUBJECT_HPP
#define __DB_SUBJECT_HPP

#include <vector>
#include <memory>

#include "primitives/definitions.hpp"
#include "primitives/utils_logger.hpp"
#include "primitives/subject.hpp"

class DB_Subject
{
public:
    /**
     * @brief Construct a new DB_Subject object
     *
     */
    DB_Subject(void);

    /**
     * @brief Create Subject table on database
     *
     * @return rst_code_e
     */
    rst_code_e subject_tables_create(void) const;

    /**
     * @brief Add new subject to the DB. Sets the subject object with its new identifier.
     *
     * @param subject
     * @return rst_code_e
     */
    rst_code_e add_new_subject(Subject &subject) const;

    /**
     * @brief Update existing subject in the DB.
     *
     * @param subject
     * @return rst_code_e
     */
    rst_code_e update_subject(const Subject &subject) const;

    /**
     * @brief Remove subject by ID.
     *
     * @param id
     * @return rst_code_e
     */
    rst_code_e remove_subject(unsigned int id) const;

    /**
     * @brief Remove all subjects associated with a specific user.
     *
     * @param user_id
     * @return rst_code_e
     */
    rst_code_e remove_all_subjects_from_user(unsigned int user_id) const;

    

    /**
     * @brief Retrieve a subject by its ID.
     *
     * @param id
     * @param subject Output shared pointer to the subject
     * @return rst_code_e
     */
    rst_code_e get_subject_by_id(unsigned int id, std::shared_ptr<Subject> &subject) const;

    /**
     * @brief Retrieve all subjects associated with a specific category.
     *
     * @param category_id
     * @param subjects Output vector of shared pointers to subjects
     * @return rst_code_e
     */
    rst_code_e get_all_subjects_by_category(unsigned int category_id, std::vector<std::shared_ptr<Subject>> &subjects) const;

    /**
     * @brief Retrieve all subjects associated with a specific user.
     *
     * @param user_id
     * @param subjects Output vector of shared pointers to subjects
     * @return rst_code_e
     */
    rst_code_e get_all_subjects_by_user(unsigned int user_id, std::vector<std::shared_ptr<Subject>> &subjects) const;

private:
    void load_tags_for_subject(struct sqlite3 *db, Subject &subject) const;
};

#endif //__DB_SUBJECT_HPP