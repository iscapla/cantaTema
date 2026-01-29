
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

    # REQUIRED to allow linking into shared libraries
    set_target_properties(tinyfiledialogs PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

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
            ARCHIVE DESTINATION ${CMAKE_BINARY_DIR}/lib
            LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/lib
            RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/bin
            INCLUDES DESTINATION ${INSTALL_INCLUDE_DIR}
    )

    install(EXPORT tinyfiledialogsTargets
            FILE tinyfiledialogsTargets.cmake
            NAMESPACE ${CMAKE_MAIN_PROJECT_NAME}::
            DESTINATION "${INSTALL_CMAKE_DIR}/tinyfiledialogs"
    )
endif()
