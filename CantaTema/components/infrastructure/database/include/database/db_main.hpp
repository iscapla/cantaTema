#ifndef __DB_MAIN_HPP
#define __DB_MAIN_HPP

#include <string>

#include "primitives/utils_logger.hpp"
#include "primitives/definitions.hpp"

/**
 * @brief DB_Main class impement a Singleton structure as a unique point to initialize database
 *
 */
class DB_Main
{
private:
    /**
     * @brief Local instance to work as a Singleton. Static pointer which will points to the instance of this class
     */
    static DB_Main *instancePtr;

    /**
     * @brief Construct a new Request Singleton object
     */
    DB_Main(void);

    /**
     * @brief Destroy the Request Singleton object
     */
    ~DB_Main(void);

    /**
     * @brief Flag to indicate if the database system has been initialized
     */
    static bool initialized;
    

public:
    /**
     * @brief [DELETED] Construct a new Request Singleton object
     * @param obj
     */
    DB_Main(const DB_Main &obj) = delete;

    /**
     * @brief Get the Instance object
     * @return DB_Main*
     */
    static DB_Main *getInstance(void)
    {

        // Initialize the CURL stack on the first call
        if (instancePtr == NULL)
        {
            // We can access private members within the class.
            instancePtr = new DB_Main();

            // returning the instance pointer
            return instancePtr;
        }
        else
        {
            // Return instance if it was created before
            return instancePtr;
        }
    }

    /**
     * @brief Purge the database
     */
    void purge(void);
    
};

#endif //__DB_MAIN_HPP