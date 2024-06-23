#include "nvdsinfer_custom_impl.h"
#include "nvdsinfer_context.h"
#include <stdio.h>
#include<iostream>
#include<algorithm>


#define VIDEO_INPUT_WIDTH 640
#define VIDEO_INPUT_HEIGHT 640


extern "C" bool NvDsParseCustomBoundingBox(
    std::vector<NvDsInferLayerInfo> const& outputLayerInfo,
    NvDsInferNetworkInfo const& networkInfo,
    NvDsInferParseDetectionParams const& detectionParams,
    std::vector<NvDsInferParseObjectInfo>& objectList);


extern "C" bool NvDsParseCustomBoundingBox(
    std::vector<NvDsInferLayerInfo> const& outputLayerInfo,
    NvDsInferNetworkInfo const& networkInfo,
    NvDsInferParseDetectionParams const& detectionParams,
    std::vector<NvDsInferParseObjectInfo>& objectList)
    {
        auto numOutputLayers = outputLayerInfo.size();

        float netWidth = networkInfo.width;
        float netHeight = networkInfo.height;

        float x_scale_factor = float(VIDEO_INPUT_WIDTH / netWidth); // use your scale factor
        float y_scale_factor = float(VIDEO_INPUT_HEIGHT / netHeight); // use your scale factor

        NvDsInferLayerInfo layerInfo = outputLayerInfo[0];
        NvDsInferDims inferDims = layerInfo.inferDims;
        int numConfiguredClasses = detectionParams.numClassesConfigured;
        float threshold = detectionParams.perClassPreclusterThreshold[0];

        int dimPreds, dimCoords;
        int dimBatch = 1;
        if (inferDims.numDims == 2){
            dimPreds = inferDims.d[0];
            dimCoords = inferDims.d[1];
        }
        else{
            dimBatch = inferDims.d[0];
            dimPreds = inferDims.d[1];
            dimCoords = inferDims.d[2];
        }

        std::vector<NvDsInferParseObjectInfo> parsedDetections;
        
        float *data = (float*) outputLayerInfo[0].buffer;
        std::vector<NvDsInferParseObjectInfo> objects;
        NvDsInferParseObjectInfo obj;
        for(int i=0; i< dimPreds; i++){
            float x = data[i * 6 + 0]; // Access x
            float y = data[i * 6 + 1]; // Access y
            float w = data[i * 6 + 2]; // Access w
            float h = data[i * 6 + 3]; // Access h
            float left = x * x_scale_factor;
            float top = y * y_scale_factor;
            float width = (w - x) * x_scale_factor;
            float height = (h - y) * y_scale_factor;
                        
            std::vector<float> cls_confidences;
            for (int j=0; j < numConfiguredClasses; j++){
                cls_confidences.push_back(data[i * 6 + 4 + j]);
            }

            unsigned int maxClsIdx = std::max_element(cls_confidences.begin(), cls_confidences.end()) - cls_confidences.begin();
            float maxClsConfidence = cls_confidences[maxClsIdx]/100.f; //condidence is coming 0-100
            
            if (maxClsConfidence < threshold){
                continue;
            }

            obj = {maxClsIdx, left, top, width, height, maxClsConfidence};
            objects.push_back(obj);
        }
        objectList = objects;
        /*
        //print all objectlist - debug purpose
        std::cout << "Object List" << std::endl;
        for (int i = 0; i < objectList.size(); i++){
            std::cout << "Object " << i << " : " << objectList[i].classId << " " << \
            objectList[i].left << " " << objectList[i].top << " " << objectList[i].width << " " << \
            objectList[i].height << " " << objectList[i].detectionConfidence << std::endl;
        } */
        return true;
    }