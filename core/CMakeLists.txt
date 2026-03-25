include(FetchContent)

FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# Copy the nlohmann/json single-include headers to core/vendor/ so that Xcode
# can find them without needing to know the cmake-build-* directory path.
file(COPY
    ${nlohmann_json_SOURCE_DIR}/single_include/nlohmann/json.hpp
    ${nlohmann_json_SOURCE_DIR}/single_include/nlohmann/json_fwd.hpp
    DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/vendor/nlohmann)

add_library(climbinglog
        src/ClimbingLog.cpp
        src/ClimbingLog.hpp
        src/climbing_log_bridge.cpp
        include/climbing_log.h
)

target_include_directories(climbinglog
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(climbinglog
        PUBLIC
        nlohmann_json::nlohmann_json
)
