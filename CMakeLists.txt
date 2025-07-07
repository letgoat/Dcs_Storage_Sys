cmake_minimum_required (VERSION 3.10)

project(SkipListProject)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 包含头文件目录
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}
)

# 查找必要的包
find_package(Threads REQUIRED)

# 收集所有源目录
file(GLOB_RECURSE SOURCES
    "node/*.cpp"
    "skiplist/*.cpp"
    "test/*.cpp"
    "network/*.cpp"
    "server/*.cpp"
    "logger/*.cpp"
    "config/*.cpp"
    "utils/*.cpp"
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接库
target_link_libraries(${PROJECT_NAME} 
    PUBLIC 
    Threads::Threads
    stdc++fs
)

# 设置编译选项
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -O2)

# 创建store目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/store
)
