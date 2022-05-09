#include "nodes.hpp"

using namespace geoflow::nodes::las;

void register_nodes(geoflow::NodeRegister& node_register) {
    node_register.register_node<LASLoaderNode>("LASLoader");
    node_register.register_node<LASVecLoaderNode>("LASVecLoader");
    node_register.register_node<LASWriterNode>("LASWriter");
    node_register.register_node<LASVecWriterNode>("LASVecWriter");
    node_register.register_node<PointCloudClassSplitNode>("PointCloudClassSplit");
}