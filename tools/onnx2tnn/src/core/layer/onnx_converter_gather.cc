// Tencent is pleased to support the open source community by making TNN available.
//
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the 
// specific language governing permissions and limitations under the License.

#include "onnx_op_converter.h"
#include "onnx_utility.h"

DECLARE_OP_CONVERTER(Gather);

string OnnxOpConverterGather::TNNOpType(NodeProto &node,
                                           OnnxNetInfo &net_info) {
//    auto indices = get_node_attr_ai(node, "indices", net_info, 1);
//    if (indices.size() == 1) {
//        return "StridedSlice";
//    }
    return "Gather";
}

string OnnxOpConverterGather::TNNLayerParam(NodeProto &node,
                                               OnnxNetInfo &net_info) {
    const std::string &onnx_op = node.op_type();
    auto tnn_op_type = TNNOpType(node, net_info);

    int axis = INT_MAX;
    if (node_has_attr(node, "axis")) {
        axis = (int)get_node_attr_i(node, "axis");
    }
    
    auto indices = get_node_attr_ai(node, "indices", net_info, 1);

    ostringstream layer_param;
    if (tnn_op_type == "StridedSlice") {
        int dimension = 4;
        std::vector<int64_t> all_starts, all_ends, all_steps;
        for (int ii = 0; ii < axis; ii++) {
            all_starts.push_back(0);
            all_ends.push_back(0);
            all_steps.push_back(1);
        }

        all_starts.push_back(indices[0]);
        all_ends.push_back(indices[0] + 1);
        all_steps.push_back(1);

        for (int ii = axis + 1; ii < dimension; ii++) {
            all_starts.push_back(0);
            all_ends.push_back(0);
            all_steps.push_back(1);
        }

        layer_param << all_starts.size() << " ";
        for (int ii = 0; ii < all_starts.size(); ii++) {
            layer_param << all_starts[ii] << " ";
        }
        layer_param << all_ends.size() << " ";
        for (int ii = 0; ii < all_ends.size(); ii++) {
            layer_param << all_ends[ii] << " ";
        }
        layer_param << all_steps.size() << " ";
        for (int ii = 0; ii < all_steps.size(); ii++) {
            layer_param << all_steps[ii] << " ";
        }
    } else {
        layer_param << axis << " ";
        auto data_iter = net_info.weights_map.find(node.input(0));
        auto indices_iter = net_info.weights_map.find(node.input(1));
        layer_param << (data_iter == net_info.weights_map.end() ? 0 : 1) << " ";
        layer_param << (indices_iter == net_info.weights_map.end() ? 0 : 1) << " ";
    }

    return layer_param.str();
}

int OnnxOpConverterGather::WriteTNNModel(serializer *net_writer,
                                            NodeProto &node,
                                            OnnxNetInfo &net_info) {
    std::string name = !node.name().empty() ? node.name() : node.output(0);
    const std::string& tnn_layer_type = TNNOpType(node, net_info);
    
    //写头信息
    net_writer->put_int(0);  //触发type from string
    net_writer->put_string(tnn_layer_type);
    net_writer->put_string(name);
    
    //写数据
    auto data_iter = net_info.weights_map.find(node.input(0));
    auto indices_iter = net_info.weights_map.find(node.input(1));
    if (data_iter != net_info.weights_map.end()) {
        net_writer->put_int(1);
    } else {
        net_writer->put_int(0);
    }
    if (indices_iter != net_info.weights_map.end()) {
        net_writer->put_int(1);
    } else {
        net_writer->put_int(0);
    }
    
    if (data_iter != net_info.weights_map.end()) {
        WriteTensorData(data_iter->second, net_writer, net_info.data_type);
    }
    if (indices_iter != net_info.weights_map.end()) {
        WriteTensorData(indices_iter->second, net_writer, DATA_TYPE_INT32);
    }
    return 1;
}

REGISTER_OP_CONVERTER(Gather, Gather);
