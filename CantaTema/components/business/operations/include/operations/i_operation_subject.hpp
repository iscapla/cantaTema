#ifndef __IOPERATION_SUBJECT_HPP
#define __IOPERATION_SUBJECT_HPP

#include <vector>
#include <memory>
#include <string>

#include "primitives/definitions.hpp"
#include "primitives/user.hpp"
#include "primitives/subject.hpp"

class IOperationSubject
{
public:
    virtual ~IOperationSubject() = default;

    virtual rst_code_e subject_add(const std::shared_ptr<const User> &user, const std::string source_file, Subject &subject) = 0;

    virtual rst_code_e subject_update(const std::shared_ptr<const User> &user, const Subject &subject) = 0;

    virtual rst_code_e subject_remove(const std::shared_ptr<const User> &user, unsigned int id) = 0;

    virtual rst_code_e subject_get_by_id(unsigned int id, std::shared_ptr<Subject> &subject) = 0;

    virtual rst_code_e subject_get_all_by_category(unsigned int category_id, std::vector<std::shared_ptr<Subject>> &subjects) = 0;

    virtual rst_code_e subject_get_all_by_user(const std::shared_ptr<const User> &user, std::vector<std::shared_ptr<Subject>> &subjects) = 0;
};

#endif //__IOPERATION_SUBJECT_HPP