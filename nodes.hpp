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
    std::string attribute_name = "identificatie";
    public:
    using Node::Node;
    void init() {
      add_input("point_clouds", {typeid(PointCollection)});
      // add_output("classification", typeid(vec1i));
      // add_output("intensity", typeid(vec1f));
      add_poly_input("attributes", {typeid(bool), typeid(int), typeid(float), typeid(std::string), typeid(Date), typeid(Time), typeid(DateTime)});

      add_param(ParamString(attribute_name, "attribute_name", "attribute to use as filename. Has to be a string attribute."));
      add_param(ParamPath(filepath, "filepath", "File path"));
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
      
      add_param(ParamPath(filepath, "filepath", "File path with stem"));
    }
    void process();
  };

}