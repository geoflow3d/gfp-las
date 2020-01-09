#include <lasreader.hpp>
#include <laswriter.hpp>

#include "nodes.hpp"

namespace geoflow::nodes::las {

void LASLoaderNode::process(){

  PointCollection points;
  vec1i classification;
  vec1f intensity;
  vec1f order;
  vec3f colors;

  LASreadOpener lasreadopener;
  lasreadopener.set_file_name(filepath.c_str());
  LASreader* lasreader = lasreadopener.open();
  if (!lasreader)
    return;

  // geometry.bounding_box.set(
  //   {float(lasreader->get_min_x()), float(lasreader->get_min_y()), float(lasreader->get_min_z())},
  //   {float(lasreader->get_max_x()), float(lasreader->get_max_y()), float(lasreader->get_max_z())}
  // );
  bool found_offset = manager.data_offset.has_value();

  size_t i=0;
  while (lasreader->read_point()) {
    if (!found_offset) {
      manager.data_offset = {lasreader->point.get_x(), lasreader->point.get_y(), lasreader->point.get_z()};
      found_offset = true;
    }
    ++i;
    if (do_class_filter && lasreader->point.get_classification() != filter_class) {
      continue;
    }
    // if (i % 1000000 == 0) {
    //   std::cout << "Read " << i << " points...\n";
    // }
    if (thin_nth != 0) {
      if (i % thin_nth != 0) {
        continue;
      }
    }
    classification.push_back(lasreader->point.get_classification());
    intensity.push_back(float(lasreader->point.get_intensity()));
    colors.push_back({
      float(lasreader->point.get_R())/65535,
      float(lasreader->point.get_G())/65535,
      float(lasreader->point.get_B())/65535
    });
    points.push_back({
      float(lasreader->point.get_x() - (*manager.data_offset)[0]), 
      float(lasreader->point.get_y() - (*manager.data_offset)[1]), 
      float(lasreader->point.get_z() - (*manager.data_offset)[2])}
    );
    order.push_back(float(i)/1000);
  }
  lasreader->close();
  delete lasreader;

  output("points").set(points);
  output("colors").set(colors);
  output("classification").set(classification);
  output("intensity").set(intensity);
  output("order").set(order);
}

void write_point_cloud_collection(const PointCollection& point_cloud, std::string path, const std::array<double,3> offset) {
  LASwriteOpener laswriteopener;
  laswriteopener.set_file_name(path.c_str());

  LASheader lasheader;
  lasheader.x_scale_factor = 0.01;
  lasheader.y_scale_factor = 0.01;
  lasheader.z_scale_factor = 0.01;
  lasheader.x_offset = 0.0;
  lasheader.y_offset = 0.0;
  lasheader.z_offset = 0.0;
  lasheader.point_data_format = 0;
  lasheader.point_data_record_length = 20;

  LASpoint laspoint;
  laspoint.init(&lasheader, lasheader.point_data_format, lasheader.point_data_record_length, 0);

  LASwriter* laswriter = laswriteopener.open(&lasheader);
  if (laswriter == 0)
  {
    std::cerr << "ERROR: could not open laswriter\n";
    return;
  }

  // bool found_offset = manager.data_offset.has_value();

  size_t i=0;
  for (auto& p : point_cloud) {
    laspoint.set_x(p[0] + offset[0]);
    laspoint.set_y(p[1] + offset[1]);
    laspoint.set_z(p[2] + offset[2]);

    laswriter->write_point(&laspoint);
    laswriter->update_inventory(&laspoint);
    
    if((++i)%100000000==0) std::cout << "Wrtten " << i << " points...\n";
  } 

  laswriter->update_header(&lasheader, TRUE);
  laswriter->close();
  delete laswriter;
}

void LASWriterNode::process(){

  auto input_geom = input("point_clouds");

  auto point_cloud = input_geom.get<PointCollection>();
  write_point_cloud_collection(point_cloud, filepath, *manager.data_offset);
}

void LASVecWriterNode::process(){

  auto point_clouds = vector_input("point_clouds");

  for (size_t i=0; i<point_clouds.size(); ++i) {
    write_point_cloud_collection(
      point_clouds.get<PointCollection>(i), 
      filepath+"." + std::to_string(i+1) + ".las", 
      *manager.data_offset
    );
  }
}

}