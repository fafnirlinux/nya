cmake_minimum_required(VERSION 3.11)
project(nya)

file(GLOB SRC
     "src/*.hxx"
     "src/*.cxx"
)

find_package(CURL REQUIRED)
find_package(LibArchive REQUIRED)
include_directories(${CURL_INCLUDE_DIR})
include_directories(${LibArchive_INCLUDE_DIRS})
add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} ${CURL_LIBRARIES} ${LibArchive_LIBRARIES})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)