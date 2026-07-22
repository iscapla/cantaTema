/**
 * @file utils_prints.hpp
 * @author Ismael C
 * @brief Utilities to print formatted tables for primitive objects
 *
 */

#ifndef __UTILS_PRINTS_HPP
#define __UTILS_PRINTS_HPP

#include <iostream>
#include <string>

#include "primitives/definitions.hpp"

#include "primitives/user.hpp"
#include "primitives/category.hpp"
#include "primitives/subject.hpp"
#include "primitives/practice_event.hpp"
#include "primitives/user_metrics.hpp"

/**
 * @brief Helper class to generate formatted strings for console output
 * 
 */
class UtilsPrints{

public:
    /**
     * @brief Get the header string for the User table
     * 
     * @return const std::string 
     */
    static const std::string get_user_header(void);

    /**
     * @brief Get the header string for the Category table
     * 
     * @return const std::string 
     */
    static const std::string get_category_header(void);

    /**
     * @brief Get the header string for the Subject table
     * 
     * @return const std::string 
     */
    static const std::string get_subject_header(void);

    /**
     * @brief Get the header string for the Practice Event table
     * 
     * @return const std::string 
     */
    static const std::string get_practice_event_header(void);

    /**
     * @brief Get the header string for the User Metrics table
     * 
     * @return const std::string 
     */
    static const std::string get_user_metrics_header(void);

    /**
     * @brief Get the formatted body row for a User
     * 
     * @param user 
     * @return const std::string 
     */
    static const std::string get_user_body(const User &user);

    /**
     * @brief Get the formatted body row for a Category
     * 
     * @param category 
     * @return const std::string 
     */
    static const std::string get_category_body(const Category &category);

    /**
     * @brief Get the formatted body row for a Subject
     * 
     * @param subject 
     * @return const std::string 
     */
    static const std::string get_subject_body(const Subject &subject);

    /**
     * @brief Get the formatted body row for a Practice Event
     * 
     * @param practice_event 
     * @return const std::string 
     */
    static const std::string get_practice_event_body(const PracticeEvent &practice_event);

    /**
     * @brief Get the formatted body row for User Metrics
     * 
     * @param user_metrics 
     * @return const std::string 
     */
    static const std::string get_user_metrics_body(const UserMetrics &user_metrics);

    /**
     * @brief Format a file path for CLI table display, starting from "\data" or "/data" if present.
     * 
     * @param path 
     * @return const std::string 
     */
    static const std::string format_path_for_display(const std::string &path);

};

#endif //__UTILS_PRINTS_HPP