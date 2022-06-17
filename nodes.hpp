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
#include <geoflow/geoflow.hpp>

namespace geoflow::nodes::las {

  class LASLoaderNode:public Node {
    std::string filepath = "";
    int thin_nth=5;
    int filter_class = 6;
    bool do_class_filter = true;
    public:
    using Node::Node;
    void init() {
      add_output("points", typeid(PointCollection));
      add_output("classification", typeid(vec1i));
      add_output("intensity", typeid(vec1f));
      add_output("order", typeid(vec1f));
      add_output("colors", typeid(vec3f));

      add_param(ParamPath(filepath, "filepath", "File path"));
      add_param(ParamBoundedInt(thin_nth, 0, 100, "thin_nth", "Thin factor"));
      add_param(ParamBoundedInt(filter_class, 0, 100, "filter_class", "Filter class"));
      add_param(ParamBool(do_class_filter, "do_filter", "Do class filter"));
    }
    void process();
  };

  class LASVecLoaderNode:public Node {
    std::string filepaths = "";
    int thin_nth=5;
    int filter_class = 6;
    bool do_class_filter = true;
    bool merge_output = false;
    public:
    using Node::Node;
    void init() {
      add_vector_output("point_clouds", typeid(PointCollection));

      add_param(ParamPath(filepaths, "las_filepaths", "Folder with LAS files, OR a space separated list of LAS files"));
      add_param(ParamBoundedInt(thin_nth, 0, 100, "thin_nth", "Thin factor"));
      add_param(ParamBoundedInt(filter_class, 0, 100, "filter_class", "Filter class"));
      add_param(ParamBool(do_class_filter, "do_filter", "Do class filter"));
      add_param(ParamBool(merge_output, "merge_output", "Merge the input files into a single output. If true, the output vector will have only a single element which is the merged point cloud."));
    }
    void process();
  };

  class LASWriterNode:public Node {
    std::string filepath = "";
    public:
    using Node::Node;
    void init() {
      add_input("point_clouds", {typeid(PointCollection)});
      // add_output("classification", typeid(vec1i));
      // add_output("intensity", typeid(vec1f));
      // add_poly_input("attributes", {typeid(bool), typeid(int), typeid(float), typeid(std::string), typeid(Date), typeid(Time), typeid(DateTime)});

      add_param(ParamPath(filepath, "filepath", "File path"));
    }
    void process();
    bool parameters_valid() override {
      if (manager.substitute_globals(filepath).empty()) 
        return false;
      else 
        return true;
    }
  };

  class LASVecWriterNode:public Node {
    std::string filepath = "";
    public:
    using Node::Node;
    void init() {
      add_vector_input("point_clouds", {typeid(PointCollection)});
      // add_output("classification", typeid(vec1i));
      // add_output("intensity", typeid(vec1f));
      
      add_param(ParamPath(filepath, "filepath", "File path with stem"));
    }
    void process();
    bool parameters_valid() override {
      if (manager.substitute_globals(filepath).empty()) 
        return false;
      else 
        return true;
    }
  };

  class PointCloudClassSplitNode:public Node {
    int class_A_ = 2;
    int class_B_ = 6;
    int class_C_ = 0;
    int class_D_ = 1;
    public:
    using Node::Node;
    void init() {
      add_input("point_cloud", {typeid(PointCollection)});
      add_output("A", typeid(PointCollection));
      add_output("B", typeid(PointCollection));
      add_output("C", typeid(PointCollection));
      add_output("D", typeid(PointCollection));
      // add_output("intensity", typeid(vec1f));
      
      add_param(ParamInt(class_A_, "class A", "Classification code for output A"));
      add_param(ParamInt(class_B_, "class B", "Classification code for output A"));
      add_param(ParamInt(class_C_, "class C", "Classification code for output A"));
      add_param(ParamInt(class_D_, "class D", "Classification code for output A"));
    }
    void process();
  };

  // class PointCloudStatsCalcNode:public Node {
  //   float percentile_=0.05;

  //   public:
  //   using Node::Node;
  //   void init() {
  //     add_input("point_cloud", {typeid(PointCollection)});
  //     add_output("percentile_z", typeid(float));

  //     add_param(ParamBoundedFloat(ground_percentile, 0, 1, "ground_percentile",  "Ground elevation percentile"));
  //   }
  //   void process();
  // };

}