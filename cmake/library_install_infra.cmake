include(CMakeParseArguments)

function(create_infra_library)

    cmake_parse_arguments(
        PARSED_ARGS # prefix of output variables
        "" # list of names of the boolean arguments (only defined ones will be true)
        "TARGET" # list of names of mono-valued arguments
        "SRCS;INC_PUBLIC;INC_PRIVATE" # list of names of multi-valued arguments (output variables are lists)
        ${ARGN} # arguments of the function to parse, here we take the all original ones
    )
    # note: if it remains unparsed arguments, here, they can be found in variable PARSED_ARGS_UNPARSED_ARGUMENTS
    if(NOT PARSED_ARGS_TARGET)
        message(FATAL_ERROR "You must provide a target")
    endif(NOT PARSED_ARGS_TARGET)

    string(TOUPPER "${PARSED_ARGS_TARGET}" PARSED_ARGS_TARGET)
    set(TARGET_NAME "_infra_${PARSED_ARGS_TARGET}")

    project(${TARGET_NAME})

    message(STATUS "Building INFRA library for ${TARGET_NAME} project...")
    
    # do not define neither static nor shared mode, this must be set by
    # -DBUILD_SHARED_LIBS=ON|OFF
    add_library(${TARGET_NAME} ${PARSED_ARGS_SRCS})
    
    if(PARSED_ARGS_INC_PUBLIC)
        list(REMOVE_ITEM PARSED_ARGS_INC_PUBLIC ${TARGET_NAME})
        target_link_libraries(${TARGET_NAME} PUBLIC ${PARSED_ARGS_INC_PUBLIC})
    endif(PARSED_ARGS_INC_PUBLIC)
    
    if(PARSED_ARGS_INC_PRIVATE)
        list(REMOVE_ITEM PARSED_ARGS_INC_PRIVATE ${TARGET_NAME})
        target_link_libraries(${TARGET_NAME} PRIVATE ${PARSED_ARGS_INC_PRIVATE})
    endif(PARSED_ARGS_INC_PRIVATE)
    
    # define alias
    add_library(${COMPONENT_PROJECT_NAME}::${PARSED_ARGS_TARGET} ALIAS ${TARGET_NAME})
    # message(STATUS "${TARGET_NAME} will be accessible as ${COMPONENT_PROJECT_NAME}::${PARSED_ARGS_TARGET}")
    
    # instruct the target to know how to find the include files (the generated one
    # above and the lib header)
    target_include_directories(
        ${TARGET_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include_priv>
    )
    
    if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${TARGET_NAME} PUBLIC --coverage)
        target_link_options(${TARGET_NAME} PUBLIC --coverage)
    endif()


    # install target
    install(TARGETS ${TARGET_NAME}
            EXPORT ${TARGET_NAME}Targets
            LIBRARY DESTINATION ${INSTALL_LIB_DIR}
            RUNTIME DESTINATION ${INSTALL_BIN_DIR}
            ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
            INCLUDES DESTINATION ${INSTALL_INCLUDE_DIR}
    )


endfunction(create_infra_library)