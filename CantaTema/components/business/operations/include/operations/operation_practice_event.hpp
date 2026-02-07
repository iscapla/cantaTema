#ifndef __OPERATION_PRACTICE_EVENT_LOGIC_HPP
#define __OPERATION_PRACTICE_EVENT_LOGIC_HPP

#include "operations/i_operation_practice_event.hpp"
#include "operations/i_operation_user_metrics.hpp"
#include "operations/i_operation_subject.hpp"

class OperationPracticeEvent : public IOperationPracticeEvent
{
public:
    OperationPracticeEvent(std::shared_ptr<IOperationUserMetrics> &&_user_metrics_op, std::shared_ptr<IOperationSubject> &&_subject_op);
    ~OperationPracticeEvent();

    rst_code_e practice_event_add_planned(const std::shared_ptr<const User> &user, PracticeEvent &practice) override;
    rst_code_e practice_event_add_recorded(const std::shared_ptr<const User> &user, const std::string source_file, PracticeEvent &practice) override;

    rst_code_e practice_event_update(const std::shared_ptr<const User> &user, const PracticeEvent &practice) override;

    rst_code_e practice_event_remove(const std::shared_ptr<const User> &user, unsigned int id) override;

    rst_code_e practice_event_remove_by_subject_id(const std::shared_ptr<const User> &user, unsigned int id) override;

    rst_code_e practice_event_remove_by_user_id(const std::shared_ptr<const User> &user) override;

    rst_code_e practice_event_get_by_id(const std::shared_ptr<const User> &user, unsigned int id, std::shared_ptr<PracticeEvent> &practice) override;

    rst_code_e practice_event_get_all_by_subject(const std::shared_ptr<const User> &user, unsigned int subject_id, std::vector<std::shared_ptr<PracticeEvent>> &practices) override;

    rst_code_e practice_event_get_all_by_user(const std::shared_ptr<const User> &user, std::vector<std::shared_ptr<PracticeEvent>> &practices) override;

private:
    std::shared_ptr<IOperationUserMetrics> user_metrics_op{nullptr};
    std::shared_ptr<IOperationSubject> subject_op{nullptr};

    rst_code_e check_updates(const PracticeEvent *from, const PracticeEvent *to) const;
    rst_code_e check_class_consistency(const PracticeEvent *event) const;

};

#endif //__OPERATION_PRACTICE_EVENT_LOGIC_HPP