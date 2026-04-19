include (FetchContent)

FetchContent_Declare (yaml-schema-cpp
    GIT_REPOSITORY https://github.com/joanvallve/yaml-schema-cpp.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable (yaml-schema-cpp)
