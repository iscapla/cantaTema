
FetchContent_Declare(
tinyfiledialogs
GIT_REPOSITORY http://git.code.sf.net/p/tinyfiledialogs/code
GIT_TAG 5fdb74764a5026d059517911fc085a1cd9da8cee
GIT_SHALLOW FALSE
)

FetchContent_GetProperties(tinyfiledialogs)
if(NOT tinyfiledialogs_POPULATED)
    FetchContent_Populate(tinyfiledialogs)
    FetchContent_MakeAvailable(tinyfiledialogs)
    add_library(tinyfiledialogs ${tinyfiledialogs_SOURCE_DIR}/tinyfiledialogs.c)
    target_include_directories(tinyfiledialogs
        PUBLIC
        $<BUILD_INTERFACE:${tinyfiledialogs_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    )
    if(WIN32)
        target_link_libraries(tinyfiledialogs PRIVATE comdlg32 ole32)
    endif()

    install(TARGETS tinyfiledialogs
            EXPORT tinyfiledialogsTargets
            LIBRARY DESTINATION ${INSTALL_LIB_DIR}
            RUNTIME DESTINATION ${INSTALL_BIN_DIR}
            ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
            INCLUDES DESTINATION ${INSTALL_INCLUDE_DIR}
    )

    install(EXPORT tinyfiledialogsTargets
            FILE tinyfiledialogsTargets.cmake
            NAMESPACE ${CMAKE_MAIN_PROJECT_NAME}::
            DESTINATION "${INSTALL_CMAKE_DIR}/tinyfiledialogs"
    )
endif()
