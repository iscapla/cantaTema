#include "primitives/utils_prints.hpp"
#include "primitives/utils_functions.hpp"
#include <spdlog/fmt/fmt.h>

const std::string UtilsPrints::get_user_header(void)
{
    return fmt::format("| {:>2s} | {:>6s} | {:>10s} | {:>15s} | {:>20s} | {:>20s} | {:>3s} | {:>10s} |",
                       "Ok", "ID", "Name", "Password", "Creation Date", "Status", "Rol", "Space (KB)");
}

const std::string UtilsPrints::get_category_header(void)
{
    return fmt::format("| {:>6s} | {:>6s} | {:>20s} |", "ID", "User", "Name");
}

const std::string UtilsPrints::get_subject_header(void)
{
    return fmt::format("| {:>6s} | {:>6s} | {:>6s} | {:>20s} | {:>30s} |", "ID", "User", "Cat", "Name", "Filepath");
}

const std::string UtilsPrints::get_practice_event_header(void)
{
    return fmt::format("| {:>6s} | {:>6s} | {:>6s} | {:>10s} | {:>10s} | {:>10s} | {:>10s} | {:>30s} | {:>20s} |",
                       "ID", "User", "Subj", "Date", "Rec Date", "Duration", "Status", "Filepath", "Description");
}

const std::string UtilsPrints::get_user_metrics_header(void)
{
    return fmt::format("| {:>6s} | {:>10s} |", "ID", "Space (KB)");
}

const std::string UtilsPrints::get_user_body(const User &user)
{
    std::string t_creationdate = parse_time_t_to_string(user.get_creationdate(), DATE_STRING_FORMAT_LONG);
    return fmt::format("| {:>2d} | {:>6d} | {:>10s} | {:>15s} | {:>20s} | {:>20s} | {:>3d} | {:>10d} |",
                       user.get_is_authenticated(), user.get_useraccountid(), user.get_name(), user.get_passwordkey(),
                       t_creationdate, user.parse_status_to_string(user.get_status()), user.get_roleid(), user.get_max_space_size_in_kb());
}

const std::string UtilsPrints::get_category_body(const Category &category)
{
    return fmt::format("| {:>6d} | {:>6d} | {:>20s} |",
                       category.get_id(), category.get_user_id(), category.get_name());
}

const std::string UtilsPrints::get_subject_body(const Subject &subject)
{
    return fmt::format("| {:>6d} | {:>6d} | {:>6d} | {:>20s} | {:>30s} |",
                       subject.get_id(), subject.get_user_id(), subject.get_category_id(), subject.get_name(), subject.get_filepath());
}

const std::string UtilsPrints::get_practice_event_body(const PracticeEvent &practice_event)
{
    std::string date_str = parse_time_t_to_string((time_t)practice_event.get_date(), DATE_STRING_FORMAT_SHORT);
    std::string recorded_date_str = parse_time_t_to_string((time_t)practice_event.get_recorded_date(), DATE_STRING_FORMAT_SHORT);
    
    return fmt::format("| {:>6d} | {:>6d} | {:>6d} | {:>10s} | {:>10s} | {:>10d} | {:>10s} | {:>30s} | {:>20s} |",
                       practice_event.get_id(), practice_event.get_user_id(), practice_event.get_subject_id(),
                       date_str, recorded_date_str, practice_event.get_duration(),
                       PracticeEvent::get_status_as_string(practice_event.get_status()),
                       practice_event.get_filepath(), practice_event.get_description());
}

const std::string UtilsPrints::get_user_metrics_body(const UserMetrics &user_metrics)
{
    return fmt::format("| {:>6d} | {:>10d} |",
                       user_metrics.get_useraccountid(), user_metrics.get_space_used_kb());
}