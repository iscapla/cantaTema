include(library_install_infra)

create_infra_library(
    TARGET
    "operations"
    SRCS
    src/Operation_User_Logic.cpp
    src/Operation_Category_Logic.cpp
    src/Operation_Subject_Logic.cpp
    INC_PUBLIC
    INC_PRIVATE
    CORE::PRIMITIVES
    CORE::CONFIGURATION
    INFRASTRUCTURE::FILE_HANDLER
    INFRASTRUCTURE::DATABASE
)

# add_subdirectory(mocks)
# add_subdirectory(test)