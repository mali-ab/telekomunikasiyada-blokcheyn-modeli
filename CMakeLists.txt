cmake_minimum_required(VERSION 3.16)

project(TurkmentelekomBlockchain LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_CONSOLE "Build the original terminal operator console" ON)

find_package(Qt6 COMPONENTS Widgets QUIET)
if (NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Widgets QUIET)
endif()

if (Qt6_FOUND)
    set(QT_WIDGETS_TARGET Qt6::Widgets)
elseif (Qt5_FOUND)
    set(QT_WIDGETS_TARGET Qt5::Widgets)
else()
    message(FATAL_ERROR "Qt Widgets was not found. Install Qt 6 (or Qt 5) development packages.")
endif()

find_package(OpenSSL REQUIRED)

find_package(PQXX QUIET)
if (NOT PQXX_FOUND)
    find_path(PQXX_INCLUDE_DIR NAMES pqxx/pqxx)
    find_library(PQXX_LIBRARY NAMES pqxx)
    if (PQXX_INCLUDE_DIR AND PQXX_LIBRARY)
        add_library(pqxx::pqxx UNKNOWN IMPORTED)
        set_target_properties(pqxx::pqxx PROPERTIES
            IMPORTED_LOCATION "${PQXX_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${PQXX_INCLUDE_DIR}")
        set(PQXX_LIBRARIES pqxx::pqxx)
    else()
        message(FATAL_ERROR "libpqxx not found. Install libpqxx-dev (e.g. apt install libpqxx-dev).")
    endif()
else()
    set(PQXX_LIBRARIES pqxx)
endif()

set(TELEKOM_CORE_SOURCES
    src/blockchain.cpp
    src/database.cpp
    src/signature.cpp
    src/smartContract.cpp
    src/subscriber_repository.cpp
)

add_executable(telekom_desktop
    ui/main.cpp
    ui/main_window.cpp
    ui/main_window_pages.cpp
    ui/ui_helpers.cpp
    ${TELEKOM_CORE_SOURCES}
)
target_include_directories(telekom_desktop PRIVATE include)
target_link_libraries(telekom_desktop PRIVATE ${QT_WIDGETS_TARGET} OpenSSL::SSL OpenSSL::Crypto ${PQXX_LIBRARIES} pq)
set_target_properties(telekom_desktop PROPERTIES WIN32_EXECUTABLE TRUE MACOSX_BUNDLE TRUE)

if (BUILD_CONSOLE)
    find_package(Boost REQUIRED COMPONENTS system)

    add_executable(telekom_system
        console/main.cpp
        ${TELEKOM_CORE_SOURCES}
        src/p2p.cpp
    )
    target_include_directories(telekom_system PRIVATE include)
    target_link_libraries(telekom_system PRIVATE OpenSSL::SSL OpenSSL::Crypto Boost::system ${PQXX_LIBRARIES} pq)
endif()
