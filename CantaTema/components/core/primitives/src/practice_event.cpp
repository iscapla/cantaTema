#include "primitives/practice_event.hpp"
#include "primitives/utils_functions.hpp"
#include "primitives/utils_logger.hpp"

const std::string PracticeEvent::get_status_as_string(PracticeEvent_status status)
{
    switch (status)
    {
    case PLANNED:
        return "PLANNED";
    case RECORDED:
        return "RECORDED";
    case REMOVED:
        return "REMOVED";
    default:
        return "UNKNOWN";
    }
}

const PracticeEvent::PracticeEvent_status PracticeEvent::parse_status_from_string(const std::string status)
{
    if (status == "PLANNED")
    {
        return PLANNED;
    }
    else if (status == "RECORDED")
    {
        return RECORDED;
    }
    else if (status == "REMOVED")
    {
        return REMOVED;
    }
    return UNKNOWN;
}

PracticeEvent::PracticeEvent(void) : id(0),
                                    user_id(0),
                                    subject_id(0),
                                    date(0),
                                    recorded_date(0),
                                    duration(0),
                                    filepath(""),
                                    description(""),
                                    status(PracticeEvent_status::UNKNOWN)
{
}

PracticeEvent::~PracticeEvent(void) {}

unsigned int PracticeEvent::get_id(void) const { return id; }
void PracticeEvent::set_id(unsigned int new_id) { id = new_id; }

unsigned int PracticeEvent::get_user_id(void) const { return user_id; }
void PracticeEvent::set_user_id(unsigned int new_user_id) { user_id = new_user_id; }

unsigned int PracticeEvent::get_subject_id(void) const { return subject_id; }
void PracticeEvent::set_subject_id(unsigned int new_subject_id) { subject_id = new_subject_id; }

unsigned int PracticeEvent::get_date(void) const { return date; }
void PracticeEvent::set_date(unsigned int new_date) { date = new_date; }

unsigned int PracticeEvent::get_recorded_date(void) const { return recorded_date; }
void PracticeEvent::set_recorded_date(unsigned int new_recorded_date) { recorded_date = new_recorded_date; }

unsigned int PracticeEvent::get_duration(void) const { return duration; }
void PracticeEvent::set_duration(unsigned int new_duration) { duration = new_duration; }

std::string PracticeEvent::get_filepath(void) const { return filepath; }
void PracticeEvent::set_filepath(std::string new_filepath) { filepath = new_filepath; }

std::string PracticeEvent::get_description(void) const { return description; }
void PracticeEvent::set_description(std::string new_description) { description = new_description; }

PracticeEvent::PracticeEvent_status PracticeEvent::get_status(void) const { return status; }
void PracticeEvent::set_status(PracticeEvent_status new_status) { status = new_status; }
