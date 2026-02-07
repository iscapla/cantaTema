#ifndef __PRACTICE_EVENT_HPP
#define __PRACTICE_EVENT_HPP

#include <string>

#include "primitives/definitions.hpp"
#include "primitives/category.hpp"
#include <memory>

class PracticeEvent
{
public:
    enum PracticeEvent_status
    {
        PLANNED,
        RECORDED,
        REMOVED,
        UNKNOWN
    };

    static const std::string get_status_as_string(PracticeEvent_status status);
    static const PracticeEvent_status parse_status_from_string(const std::string status);

    PracticeEvent(void);
    ~PracticeEvent(void);

    unsigned int get_id(void) const;
    void set_id(unsigned int new_id);

    unsigned int get_user_id(void) const;
    void set_user_id(unsigned int new_user_id);

    unsigned int get_subject_id(void) const;
    void set_subject_id(unsigned int new_subject_id);

    PracticeEvent_status get_status(void) const;
    void set_status(PracticeEvent_status new_status);

    unsigned int get_date(void) const;
    void set_date(unsigned int new_date);

    unsigned int get_recorded_date(void) const;
    void set_recorded_date(unsigned int new_recorded_date);

    unsigned int get_duration(void) const;
    void set_duration(unsigned int new_duration);

    std::string get_filepath(void) const;
    void set_filepath(std::string new_filepath);

    std::string get_description(void) const;
    void set_description(std::string new_description);

    void print(void) const;

private:
    unsigned int id;
    unsigned int user_id;
    unsigned int subject_id;
    unsigned int date;
    unsigned int recorded_date;
    unsigned int duration;
    std::string filepath;
    std::string description;
    PracticeEvent_status status;
};

#endif //__PRACTICE_EVENT_HPP
