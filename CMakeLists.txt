cmake_minimum_required(VERSION 3.5)

option(BUILD_PI "Build for Raspberry PI" OFF)

message(STATUS "###############")
message(STATUS "Architecture PI: ${BUILD_PI}")

#set(CMAKE_TOOLCHAIN_FILE "/mnt/hd0/Distrib/Projects/azad/build/x586.cmake")

project(azad)

set(CMAKE_BUILD_TYPE Debug)

string(TIMESTAMP BUILDDATE "%Y%m%d")
string(TIMESTAMP BUILDTIME "%H%M%S")

set("${PROJECT_NAME}_VERSION_MAJOR" "10")
set("${PROJECT_NAME}_VERSION_MINOR" "1100")
set("${PROJECT_NAME}_VERSION_PATCH" ${BUILDDATE})
set("${PROJECT_NAME}_VERSION_TWEAK" ${BUILDTIME})
set("${PROJECT_NAME}_VERSION" "${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_VERSION_PATCH}.${${PROJECT_NAME}_VERSION_TWEAK}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")

file(GLOB C_FILES "src/*.c")
if(BUILD_PI)    	
   		list(FILTER C_FILES EXCLUDE REGEX ".*/dummy.c$")
endif()

add_executable(${PROJECT_NAME} ${C_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/src/include")
target_include_directories(${PROJECT_NAME} PUBLIC "/opt/vc/include")
target_include_directories(${PROJECT_NAME} PUBLIC "/opt/vc/include/interface/vcos/pthreads")
target_include_directories(${PROJECT_NAME} PUBLIC "/opt/vc/include/interface/vmcs_host/linux")

target_link_libraries(${PROJECT_NAME} avformat avcodec asound avutil m crypto swresample z pthread ssl curl udev) 

if(BUILD_PI)
   		target_link_libraries(${PROJECT_NAME} avformat avcodec asound avutil m crypto swresample z pthread ssl curl udev)
endif()


target_compile_definitions(${PROJECT_NAME} PUBLIC "-D_GNU_SOURCE")
target_compile_definitions(${PROJECT_NAME} PUBLIC "-DOMX_SKIP64BIT")
target_compile_definitions(${PROJECT_NAME} PUBLIC -DVERSION="${${PROJECT_NAME}_VERSION}")

message(STATUS "Build ${PROJECT_NAME} version ${${PROJECT_NAME}_VERSION}")
