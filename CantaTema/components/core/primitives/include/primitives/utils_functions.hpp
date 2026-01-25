#ifndef __UTILS_FUNCTIONS_HPP
#define __UTILS_FUNCTIONS_HPP

#include <sstream>
#include <iomanip>
#include <math.h>
#include <SDL3/SDL.h>

#include "definitions.hpp"

/**
 * @brief Convert string text with a given format into a time_t value
 * 
 * @param text 
 * @param format 
 * @return time_t 
 */
inline time_t parse_time_to_time_t(std::string &text, const char *format) noexcept
{

    try
    {
        if (text.empty())
        {
            return 0;
        }
        else
        {
            struct std::tm tm
            {
            };

            std::istringstream ss(text);
            ss >> std::get_time(&tm, format);

            if (ss.fail())
            {
                return 0;
            }
            else
            {
                return mktime(&tm);
            }
        }
    }
    catch (...)
    {
        return 0;
    }
}

/**
 * @brief Return a string with the value of <time> using the time format
 * 
 * @param time 
 * @param format 
 * @return std::string 
 */
inline std::string parse_time_t_to_string(time_t time, const char *format) noexcept
{
    std::ostringstream oss;

    if (time == 0)
        return std::string{};

    struct tm *tminfo{std::localtime(&time)};

    oss << std::put_time(tminfo, format);
    return oss.str();
}

/**
 * @brief Get the platform specific path for application data using SDL.
 * The directory is created if it does not exist.
 * 
 * @param org_name Organization name
 * @param app_name Application name
 * @return std::string Path with trailing separator
 */
inline std::string get_filesystem_path(const std::string &org_name, const std::string &app_name) noexcept
{
    char *base_path = SDL_GetPrefPath(org_name.c_str(), app_name.c_str());
    if (base_path)
    {
        std::string path(base_path);
        SDL_free(base_path);
        return path;
    }
    return "";
}

#endif //__UTILS_FUNCTIONS_HPP