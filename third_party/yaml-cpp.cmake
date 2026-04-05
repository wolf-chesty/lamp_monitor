include (FetchContent)

FetchContent_Declare (yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG tags/yaml-cpp-0.9.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable (yaml-cpp)

set (YAML_BUILD_SHARED_LIBS OFF)