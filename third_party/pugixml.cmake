include (FetchContent)

FetchContent_Declare (pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG tags/v1.15
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable (pugixml)
