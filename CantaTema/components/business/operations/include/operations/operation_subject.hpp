#ifndef __OPERATION_SUBJECT_LOGIC_HPP
#define __OPERATION_SUBJECT_LOGIC_HPP

#include "operations/i_operation_subject.hpp"

class OperationSubject : public IOperationSubject
{
public:
    OperationSubject();
    ~OperationSubject();

    rst_code_e subject_add(const std::string source_file, Subject &subject) override;

    rst_code_e subject_update(const Subject &subject) override;

    rst_code_e subject_remove(unsigned int id) override;

    rst_code_e subject_get_by_id(unsigned int id, std::shared_ptr<Subject> &subject) override;

    rst_code_e subject_get_all_by_category(unsigned int category_id, std::vector<std::shared_ptr<Subject>> &subjects) override;

    rst_code_e subject_get_all_by_user(unsigned int user_id, std::vector<std::shared_ptr<Subject>> &subjects) override;
};

#endif //__OPERATION_SUBJECT_LOGIC_HPP