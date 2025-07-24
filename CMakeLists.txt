
cmake_minimum_required(VERSION 3.20)
add_definitions(-D_WIN32_WINNT=0x0601)

# Pour forcer /MD (runtime dynamique)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

# Indiquer le toolchain vcpkg
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "c:/source/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(resolver LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pour Boost.Asio (Boost)
#find_package(boost-asio CONFIG REQUIRED)
#find_package(Boost COMPONENTS asio system REQUIRED)
find_package(Boost REQUIRED COMPONENTS asio system)

add_executable(resolver resolver.cpp)

target_link_libraries(resolver PRIVATE Boost::asio Boost::system)
