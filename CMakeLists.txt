cmake_minimum_required(VERSION 3.14)
project (io-las VERSION 0.0.1)

add_definitions(-DGF_PLUGIN_NAME=\"${CMAKE_PROJECT_NAME}\")

find_package(geoflow REQUIRED)

# LASlib
find_package(LASlib REQUIRED)

add_library(gfp_las MODULE
  nodes.cpp
  plugin.cpp
)
target_link_libraries( gfp_las PRIVATE
  geoflow-core
  LASlib
)
set_target_properties( gfp_las
  PROPERTIES CXX_STANDARD 17
)
target_compile_options( gfp_las PRIVATE
  "$<$<CONFIG:RELEASE>:-O3>"
)

install(TARGETS gfp_las DESTINATION ${geoflow_PLUGIN_FOLDER})
