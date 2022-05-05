#include <lasreader.hpp>
#include <laswriter.hpp>

#include <iomanip>

#include "nodes.hpp"

#if defined(__cplusplus) && __cplusplus >= 201703L && defined(__has_include)
  #if __has_include(<filesystem>)
    #define GHC_USE_STD_FS
    #include <filesystem>
    namespace fs = std::filesystem;
  #endif
#endif
#ifndef GHC_USE_STD_FS
  #include <ghc/filesystem.hpp>
  namespace fs = ghc::filesystem;
#endif

namespace geoflow::nodes::las {

std::vector<std::string> split_string(const std::string& s, std::string delimiter) {
  std::vector<std::string> parts;
  size_t last = 0;
  size_t next = 0;

  while ((next = s.find(delimiter, last)) != std::string::npos) {
    parts.push_back(s.substr(last, next-last));
    last = next + 1;
  }
  parts.push_back(s.substr(last));
  return parts;
}

void LASLoaderNode::process(){

  PointCollection points;
  auto& classification = points.add_attribute_vec1i("classification");
  auto& intensity = points.add_attribute_vec1f("intensity");
  auto& colors = points.add_attribute_vec3f("colors");
  auto& order = points.add_attribute_vec1i("order");


  LASreadOpener lasreadopener;
  lasreadopener.set_file_name(manager.substitute_globals(filepath).c_str());
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
}

void LASVecLoaderNode::process(){

  auto& point_clouds = vector_output("point_clouds");

  std::vector<std::string> lasfiles;
  if(fs::is_directory(manager.substitute_globals(filepaths))) {
    if(!fs::exists(manager.substitute_globals(filepaths))) {
      return;
    }
    for(auto& p: fs::directory_iterator(manager.substitute_globals(filepaths))) {
      auto ext = p.path().extension();
      if (ext == ".las" ||
          ext == ".LAS" ||
          ext == ".laz" ||
          ext == ".LAZ")
      {
        lasfiles.push_back(p.path().string());
      }
    }
  } else {
    std::cout << "las_filepaths is not a directory, assuming a list of LAS files" << std::endl;
    for (std::string filepath : split_string(manager.substitute_globals(filepaths), " "))
    {
      if (fs::exists(filepath)) lasfiles.push_back(filepath);
      else std::cout << filepath << " does not exist" << std::endl;
    }
  }

  std::sort(lasfiles.begin(), lasfiles.end());
  bool found_offset = manager.data_offset.has_value();
  PointCollection points;
  for(auto& lasfile : lasfiles) {
    LASreadOpener lasreadopener;
    lasreadopener.set_file_name(lasfile.c_str());
    LASreader* lasreader = lasreadopener.open();
    if (!lasreader)
      return;

    size_t i=0;
    while (lasreader->read_point()) {
      if (!found_offset) {
        manager.data_offset = {lasreader->point.get_x(), lasreader->point.get_y(), lasreader->point.get_z()};
        found_offset = true;
      }
      if (do_class_filter && lasreader->point.get_classification() != filter_class) {
        continue;
      }
      if (thin_nth != 0) {
        if (i % thin_nth != 0) {
          continue;
        }
      }
      points.push_back({
        float(lasreader->point.get_x() - (*manager.data_offset)[0]), 
        float(lasreader->point.get_y() - (*manager.data_offset)[1]), 
        float(lasreader->point.get_z() - (*manager.data_offset)[2])}
      );
    }
    lasreader->close();
    delete lasreader;
    if (!merge_output) {
      point_clouds.push_back(points);
      points.clear();
    }
  }
  if (merge_output) point_clouds.push_back(points);
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

  auto classification = point_cloud.get_attribute_vec1i("classification");
  auto intensity = point_cloud.get_attribute_vec1f("intensity");
  auto colors = point_cloud.get_attribute_vec3f("colors");

  // todo throw warnings
  if (classification) {
    if (classification->size() != point_cloud.size()) {
      classification = nullptr;
    }
  }
  if (intensity) {
    if (intensity->size() != point_cloud.size()) {
      intensity = nullptr;
    }
  }
  if (colors) {
    if (colors->size() != point_cloud.size()) {
      colors = nullptr;
    }
  }

  size_t i=0;
  for (auto& p : point_cloud) {
    laspoint.set_x(p[0] + offset[0]);
    laspoint.set_y(p[1] + offset[1]);
    laspoint.set_z(p[2] + offset[2]);
    if (classification) {
      laspoint.set_classification((*classification)[i]);
    }
    if (intensity) {
      laspoint.set_intensity((*intensity)[i]);
    }
    if (colors) {
      laspoint.set_R((*colors)[i][0] * 65535);
      laspoint.set_G((*colors)[i][1] * 65535);
      laspoint.set_B((*colors)[i][2] * 65535);
    }

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

  // get attributevalue for filename
  const gfSingleFeatureOutputTerminal* id_term;
  auto id_attr_name = manager.substitute_globals(attribute_name);
  bool use_id_from_attribute = false;
  for (auto& term : poly_input("attributes").sub_terminals()) {
    if ( term->get_name() == id_attr_name && term->accepts_type(typeid(std::string)) ) {
      id_term = term;
      use_id_from_attribute = true;
    }
  }

  auto fname = manager.substitute_globals(filepath);
  fname = substitute_from_term(fname, poly_input("attributes"));
  
  fs::create_directories(fs::path(fname).parent_path());
  write_point_cloud_collection(point_cloud, fname, *manager.data_offset);
}

void LASVecWriterNode::process(){

  auto point_clouds = vector_input("point_clouds");

  for (size_t i=0; i<point_clouds.size(); ++i) {
    std::stringstream filename;
    filename << filepath << "." << std::setw(9) << std::setfill('0') <<i+1 << ".las";
    write_point_cloud_collection(
      point_clouds.get<PointCollection>(i), 
      filename.str(), 
      *manager.data_offset
    );
  }
}

}