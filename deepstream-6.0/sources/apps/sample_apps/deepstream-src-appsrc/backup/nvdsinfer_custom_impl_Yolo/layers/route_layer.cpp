/*
 * Created by Marcos Luciano
 * https://www.github.com/marcoslucianops
 */

#include "route_layer.h"

nvinfer1::ITensor* routeLayer(
    int layerIdx,
    std::string& layers,
    std::map<std::string, std::string>& block,
    std::vector<nvinfer1::ITensor*> tensorOutputs,
    nvinfer1::INetworkDefinition* network)
{
    nvinfer1::ITensor* output;

    assert(block.at("type") == "route");
    assert(block.find("layers") != block.end());

    std::string strLayers = block.at("layers");
    std::vector<int> idxLayers;
    size_t lastPos = 0, pos = 0;
    while ((pos = strLayers.find(',', lastPos)) != std::string::npos)
    {
        int vL = std::stoi(trim(strLayers.substr(lastPos, pos - lastPos)));
        idxLayers.push_back(vL);
        lastPos = pos + 1;
    }
    if (lastPos < strLayers.length())
    {
        std::string lastV = trim(strLayers.substr(lastPos));
        if (!lastV.empty())
            idxLayers.push_back(std::stoi(lastV));
    }
    assert (!idxLayers.empty());
    std::vector<nvinfer1::ITensor*> concatInputs;
    for (uint i = 0; i < idxLayers.size(); ++i)
    {
        if (idxLayers[i] < 0)
            idxLayers[i] = tensorOutputs.size() + idxLayers[i];
        assert (idxLayers[i] >= 0 && idxLayers[i] < (int)tensorOutputs.size());
        concatInputs.push_back(tensorOutputs[idxLayers[i]]);
        if (i < idxLayers.size() - 1)
            layers += std::to_string(idxLayers[i]) + ", ";
    }
    layers += std::to_string(idxLayers[idxLayers.size() - 1]);

    if (concatInputs.size() == 1)
        return concatInputs[0];

    int axis = 0;
    if (block.find("axis") != block.end())
        axis = std::stoi(block.at("axis"));
    if (axis < 0)
        axis = concatInputs[0]->getDimensions().nbDims + axis;

    nvinfer1::IConcatenationLayer* concat = network->addConcatenation(concatInputs.data(), concatInputs.size());
    assert(concat != nullptr);
    std::string concatLayerName = "route_" + std::to_string(layerIdx);
    concat->setName(concatLayerName.c_str());
    concat->setAxis(axis);
    output = concat->getOutput(0);

    if (block.find("groups") != block.end())
    {
        nvinfer1::Dims prevTensorDims = output->getDimensions();
        int groups = stoi(block.at("groups"));
        int group_id = stoi(block.at("group_id"));
        int startSlice = (prevTensorDims.d[0] / groups) * group_id;
        int channelSlice = (prevTensorDims.d[0] / groups);
        nvinfer1::ISliceLayer* slice = network->addSlice(
            *output, nvinfer1::Dims{3, {startSlice, 0, 0}},
            nvinfer1::Dims{3, {channelSlice, prevTensorDims.d[1], prevTensorDims.d[2]}}, nvinfer1::Dims{3, {1, 1, 1}});
        assert(slice != nullptr);
        std::string sliceLayerName = "slice_" + std::to_string(layerIdx);
        slice->setName(sliceLayerName.c_str());
        output = slice->getOutput(0);
    }

    return output;
}
