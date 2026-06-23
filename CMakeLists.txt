cmake_minimum_required(VERSION 3.21)
cmake_policy(VERSION 3.21)

project(lamp_monitor VERSION 0.1.0.0 LANGUAGES CXX C)

include(GNUInstallDirs)

# ----------------------------------------------------------------------------------------------------------------------
# Project dependencies
# ----------------------------------------------------------------------------------------------------------------------
find_package(fmt 12.1.0 REQUIRED)
find_package(Git REQUIRED)
find_package(pugixml 1.15 REQUIRED)
find_package(yaml-cpp 0.7 REQUIRED)

include(CMakeFindDependencyMacro)
find_package(yaml-schema-cpp 0.0.0 REQUIRED)

include(third_party/argparse.cmake)
include(third_party/c++ami.cmake)
include(third_party/cpp-httplib.cmake)
include(third_party/dbpool.cmake)
include(third_party/exprtk.cmake)

execute_process (
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_HASH_SHORT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# ----------------------------------------------------------------------------------------------------------------------
# Project definition
# ----------------------------------------------------------------------------------------------------------------------
add_executable(lamp_monitor "")

target_compile_features(lamp_monitor PRIVATE cxx_std_20)

if(NOT DEFINED LAMP_MONITOR_SCHEMA_DIR)
    set(LAMP_MONITOR_SCHEMA_DIR ${PROJECT_SOURCE_DIR}/schema)
endif()

set(LAMP_MONITOR_NAME ${CMAKE_PROJECT_NAME})
set(LAMP_MONITOR_VERSION ${CMAKE_PROJECT_VERSION})
configure_file(${PROJECT_SOURCE_DIR}/include/lamp_defs.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/include/lamp_defs.hpp)

target_compile_options (lamp_monitor PRIVATE -fPIC -Wall -Wextra -Wshadow -pedantic -pedantic-errors)

target_include_directories(lamp_monitor
PUBLIC
    include
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

target_link_libraries(lamp_monitor
PUBLIC
    anl
    argparse
    c++ami
    dbpool
    exprtk::exprtk
    httplib
    pugixml
    yaml-cpp::yaml-cpp
    yaml-schema-cpp
)

target_sources(lamp_monitor
PRIVATE
    src/asterisk/config/PjsipWizardConfig.cpp
    src/asterisk/EventHandler.cpp
    src/asterisk/RegisterEventHandler.cpp
    src/asterisk/NightEventHandler.cpp
    src/asterisk/ParkEventHandler.cpp
    src/bridge/provider/PjsipWizardProvider.cpp
    src/bridge/AorProvider.cpp
    src/bridge/NightButton.cpp
    src/bridge/ParkButton.cpp
    src/bridge/PhoneUi.cpp
    src/bridge/PhoneUiState.cpp
    src/bridge/yealink/NightButton.cpp
    src/bridge/yealink/ParkButton.cpp
    src/bridge/yealink/PhoneUi.cpp
    src/button_state/ButtonPlan.cpp
    src/button_state/PhoneButton.cpp
    src/cache/DeskphoneCache.cpp
    src/match/SymbolMap.cpp
    src/phonebook/provider/PjsipWizardProvider.cpp
    src/phonebook/snom/Phonebook.cpp
    src/phonebook/yealink/Phonebook.cpp
    src/phonebook/Phonebook.cpp
    src/phonebook/PhonebookProvider.cpp
    src/ConfigFileParser.cpp
    src/main.cpp
)

install(TARGETS lamp_monitor
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/schema
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}
)
install(FILES ${PROJECT_SOURCE_DIR}/sample_config/config.yaml
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}
)
