// This file is part of gfp-las
// Copyright (C) 2018-2022 Ravi Peters

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
void getOgcWkt(LASheader* lasheader, std::string& wkt) {
  for (int i = 0; i < (int)lasheader->number_of_variable_length_records; i++)
  {
      if (lasheader->vlrs[i].record_id == 2111) // OGC MATH TRANSFORM WKT
      {
        std::cout << "Found and ignored: OGC MATH TRANSFORM WKT\n";
      }
      else if (lasheader->vlrs[i].record_id == 2112) // OGC COORDINATE SYSTEM WKT
      {
        std::cout << "Found: OGC COORDINATE SYSTEM WKT\n";
        wkt = (char *)(lasheader->vlrs[i].data);
      }
      else if (lasheader->vlrs[i].record_id == 34735) // GeoKeyDirectoryTag
      {
        std::cout << "Found and ignored: GeoKeyDirectoryTag\n";
      }
  }

  for (int i = 0; i < (int)lasheader->number_of_extended_variable_length_records; i++)
  {
    if (strcmp(lasheader->evlrs[i].user_id, "LASF_Projection") == 0)
    {
      if (lasheader->evlrs[i].record_id == 2111) // OGC MATH TRANSFORM WKT
      {
        std::cout << "Found and ignored: OGC MATH TRANSFORM WKT\n";

      }
      else if (lasheader->evlrs[i].record_id == 2112) // OGC COORDINATE SYSTEM WKT
      {
        std::cout << "Found: OGC COORDINATE SYSTEM WKT\n";
        wkt = (char *)(lasheader->evlrs[i].data);
      }
    }
  }
  std::cout << wkt << std::endl;
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

  std::cout << "\nAttemting to find OGC CRS WKT... \n";
  std::string wkt;
  getOgcWkt(&lasreader->header, wkt);
  manager.set_fwd_crs_transform(wkt.c_str());

  // geometry.bounding_box.set(
  //   {float(lasreader->get_min_x()), float(lasreader->get_min_y()), float(lasreader->get_min_z())},
  //   {float(lasreader->get_max_x()), float(lasreader->get_max_y()), float(lasreader->get_max_z())}
  // );

  size_t i=0;
  while (lasreader->read_point()) {
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
    points.push_back(
      manager.coord_transform_fwd(
        lasreader->point.get_x(), 
        lasreader->point.get_y(), 
        lasreader->point.get_z()
      )
    );
    order.push_back(float(i)/1000);
  }
  manager.clear_fwd_crs_transform();
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

  PointCollection points;
  for(auto& lasfile : lasfiles) {
    LASreadOpener lasreadopener;
    lasreadopener.set_file_name(lasfile.c_str());
    LASreader* lasreader = lasreadopener.open();
    if (!lasreader)
      return;
    
    std::cout << "\nAttemting to find OGC CRS WKT... \n";
    std::string wkt;
    getOgcWkt(&lasreader->header, wkt);
    manager.set_fwd_crs_transform(wkt.c_str());

    size_t i=0;
    while (lasreader->read_point()) {
      if (do_class_filter && lasreader->point.get_classification() != filter_class) {
        continue;
      }
      if (thin_nth != 0) {
        if (i % thin_nth != 0) {
          continue;
        }
      }
      points.push_back(
        manager.coord_transform_fwd(
          lasreader->point.get_x(), 
          lasreader->point.get_y(), 
          lasreader->point.get_z()
        )
      );
    }
    manager.clear_fwd_crs_transform();
    lasreader->close();
    delete lasreader;
    if (!merge_output) {
      point_clouds.push_back(points);
      points.clear();
    }
  }
  if (merge_output) point_clouds.push_back(points);
}

void write_point_cloud_collection(const PointCollection& point_cloud, std::string path, NodeManager& manager) {
  LASwriteOpener laswriteopener;
  laswriteopener.set_file_name(path.c_str());

  LASheader lasheader;
  lasheader.x_scale_factor = 0.01;
  lasheader.y_scale_factor = 0.01;
  lasheader.z_scale_factor = 0.01;
  lasheader.x_offset = 0.0;
  lasheader.y_offset = 0.0;
  lasheader.z_offset = 0.0;

  // lasheader.version_major = 1;
  // lasheader.version_minor = 4;
  // lasheader.header_size = 375;
  lasheader.point_data_format = 0;
  lasheader.point_data_record_length = 20;
  
  auto crs_wkt = manager.get_rev_crs_wkt();
  // std::cout << crs_wkt << std::endl;
  // std::cout << crs_wkt.size() << std::endl;
  // std::cout << strlen(crs_wkt.c_str()) << std::endl;
  lasheader.set_geo_ogc_wkt(crs_wkt.size(), crs_wkt.c_str());
  // lasheader.set_global_encoding_bit(LAS_TOOLS_GLOBAL_ENCODING_BIT_OGC_WKT_CRS);

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
  for (auto& p_ : point_cloud) {
    auto p = manager.coord_transform_rev(p_);
    laspoint.set_x(p[0]);
    laspoint.set_y(p[1]);
    laspoint.set_z(p[2]);
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

  auto crs = manager.substitute_globals(output_crs);
  manager.set_rev_crs_transform(crs.c_str(), true);

  // get attributevalue for filename
  const gfSingleFeatureOutputTerminal* id_term;

  auto fname = manager.substitute_globals(filepath);
  
  fs::create_directories(fs::path(fname).parent_path());
  write_point_cloud_collection(point_cloud, fname, manager);
}

void LASVecWriterNode::process(){

  auto point_clouds = vector_input("point_clouds");
  auto crs = manager.substitute_globals(output_crs);
  manager.set_rev_crs_transform(crs.c_str(), true);

  for (size_t i=0; i<point_clouds.size(); ++i) {
    std::stringstream filename;
    filename << filepath << "." << std::setw(9) << std::setfill('0') <<i+1 << ".las";
    write_point_cloud_collection(
      point_clouds.get<PointCollection>(i), 
      filename.str(), 
      manager
    );
  }
}

void PointCloudClassSplitNode::process(){

  auto& point_cloud = vector_input("point_cloud").get<PointCollection>();

  auto classification = point_cloud.get_attribute_vec1i("classification");
  // auto intensity = point_cloud.get_attribute_vec1f("intensity");
  // auto colors = point_cloud.get_attribute_vec3f("colors");

  // todo throw warnings
  if (classification) {
    if (classification->size() != point_cloud.size()) {
      classification = nullptr;
    }
  }
  if (!classification) return;

  PointCollection cloud_A, cloud_B, cloud_C, cloud_D;

  for (size_t i=0; i<point_cloud.size(); ++i) {
    
    if( class_A_ == (*classification)[i] ) {
      cloud_A.push_back(point_cloud[i]);
    } else if( class_B_ == (*classification)[i] ) {
        cloud_B.push_back(point_cloud[i]);
    } else if( class_C_ == (*classification)[i] ) {
        cloud_C.push_back(point_cloud[i]);
    } else if( class_D_ == (*classification)[i] ) {
        cloud_D.push_back(point_cloud[i]);
    }

  }

  output("A").set(cloud_A);
  output("B").set(cloud_B);
  output("C").set(cloud_C);
  output("D").set(cloud_D);
}

// void PointCloudStatsCalcNode::process(float& ground_percentile) {
//   // Compute average elevation per polygon
//   std::cout <<"Computing the average elevation per polygon..." << std::endl;
//     auto& gpt = input("point_cloud").get<PointCollection&>();
//     float ground_ele = min_ground_elevation;
//     if (gpt.size()!=0) {
//       std::sort(gpt.begin(), gpt.end(), [](auto& p1, auto& p2) {
//         return p1[2] < p2[2];
//       });
//       int elevation_id = std::floor(percentile_*float(gpt.size()-1));
//       ground_ele = gpt[elevation_id][2];
//     } else {
//       std::cout << "no ground pts found for polygon\n";
//     }
//     // Assign the median ground elevation to each polygon
//     ground_elevations.push_back(ground_ele);
// }

}