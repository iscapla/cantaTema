#ifndef __MOCK_OPERATION_PRACTICE_EVENT_HPP
#define __MOCK_OPERATION_PRACTICE_EVENT_HPP

#include <gmock/gmock.h>
#include "operations/i_operation_practice_event.hpp"

class MockOperationPracticeEvent : public IOperationPracticeEvent {
public:
    MOCK_METHOD(rst_code_e, practice_event_add_planned, (const std::shared_ptr<const User> &user, PracticeEvent &practice), (override));
    MOCK_METHOD(rst_code_e, practice_event_add_recorded, (const std::shared_ptr<const User> &user, const std::string source_file, PracticeEvent &practice), (override));
    MOCK_METHOD(rst_code_e, practice_event_update, (const std::shared_ptr<const User> &user, const PracticeEvent &subject), (override));
    MOCK_METHOD(rst_code_e, practice_event_remove, (const std::shared_ptr<const User> &user, unsigned int id), (override));
    MOCK_METHOD(rst_code_e, practice_event_remove_by_subject_id, (const std::shared_ptr<const User> &user, unsigned int id), (override));
    MOCK_METHOD(rst_code_e, practice_event_remove_by_user_id, (const std::shared_ptr<const User> &user), (override));
    MOCK_METHOD(rst_code_e, practice_event_get_by_id, (const std::shared_ptr<const User> &user, unsigned int id, std::shared_ptr<PracticeEvent> &practice), (override));
    MOCK_METHOD(rst_code_e, practice_event_get_all_by_subject, (const std::shared_ptr<const User> &user, unsigned int subject_id, std::vector<std::shared_ptr<PracticeEvent>> &practices), (override));
    MOCK_METHOD(rst_code_e, practice_event_get_all_by_user, (const std::shared_ptr<const User> &user, std::vector<std::shared_ptr<PracticeEvent>> &practices), (override));
};

#endif // __MOCK_OPERATION_PRACTICE_EVENT_HPP
