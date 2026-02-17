add_library(database
        Database.cpp
        Database.hpp
)

target_include_directories(database
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)