#include "nodes.hpp"

using namespace geoflow::nodes::las;

void register_nodes(geoflow::NodeRegister& node_register) {
    node_register.register_node<LASLoaderNode>("LASLoader");
    node_register.register_node<LASWriterNode>("LASWriter");
}