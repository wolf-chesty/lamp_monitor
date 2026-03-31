include (FetchContent)

FetchContent_Declare (argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG 3eda91b2e1ce7d569f84ba295507c4cd8fd96910
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable (argparse)
