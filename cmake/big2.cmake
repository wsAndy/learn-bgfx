include(FetchContent)

option(FETCHCONTENT_FULLY_DISCONNECTED   "Disables all attempts to download or update content and assumes source dirs already exist" ON )
option(FETCHCONTENT_UPDATES_DISCONNECTED "Enables UPDATE_DISCONNECTED behavior for all content population" ON)
option(FETCHCONTENT_QUIET                "Enables QUIET option for all content population" OFF )

file(MAKE_DIRECTORY  ${ROOT_PATH}/3rdparty)
set(FETCHCONTENT_BASE_DIR ${ROOT_PATH}/3rdparty)

fetchcontent_declare(
        big2
        GIT_REPOSITORY "https://github.com/Paper-Cranes-Ltd/big2-stack.git"
        GIT_TAG "v0.0.9"
        GIT_SHALLOW TRUE
)

fetchcontent_makeavailable(big2)