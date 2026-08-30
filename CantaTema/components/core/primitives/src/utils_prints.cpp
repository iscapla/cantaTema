#include "primitives/utils_prints.hpp"
#include "primitives/utils_functions.hpp"
#include "primitives/tool_paths.hpp"
#include <spdlog/fmt/fmt.h>
#include <algorithm>

const std::string UtilsPrints::get_user_header(void)
{
    return fmt::format("| {:>2s} | {:>6s} | {:>10s} | {:>15s} | {:>20s} | {:>20s} | {:>3s} | {:>10s} |",
                       "Ok", "ID", "Name", "Password", "Creation Date", "Status", "Rol", "Space (KB)");
}

const std::string UtilsPrints::get_category_header(void)
{
    return fmt::format("| {:>6s} | {:>6s} | {:>20s} |", "ID", "User", "Name");
}

const std::string UtilsPrints::get_tag_header(void)
{
    return fmt::format("| {:>6s} | {:>6s} | {:>20s} |", "ID", "User", "Name");
}

const std::string UtilsPrints::get_subject_header(void)
{
    return fmt::format("| {:>6s} | {:>6s} | {:>6s} | {:>20s} | {:>30s} | {:>20s} |", "ID", "User", "Cat", "Name", "Filepath", "Tags");
}

const std::string UtilsPrints::get_practice_event_header(void)
{
    return fmt::format("| {:>6s} | {:>6s} | {:>6s} | {:>10s} | {:>10s} | {:>10s} | {:>10s} | {:>36s} | {:>30s} | {:>20s} |",
                       "ID", "User", "Subj", "Date", "Rec Date", "Duration", "Status", "Analysis ID", "Filepath", "Description");
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

const std::string UtilsPrints::get_tag_body(const Tag &tag)
{
    return fmt::format("| {:>6d} | {:>6d} | {:>20s} |",
                       tag.get_id(), tag.get_user_id(), tag.get_name());
}

const std::string UtilsPrints::format_path_for_display(const std::string &path)
{
    if (path.empty()) return path;

    std::string db_dir_str = ToolPath::get_path_for_database().string();
    if (!db_dir_str.empty()) {
        std::string norm_path = std::filesystem::path(path).lexically_normal().string();
        std::string norm_db = std::filesystem::path(db_dir_str).lexically_normal().string();

        if (norm_path.rfind(norm_db, 0) == 0) {
            std::string rel = norm_path.substr(norm_db.length());
            if (!rel.empty() && (rel[0] == '\\' || rel[0] == '/')) {
                rel = rel.substr(1);
            }
            return rel;
        }
    }

    std::string lower_path = path;
    for (auto &c : lower_path) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    size_t pos = lower_path.find("\\data\\");
    if (pos != std::string::npos) {
        return path.substr(pos + 6);
    }

    pos = lower_path.find("/data/");
    if (pos != std::string::npos) {
        return path.substr(pos + 6);
    }

    pos = lower_path.find("\\data/");
    if (pos != std::string::npos) {
        return path.substr(pos + 6);
    }

    pos = lower_path.find("/data\\");
    if (pos != std::string::npos) {
        return path.substr(pos + 6);
    }

    return path;
}

const std::string UtilsPrints::get_subject_body(const Subject &subject)
{
    std::string tags_str = "";
    const auto &tags = subject.get_tags();
    for (size_t i = 0; i < tags.size(); ++i)
    {
        tags_str += tags[i].get_name();
        if (i + 1 < tags.size())
        {
            tags_str += ", ";
        }
    }
    if (tags_str.empty())
    {
        tags_str = "-";
    }

    return fmt::format("| {:>6d} | {:>6d} | {:>6d} | {:>20s} | {:>30s} | {:>20s} |",
                       subject.get_id(), subject.get_user_id(), subject.get_category_id(), subject.get_name(), format_path_for_display(subject.get_filepath()), tags_str);
}

const std::string UtilsPrints::get_practice_event_body(const PracticeEvent &practice_event)
{
    std::string date_str = parse_time_t_to_string((time_t)practice_event.get_date(), DATE_STRING_FORMAT_SHORT);
    std::string recorded_date_str = parse_time_t_to_string((time_t)practice_event.get_recorded_date(), DATE_STRING_FORMAT_SHORT);
    std::string analysis_id = practice_event.get_analysis_execution_id().empty() ? "None" : practice_event.get_analysis_execution_id();
    
    return fmt::format("| {:>6d} | {:>6d} | {:>6d} | {:>10s} | {:>10s} | {:>10d} | {:>10s} | {:>36s} | {:>30s} | {:>20s} |",
                       practice_event.get_id(), practice_event.get_user_id(), practice_event.get_subject_id(),
                       date_str, recorded_date_str, practice_event.get_duration(),
                       PracticeEvent::get_status_as_string(practice_event.get_status()),
                       analysis_id,
                       format_path_for_display(practice_event.get_filepath()), practice_event.get_description());
}

const std::string UtilsPrints::get_user_metrics_body(const UserMetrics &user_metrics)
{
    return fmt::format("| {:>6d} | {:>10d} |",
                       user_metrics.get_useraccountid(), user_metrics.get_space_used_kb());
}