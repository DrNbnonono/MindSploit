cmake_minimum_required(VERSION 3.20)
project(MindSploit VERSION 2.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt设置
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_PREFIX_PATH "E:/Qt/6.9.1/mingw_64") # Qt Kit Dir
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

# 编译选项
if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 查找依赖库
find_package(Qt6 REQUIRED COMPONENTS Core Network Sql)
find_package(Threads REQUIRED)

# 源文件
set(SOURCES
    src/main.cpp
    src/core/terminal_interface.cpp
    src/core/engine_manager.cpp
    src/core/session_manager.cpp
    src/core/command_parser.cpp
    src/engines/network/network_engine.cpp
    src/ai/ai_manager.cpp
    src/utils/network_utils.cpp
    src/core/database.cpp
    src/core/config_manager.cpp
)

# 头文件
set(HEADERS
    src/core/terminal_interface.h
    src/core/engine_manager.h
    src/core/session_manager.h
    src/core/command_parser.h
    src/engines/engine_interface.h
    src/engines/network/network_engine.h
    src/ai/ai_manager.h
    src/utils/network_utils.h
    src/core/database.h
    src/core/config_manager.h
)

# 创建可执行文件 (控制台应用)
add_executable(MindSploit ${SOURCES} ${HEADERS})

# 链接库
target_link_libraries(MindSploit
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    Threads::Threads
)

# Windows特定链接
if(WIN32)
    target_link_libraries(MindSploit ws2_32 iphlpapi)
endif()

# 设置包含目录
target_include_directories(MindSploit PRIVATE src)

# 编译定义
target_compile_definitions(MindSploit PRIVATE
    MINDSPLOIT_VERSION="${PROJECT_VERSION}"
    QT_NO_KEYWORDS
)