include(CMakeParseArguments)

function(create_test_exec)

    cmake_parse_arguments(
        PARSED_ARGS # prefix of output variables
        "" # list of names of the boolean arguments (only defined ones will be true)
        "TARGET" # list of names of mono-valued arguments
        "SRCS;INC_PRIVATE" # list of names of multi-valued arguments (output variables are lists)
        ${ARGN} # arguments of the function to parse, here we take the all original ones
    )
    # note: if it remains unparsed arguments, here, they can be found in variable PARSED_ARGS_UNPARSED_ARGUMENTS
    if(NOT PARSED_ARGS_TARGET)
        message(FATAL_ERROR "You must provide a target")
    endif(NOT PARSED_ARGS_TARGET)

    project(${PARSED_ARGS_TARGET})

    message(STATUS "Building TEST ${PARSED_ARGS_TARGET} project...")
    
    add_executable(${PARSED_ARGS_TARGET} ${PARSED_ARGS_SRCS})
    
    if(TARGET mupdf_make)
        add_dependencies(${PARSED_ARGS_TARGET} mupdf_make)
    endif()
    if(TARGET mupdf_ext)
        add_dependencies(${PARSED_ARGS_TARGET} mupdf_ext)
    endif()
    
    if(PARSED_ARGS_INC_PRIVATE)
        target_link_libraries(${PARSED_ARGS_TARGET} PRIVATE gtest gmock ${PARSED_ARGS_INC_PRIVATE})
    endif(PARSED_ARGS_INC_PRIVATE)

    target_include_directories(
        ${PARSED_ARGS_TARGET}
        PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include_priv>
    )

    if(MINGW OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_options(${PARSED_ARGS_TARGET} PRIVATE -Wl,--allow-multiple-definition)
    endif()

    if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${PARSED_ARGS_TARGET} PRIVATE --coverage)
        target_link_options(${PARSED_ARGS_TARGET} PRIVATE --coverage)
    endif()

    # Set the runtime output directory for each executable
    set_target_properties(${PARSED_ARGS_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/Test)

    add_test(NAME ${PARSED_ARGS_TARGET} COMMAND ${PARSED_ARGS_TARGET} WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/Test)


endfunction(create_test_exec)