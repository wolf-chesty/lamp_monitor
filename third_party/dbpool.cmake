include (FetchContent)

FetchContent_Declare (dbpool
    GIT_REPOSITORY https://github.com/wolf-chesty/dbpool.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable (dbpool)
