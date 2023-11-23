cmake_minimum_required(VERSION 3.12)

project(assistant)

find_package(OpenSSL REQUIRED)

include_directories(${OPENSSL_INCLUDE_DIR}/include)

add_executable(${PROJECT_NAME} assistant_demo.cpp)

target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})

set(RUN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/run_dir)

install(TARGETS ${PROJECT_NAME} DESTINATION ${RUN_DIR})

set(CONFIG_FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/configs)
set(CONFIG_FILE_TARGET_PATH ${RUN_DIR}/configs)

install(FILES ${CONFIG_FILE_PATH}/assistant.json DESTINATION ${CONFIG_FILE_TARGET_PATH})
install(FILES ${CONFIG_FILE_PATH}/client.json DESTINATION ${CONFIG_FILE_TARGET_PATH})