add_executable(database_test
        DatabaseTest.cpp
)

target_link_libraries(database_test
        PRIVATE
        database
)

target_include_directories(database_test
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)