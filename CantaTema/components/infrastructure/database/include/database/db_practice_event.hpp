#ifndef __DB_PRACTICE_EVENT_HPP
#define __DB_PRACTICE_EVENT_HPP

#include <vector>
#include <memory>

#include "primitives/definitions.hpp"
#include "primitives/utils_logger.hpp"
#include "primitives/practice_event.hpp"

class DB_PracticeEvent
{
public:
    /**
     * @brief Construct a new DB_PracticeEvent object
     *
     */
    DB_PracticeEvent(void);

    /**
     * @brief Create PracticeEvent table on database
     *
     * @return rst_code_e
     */
    rst_code_e practice_event_tables_create(void) const;

    /**
     * @brief Add new practice event to the DB. Sets the object with its new identifier.
     *
     * @param event
     * @return rst_code_e
     */
    rst_code_e add_new_practice_event(PracticeEvent &event) const;

    /**
     * @brief Update existing practice event in the DB.
     *
     * @param event
     * @return rst_code_e
     */
    rst_code_e update_practice_event(const PracticeEvent &event) const;

    /**
     * @brief Remove practice event by ID.
     *
     * @param id
     * @return rst_code_e
     */
    rst_code_e remove_practice_event(unsigned int id) const;

    /**
     * @brief Remove all practice events associated with a specific user.
     *
     * @param user_id
     * @return rst_code_e
     */
    rst_code_e remove_all_practice_events_by_user(unsigned int user_id) const;

    /**
     * @brief Remove all practice events associated with a specific subject.
     *
     * @param subject_id
     * @return rst_code_e
     */
    rst_code_e remove_all_practice_events_by_subject(unsigned int subject_id) const;

    /**
     * @brief Retrieve a practice event by its ID.
     *
     * @param id
     * @param event Output shared pointer to the practice event
     * @return rst_code_e
     */
    rst_code_e get_practice_event_by_id(unsigned int id, std::shared_ptr<PracticeEvent> &event) const;

    /**
     * @brief Retrieve all practice events associated with a specific user.
     *
     * @param user_id
     * @param events Output vector of shared pointers to practice events
     * @return rst_code_e
     */
    rst_code_e get_all_practice_events_by_user(unsigned int user_id, std::vector<std::shared_ptr<PracticeEvent>> &events) const;

    /**
     * @brief Retrieve all practice events associated with a specific subject.
     *
     * @param subject_id
     * @param events Output vector of shared pointers to practice events
     * @return rst_code_e
     */
    rst_code_e get_all_practice_events_by_subject(unsigned int subject_id, std::vector<std::shared_ptr<PracticeEvent>> &events) const;
};

#endif //__DB_PRACTICE_EVENT_HPP