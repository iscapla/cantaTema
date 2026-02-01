/**
 * @file definitions.hpp
 * @author Ismael C
 * @date 2026-01-06
 * @brief Defines main project definitions and methods
 *
 */

#ifndef __GLOBAL_DEFINITIONS_HPP
#define __GLOBAL_DEFINITIONS_HPP

#include <primitives/utils_logger.hpp>

typedef long long _type_integer;
typedef double _type_decimal;

#define DATE_STRING_FORMAT_SHORT "%Y-%m-%d"
#define DATE_STRING_FORMAT_LONG  "%Y-%m-%d %H:%M:%S"

/**
 * @brief General return definition for the entire project
 *
 */
enum rst_code_e
{

    RST_OK = 0, // No error

    CONFIG_FILE,  // Error loading config files
    CONFIG_PARSE, // Error parsing config variable

    DB_FAIL,      // General DB fail
    DB_NOT_FOUND, // Key not found on DB
    DB_BAD_PARAM, // Format or type error

    CONSOLE_EXP, // Exception on console

    USER_ERROR,      // User general error
    USER_NOT_FOUND,  // User not present on the DB
    USER_NO_AUTH,    // User not authorized
    USER_DUPLICATED, // User is already present on the database

    USER_METRICS_ERROR, // User metrics general error
    USER_METRICS_NOT_FOUND, // User metrics not found
    USER_METRICS_NOT_ENOUGH_SPACE, // User metrics not enough space

    CATEGORY_ERROR,      // Category general error
    CATEGORY_NOT_FOUND,  // Category not found
    CATEGORY_DUPLICATED, // Category already exists

    SUBJECT_ERROR,      // Subject general error
    SUBJECT_NOT_FOUND,  // Subject not found
    SUBJECT_DUPLICATED, // Subject already exists

    FILE_NOT_FOUND, // File not found
    FILE_READ_ERROR, // File read error
    FILE_UPLOAD_ERROR, // File upload error

    UNKNOWN // General error. Also used to point to the last value of the table
};

inline const unsigned int get_rst_code(const rst_code_e &rst)
{
    return rst;
}

inline const std::string get_rst_txt(const rst_code_e &rst)
{

    std::string txt{};

    switch (rst)
    {
    case RST_OK:
        txt = "RST_OK";
        break;

    case CONFIG_FILE:
        txt = "CONFIG_FILE";
        break;
    case CONFIG_PARSE:
        txt = "CONFIG_PARSE";
        break;

    case DB_FAIL:
        txt = "DB_FAIL";
        break;
    case DB_NOT_FOUND:
        txt = "DB_NOT_FOUND";
        break;
    case DB_BAD_PARAM:
        txt = "DB_BAD_PARAM";
        break;

    case CONSOLE_EXP:
        txt = "CONSOLE_EXP";
        break;

    case USER_ERROR:
        txt = "USER_ERROR";
        break;
    case USER_NOT_FOUND:
        txt = "USER_NOT_FOUND";
        break;
    case USER_NO_AUTH:
        txt = "USER_NO_AUTH";
        break;
    case USER_DUPLICATED:
        txt = "USER_DUPLICATED";
        break;
    
    case USER_METRICS_ERROR:
        txt = "USER_METRICS_ERROR";
        break;
    case USER_METRICS_NOT_FOUND:
        txt = "USER_METRICS_NOT_FOUND";
        break;
    case USER_METRICS_NOT_ENOUGH_SPACE:
        txt = "USER_METRICS_NOT_ENOUGH_SPACE";
        break;

    case CATEGORY_ERROR:
        txt = "CATEGORY_ERROR";
        break;
    case CATEGORY_NOT_FOUND:
        txt = "CATEGORY_NOT_FOUND";
        break;
    case CATEGORY_DUPLICATED:
        txt = "CATEGORY_DUPLICATED";
        break;

    case SUBJECT_ERROR:
        txt = "SUBJECT_ERROR";
        break;
    case SUBJECT_NOT_FOUND:
        txt = "SUBJECT_NOT_FOUND";
        break;
    case SUBJECT_DUPLICATED:
        txt = "SUBJECT_DUPLICATED";
        break;

    case FILE_NOT_FOUND:
        txt = "FILE_NOT_FOUND";
        break;
    case FILE_READ_ERROR:
        txt = "FILE_READ_ERROR";
        break;
    case FILE_UPLOAD_ERROR:
        txt = "FILE_UPLOAD_ERROR";
        break;

    case UNKNOWN:
    default:
        txt = "UNKNOWN";
        break;
    }

    return txt;
}

#endif //__GLOBAL_DEFINITIONS_HPP