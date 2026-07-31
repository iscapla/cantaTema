
include(FetchContent)
find_package(Threads REQUIRED)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-36-0
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(asio)

# ASIO doesn't use CMake, we have to configure it manually. Extra notes for using on Windows:
#
# 1) If _WIN32_WINNT is not set, ASIO assumes _WIN32_WINNT=0x0501, i.e. Windows XP target, which is
# definitely not the platform which most users target.
#
# 2) WIN32_LEAN_AND_MEAN is defined to make Winsock2 work.

add_library(asio INTERFACE)

# REQUIRED to allow linking into shared libraries
set_target_properties(asio PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_include_directories(asio SYSTEM INTERFACE  
    $<BUILD_INTERFACE:${asio_SOURCE_DIR}/asio/include>  
    $<INSTALL_INTERFACE:asio/include>
)

target_compile_definitions(asio INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)
target_link_libraries(asio INTERFACE Threads::Threads)

# install target
install(TARGETS asio
        EXPORT asioTargets
        ARCHIVE DESTINATION ${CMAKE_BINARY_DIR}/lib
        LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/lib
        RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/bin
        INCLUDES DESTINATION ${INSTALL_INCLUDE_DIR}
)

# Install the export set for use with the install-tree
install(EXPORT asioTargets
        FILE asioTargets.cmake
        NAMESPACE ${CMAKE_MAIN_PROJECT_NAME}::
        DESTINATION "${INSTALL_CMAKE_DIR}/asio"
)

if(UNIX)
  target_link_libraries(asio INTERFACE pthread)
elseif(WIN32)
  # macro see @ https://stackoverflow.com/a/40217291/1746503
  macro(get_win32_winnt version)
    if(CMAKE_SYSTEM_VERSION)
      set(ver ${CMAKE_SYSTEM_VERSION})
      string(REGEX MATCH "^([0-9]+).([0-9])" ver ${ver})
      string(REGEX MATCH "^([0-9]+)" verMajor ${ver})
      # Check for Windows 10, b/c we'll need to convert to hex 'A'.
      if("${verMajor}" MATCHES "10")
        set(verMajor "A")
        string(REGEX REPLACE "^([0-9]+)" ${verMajor} ver ${ver})
      endif("${verMajor}" MATCHES "10")
      # Remove all remaining '.' characters.
      string(REPLACE "." "" ver ${ver})
      # Prepend each digit with a zero.
      string(REGEX REPLACE "([0-9A-Z])" "0\\1" ver ${ver})
      set(${version} "0x${ver}")
    endif()
  endmacro()

  if(NOT DEFINED _WIN32_WINNT)
    get_win32_winnt(ver)
    set(_WIN32_WINNT ${ver})
  endif()

  message(STATUS "Set _WIN32_WINNET=${_WIN32_WINNT}")

  target_compile_definitions(asio INTERFACE _WIN32_WINNT=${_WIN32_WINNT} WIN32_LEAN_AND_MEAN)
  target_link_libraries(asio INTERFACE ws2_32)
endif()