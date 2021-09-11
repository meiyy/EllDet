cmake_minimum_required(VERSION 3.10)

if(DEFINED ENV{VCPKG_DIR})
    set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake
  CACHE STRING "Vcpkg toolchain file")
endif()

project(EllipseDetection)
find_package(OpenCV  REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/)

file(GLOB SourceFiles "src/*.cpp")
file(GLOB HeaderFiles "src/*.h")
source_group("Header Files" FILES ${HeaderFiles}) 
source_group("Source Files" FILES ${SourceFiles}) 

add_executable(elldetect ${HeaderFiles} ${SourceFiles})
target_include_directories(elldetect  PUBLIC ${OpenCV_INCLUDE_DIRS} "src")
target_link_libraries(elldetect PUBLIC ${OpenCV_LIBS})
set_property(TARGET elldetect PROPERTY CXX_STANDARD 17)