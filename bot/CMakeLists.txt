cmake_minimum_required(VERSION 3.16)
project(AndrBot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

file(GLOB SRC_FILES
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/strategies/*.cpp
)

add_executable(bot
    main.cpp
    ${SRC_FILES}
)

find_package(Threads REQUIRED)
target_link_libraries(bot PRIVATE Threads::Threads)

find_package(CURL REQUIRED)
if(CURL_FOUND)
    target_include_directories(bot PRIVATE ${CURL_INCLUDE_DIRS})
    target_link_libraries(bot PRIVATE ${CURL_LIBRARIES})
else()
    message(FATAL_ERROR "libcurl not found")
endif()

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    target_include_directories(bot PRIVATE ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(bot PRIVATE OpenSSL::Crypto)
else()
    message(FATAL_ERROR "OpenSSL not found")
endif()

find_package(nlohmann_json REQUIRED)
if(nlohmann_json_FOUND)
    target_link_libraries(bot PRIVATE nlohmann_json::nlohmann_json)
endif()

if(MSVC)
    target_compile_options(bot PRIVATE /W4 /permissive-)
else()
    target_compile_options(bot PRIVATE -Wall -Wextra -pedantic)
endif()
