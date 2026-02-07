#ifndef __IOPERATION_PRACTICE_EVENT_HPP
#define __IOPERATION_PRACTICE_EVENT_HPP

#include <vector>
#include <memory>
#include <string>

#include "primitives/definitions.hpp"
#include "primitives/user.hpp"
#include "primitives/practice_event.hpp"


class IOperationPracticeEvent
{
public:
    virtual ~IOperationPracticeEvent() = default;

    virtual rst_code_e practice_event_add_planned(const std::shared_ptr<const User> &user, PracticeEvent &practice) = 0;
    virtual rst_code_e practice_event_add_recorded(const std::shared_ptr<const User> &user, const std::string source_file, PracticeEvent &practice) = 0;

    virtual rst_code_e practice_event_update(const std::shared_ptr<const User> &user, const PracticeEvent &subject) = 0;

    virtual rst_code_e practice_event_remove(const std::shared_ptr<const User> &user, unsigned int id) = 0;

    virtual rst_code_e practice_event_remove_by_subject_id(const std::shared_ptr<const User> &user, unsigned int id) = 0;

    virtual rst_code_e practice_event_remove_by_user_id(const std::shared_ptr<const User> &user) = 0;

    virtual rst_code_e practice_event_get_by_id(const std::shared_ptr<const User> &user, unsigned int id, std::shared_ptr<PracticeEvent> &practice) = 0;

    virtual rst_code_e practice_event_get_all_by_subject(const std::shared_ptr<const User> &user, unsigned int subject_id, std::vector<std::shared_ptr<PracticeEvent>> &practices) = 0;

    virtual rst_code_e practice_event_get_all_by_user(const std::shared_ptr<const User> &user, std::vector<std::shared_ptr<PracticeEvent>> &practices) = 0;
};

#endif //__IOPERATION_PRACTICE_EVENT_HPP