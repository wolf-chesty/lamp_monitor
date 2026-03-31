include (FetchContent)

FetchContent_Declare (inifile-cpp
    GIT_REPOSITORY https://github.com/Rookfighter/inifile-cpp.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable (inifile-cpp)