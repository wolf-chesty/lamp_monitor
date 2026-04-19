cmake_minimum_required(VERSION 3.12)
cmake_policy(VERSION 3.12)

set(CMAKE_CXX_STANDARD 20)

project(lamp_monitor VERSION 0.0.0.0 LANGUAGES CXX C)

# ----------------------------------------------------------------------------------------------------------------------
# Options
# ----------------------------------------------------------------------------------------------------------------------
option(COVERAGE "Enable GCC Coverage" OFF)
option(PERF "Set GCC perf flags" OFF)
option(SANITIZE_ADDRESS "Enable AddressSanitizer (ASan) for memory error detection tests." OFF)
option(SANITIZE_THREAD "Enable ThreadSanitizer (TSan) for detecting thread-related issues." OFF)
option(UNIT_TESTS "Build unit tests" ON)

# ----------------------------------------------------------------------------------------------------------------------
# Project dependencies
# ----------------------------------------------------------------------------------------------------------------------
include(third_party/argparse.cmake)
include(third_party/c++ami.cmake)
include(third_party/cpp-httplib.cmake)
include(third_party/dbpool.cmake)
include(third_party/fmt.cmake)
include(third_party/pugixml.cmake)
include(third_party/yaml-cpp.cmake)
#include(third_party/yaml-schema-cpp.cmake)

if (COVERAGE)
    set(CPPAMI_COVERAGE ON)
    set(DBPOOL_COVERAGE ON)
elseif (PERF)
    set(CPPAMI_PERF ON)
    set(DBPOOL_PERF ON)
elseif (SANITIZE_ADDRESS)
    set(CPPAMI_SANITIZE_ADDRESS ON)
    set(DBPOOL_SANITIZE_ADDRESS ON)
elseif (SANITIZE_THREAD)
    set(CPPAMI_SANITIZE_THREAD ON)
    set(DBPOOL_SANITIZE_THREAD ON)
endif ()

# ----------------------------------------------------------------------------------------------------------------------
# Resource files
# ----------------------------------------------------------------------------------------------------------------------
#find_program(XXD_EXE xxd REQUIRED)
#
#set(SCHEMA_FILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/res/config.schema)
#set(SCHEMA_FILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/config_schema.h)
#add_custom_command(OUTPUT ${SCHEMA_FILE_OUT}
#    COMMAND ${XXD_EXE} -n config_schema -i ${SCHEMA_FILE_IN}  ${SCHEMA_FILE_OUT}
#    DEPENDS ${SCHEMA_FILE_IN}
#    COMMENT "Transforming ${SCHEMA_FILE_IN} to ${SCHEMA_FILE_OUT}"
#)

# ----------------------------------------------------------------------------------------------------------------------
# Project definition
# ----------------------------------------------------------------------------------------------------------------------
add_executable(lamp_monitor "")

set(CMAKE_CXX_STANDARD 20)

target_include_directories(lamp_monitor
    PUBLIC
    include
)

target_link_libraries(lamp_monitor
    PUBLIC
    anl
    argparse
    c++ami
    dbpool
    httplib
    pugixml
    yaml-cpp::yaml-cpp
)

target_sources(lamp_monitor
    PRIVATE
    src/asterisk/EventHandler.cpp
    src/asterisk/RegisterEventHandler.cpp
    src/asterisk/NightEventHandler.cpp
    src/asterisk/ParkEventHandler.cpp
    src/button_state/ButtonPlan.cpp
    src/button_state/PhoneButton.cpp
    src/cache/DeskphoneCache.cpp
    src/phonebook/Adapter.cpp
    src/phonebook/HTTPPhonebook.cpp
    src/phonebook/adapter/PjsipWizardAdapter.cpp
    src/ui/PhoneEventDispatcher.cpp
    src/ConfigFileParser.cpp
    src/ui/HttpNightButton.cpp
    src/ui/HttpParkButton.cpp
    src/ui/PhoneUi.cpp
    src/ui/PhoneUiState.cpp
    src/xml/yealink/CallParkMenu.cpp
    src/xml/yealink/HttpNightButton.cpp
    src/xml/yealink/PhoneUi.cpp
    src/xml/yealink/XmlPhonebook.cpp
    src/main.cpp
)


