include (FetchContent)

FetchContent_Declare (c++ami
    GIT_REPOSITORY https://github.com/wolf-chesty/cppami.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable (c++ami)
