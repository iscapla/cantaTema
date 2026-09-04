include(library_install_test)

create_test_exec(
    TARGET
        "t_c_api"
    SRCS
        t_c_api.cxx
    INC_PRIVATE
        cantatema_bridge
        CORE::PRIMITIVES
        CORE::CONFIGURATION
        BUSINESS::SESSION
        nlohmann_json::nlohmann_json
)

add_custom_command(TARGET t_c_api POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:cantatema_bridge>
        $<TARGET_FILE_DIR:t_c_api>
)

