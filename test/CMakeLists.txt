add_executable(climbinglog_test
        ClimbingLogTest.cpp
)

target_link_libraries(climbinglog_test
        PRIVATE
        climbinglog
)

target_include_directories(climbinglog_test
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)