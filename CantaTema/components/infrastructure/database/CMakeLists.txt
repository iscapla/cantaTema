include(library_install_infra)

create_infra_library(
    TARGET
    "database"
    SRCS
    src/db_category.cpp
    src/db_connection.cpp
    src/db_main.cpp
    src/db_subject.cpp
    src/db_user.cpp
    src/db_user_metrics.cpp
    INC_PUBLIC
    INC_PRIVATE
    sqlite3
    CORE::PRIMITIVES
)

add_subdirectory(test)
