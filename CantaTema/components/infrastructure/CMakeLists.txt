# Set COMPONENT_PROJECT_NAME based on the directory name
get_filename_component(DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(TOUPPER ${DIR_NAME} COMPONENT_PROJECT_NAME)

add_subdirectory(database)
add_subdirectory(file_handler)
