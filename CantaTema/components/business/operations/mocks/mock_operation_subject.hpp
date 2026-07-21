#ifndef __MOCK_OPERATION_SUBJECT_HPP
#define __MOCK_OPERATION_SUBJECT_HPP

#include <gmock/gmock.h>
#include "operations/i_operation_subject.hpp"

class MockOperationSubject : public IOperationSubject {
public:
    MOCK_METHOD(rst_code_e, subject_add, (const std::shared_ptr<const User> &user, const std::string source_file, Subject &subject), (override));
    MOCK_METHOD(rst_code_e, subject_update, (const std::shared_ptr<const User> &user, const Subject &subject), (override));
    MOCK_METHOD(rst_code_e, subject_remove, (const std::shared_ptr<const User> &user, unsigned int id), (override));
    MOCK_METHOD(rst_code_e, subject_get_by_id, (const std::shared_ptr<const User> &user, unsigned int id, std::shared_ptr<Subject> &subject), (override));
    MOCK_METHOD(rst_code_e, subject_get_all_by_category, (const std::shared_ptr<const User> &user, unsigned int category_id, std::vector<std::shared_ptr<Subject>> &subjects), (override));
    MOCK_METHOD(rst_code_e, subject_get_all_by_user, (const std::shared_ptr<const User> &user, std::vector<std::shared_ptr<Subject>> &subjects), (override));
};

#endif // __MOCK_OPERATION_SUBJECT_HPP
