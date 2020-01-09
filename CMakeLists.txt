cmake_minimum_required(VERSION 3.14)
project (io-las VERSION 0.0.1)

find_package(geoflow REQUIRED)

# LASlib
find_package(LASlib)

set(GF_PLUGIN_NAME ${PROJECT_NAME})
set(GF_PLUGIN_TARGET_NAME "gfp_las")
set(GF_PLUGIN_REGISTER ${PROJECT_SOURCE_DIR}/register.hpp)
geoflow_create_plugin(
  nodes.cpp
)
target_link_libraries( gfp_las
  geoflow-core
  LASlib
)
target_compile_options( gfp_las PUBLIC
  "$<$<CONFIG:RELEASE>:-O3>"
)