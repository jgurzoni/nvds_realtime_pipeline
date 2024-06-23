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
        // get the per class pre-cluster threshold
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

        if (dimCoords != 6){
            std::cerr << "Error: Number of coordinates should be 6" << std::endl;
            return false;
        }
        
        float *data = (float*) outputLayerInfo[0].buffer;
        std::vector<NvDsInferParseObjectInfo> objects;
        NvDsInferParseObjectInfo obj;

        for(int i=0; i< dimPreds; i++){
            float x = data[i * dimCoords + 0];
            float y = data[i * dimCoords + 1];
            float w = data[i * dimCoords + 2];
            float h = data[i * dimCoords + 3];
            float left = x * x_scale_factor;
            float top = y * y_scale_factor;
            float width = (w - x) * x_scale_factor;
            float height = (h - y) * y_scale_factor;
                        
            float cls_confidence = data[i * dimCoords + 4];
            unsigned int class_id = data[i * dimCoords + 5];
            
            if (cls_confidence < threshold || class_id >= numConfiguredClasses){
                continue;
            }

            obj = {class_id, left, top, width, height, cls_confidence};
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