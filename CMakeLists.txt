cmake_minimum_required(VERSION 3.14)
project (io-las VERSION 0.0.1)

if(PROJECT_IS_TOP_LEVEL)
  find_package(geoflow REQUIRED)
  include_directories(${geoflow_INCLUDE_DIRS})
endif()

# LASlib
message("CMAKE MP: ${CMAKE_MODULE_PATH}")
find_package(laslib CONFIG REQUIRED)

# ghc filesystem
if(EXISTS "${PROJECT_SOURCE_DIR}/.gitmodules")
  execute_process(
    COMMAND             git submodule update --init filesystem
    WORKING_DIRECTORY   ${PROJECT_SOURCE_DIR}/thirdparty
    )
endif() 

set(GF_PLUGIN_NAME ${PROJECT_NAME})
set(GF_PLUGIN_TARGET_NAME "gfp_las")
set(GF_PLUGIN_REGISTER ${PROJECT_SOURCE_DIR}/register.hpp)
geoflow_create_plugin(
  nodes.cpp
)
target_include_directories(gfp_las PRIVATE thirdparty/filesystem/include)
target_link_libraries( gfp_las PRIVATE
  geoflow-core
  LASlib
)
target_compile_options( gfp_las PUBLIC
  "$<$<CONFIG:RELEASE>:-O3>"
)
if(MSVC)
  add_definitions(-DNOMINMAX)

  # collect dll's required for runtime
  INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Release/ DESTINATION bin
    FILES_MATCHING 
    PATTERN "*.dll"
    PATTERN "gfp*" EXCLUDE)
endif()
