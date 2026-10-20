cmake_minimum_required(VERSION 3.16)

project(TurkmentelekomBlockchain LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_DESKTOP "Build the Qt desktop operator interface" ON)
option(BUILD_CONSOLE "Build the terminal operator and P2P prototype" ON)

if (NOT BUILD_DESKTOP AND NOT BUILD_CONSOLE)
    message(FATAL_ERROR "Enable BUILD_DESKTOP, BUILD_CONSOLE, or both.")
endif()

find_package(OpenSSL REQUIRED)

find_package(PQXX QUIET)
if (PQXX_FOUND)
    if (TARGET pqxx::pqxx)
        set(PQXX_TARGET pqxx::pqxx)
    elseif (TARGET pqxx)
        set(PQXX_TARGET pqxx)
    else()
        set(PQXX_TARGET ${PQXX_LIBRARIES})
    endif()
    set(PQXX_EXTRA_LIBRARY "")
else()
    find_path(PQXX_INCLUDE_DIR NAMES pqxx/pqxx)
    find_library(PQXX_LIBRARY NAMES pqxx)
    find_library(PQ_LIBRARY NAMES pq)
    if (NOT PQXX_INCLUDE_DIR OR NOT PQXX_LIBRARY)
        message(FATAL_ERROR "libpqxx not found. Install libpqxx-dev (for example: apt install libpqxx-dev).")
    endif()
    add_library(pqxx::pqxx UNKNOWN IMPORTED)
    set_target_properties(pqxx::pqxx PROPERTIES
        IMPORTED_LOCATION "${PQXX_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PQXX_INCLUDE_DIR}")
    set(PQXX_TARGET pqxx::pqxx)
    set(PQXX_EXTRA_LIBRARY "${PQ_LIBRARY}")
endif()

add_library(telekom_core STATIC
    src/blockchain.cpp
    src/database.cpp
    src/signature.cpp
    src/smartContract.cpp
    src/subscriber_repository.cpp
)
target_include_directories(telekom_core PUBLIC include)
target_link_libraries(telekom_core PUBLIC OpenSSL::Crypto ${PQXX_TARGET} ${PQXX_EXTRA_LIBRARY})

if (MSVC)
    target_compile_options(telekom_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(telekom_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (BUILD_DESKTOP)
    find_package(Qt6 COMPONENTS Widgets QUIET)
    if (Qt6_FOUND)
        set(QT_WIDGETS_TARGET Qt6::Widgets)
    else()
        find_package(Qt5 COMPONENTS Widgets QUIET)
        if (Qt5_FOUND)
            set(QT_WIDGETS_TARGET Qt5::Widgets)
        else()
            message(FATAL_ERROR "Qt Widgets was not found. Install Qt 6 (or Qt 5), or configure with -DBUILD_DESKTOP=OFF for the console-only build.")
        endif()
    endif()

    add_executable(telekom_desktop
        ui/main.cpp
        ui/main_window.cpp
        ui/main_window_pages.cpp
        ui/ui_helpers.cpp
    )
    target_link_libraries(telekom_desktop PRIVATE telekom_core ${QT_WIDGETS_TARGET})
    set_target_properties(telekom_desktop PROPERTIES WIN32_EXECUTABLE TRUE MACOSX_BUNDLE TRUE)
endif()

if (BUILD_CONSOLE)
    find_package(Boost REQUIRED COMPONENTS system)
    find_package(Threads REQUIRED)

    add_executable(telekom_system
        console/main.cpp
        src/p2p.cpp
    )
    target_link_libraries(telekom_system PRIVATE telekom_core Boost::system Threads::Threads)
endif()

include(CTest)
if (BUILD_TESTING)
    add_executable(telekom_domain_tests tests/domain_tests.cpp)
    target_link_libraries(telekom_domain_tests PRIVATE telekom_core)
    add_test(NAME telekom_domain_tests COMMAND telekom_domain_tests)
endif()
