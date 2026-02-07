#ifndef __OPERATION_SUBJECT_LOGIC_HPP
#define __OPERATION_SUBJECT_LOGIC_HPP

#include "operations/i_operation_subject.hpp"
#include "operations/i_operation_user_metrics.hpp"

class OperationSubject : public IOperationSubject
{
public:
    OperationSubject(std::shared_ptr<IOperationUserMetrics> &&_user_metrics_op);
    ~OperationSubject();

    rst_code_e subject_add(const std::shared_ptr<const User> &user, const std::string source_file, Subject &subject) override;

    rst_code_e subject_update(const std::shared_ptr<const User> &user, const Subject &subject) override;

    rst_code_e subject_remove(const std::shared_ptr<const User> &user, unsigned int id) override;

    rst_code_e subject_get_by_id(unsigned int id, std::shared_ptr<Subject> &subject) override;

    rst_code_e subject_get_all_by_category(unsigned int category_id, std::vector<std::shared_ptr<Subject>> &subjects) override;

    rst_code_e subject_get_all_by_user(const std::shared_ptr<const User> &user, std::vector<std::shared_ptr<Subject>> &subjects) override;

private:
    std::shared_ptr<IOperationUserMetrics> user_metrics_op{nullptr};

    //TODO Add here the category_op variable to check that the subjects always contigure a valid category id

};

#endif //__OPERATION_SUBJECT_LOGIC_HPP