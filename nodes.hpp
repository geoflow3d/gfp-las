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

      add_param("filepath", ParamPath(filepath, "File path"));
      add_param("thin_nth", ParamBoundedInt(thin_nth, 0, 100, "Thin factor"));
      add_param("filter_class", ParamBoundedInt(filter_class, 0, 100, "Filter class"));
      add_param("do_filter", ParamBool(do_class_filter, "Do class filter"));
    }
    void process();
  };

  class LASVecLoaderNode:public Node {
    std::string las_folder = "";
    public:
    using Node::Node;
    void init() {
      add_vector_output("point_clouds", typeid(PointCollection));

      add_param("las_folder", ParamPath(las_folder, "Folder with LAS files"));
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
      
      add_param("filepath", ParamPath(filepath, "File path"));
    }
    void process();
  };

  class LASVecWriterNode:public Node {
    std::string filepath = "";
    public:
    using Node::Node;
    void init() {
      add_vector_input("point_clouds", {typeid(PointCollection)});
      // add_output("classification", typeid(vec1i));
      // add_output("intensity", typeid(vec1f));
      
      add_param("filepath", ParamPath(filepath, "File path with stem"));
    }
    void process();
  };

}