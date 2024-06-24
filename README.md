# E2E Realtime Detector Application

This application is a simple end-to-end detector application using the Deepstream SDK 7.0.\
It uses a Yolov10n model to detect objects in a video stream. 

**NOTE: The application will output a video file with the detected objects instead of sink to a display.\
This is because my laptop does not have a NVIDIA GPU so I had to use a cloud-based VM to develop.**\
Sorry for the inconvenience.

## Prerequisites

Before building and running this application, ensure you have the following installed:
- a NVIDIA GPU with **CUDA** support
- **Deepstream SDK 7.0**
- **CUDA 12.2**

This application has been tested on **Ubuntu 22.04**.

## Build Instructions
The build is based on CMake and assumes the following path for the Deepstream SDK 7.0. 
Change the path accordingly in the CMakeLists.txt file.
```bash
-- CMakeLists.txt --
# Deepstream SDK 7.0 path
set(DEEPSTREAM_SDK_ROOT /opt/nvidia/deepstream/deepstream)
```

The build process will generate an executable file and a shared library in the **build** folder.\
**libyolov10n_bbox_parses.so** contains the bbox parser for the provided Yolov10n model.\
**realtime_pipeline** is the executable of the application. 

Follow these steps to build the application:

```bash
cd <appfolder>
# Create a build directory
mkdir build
# Change into the build directory
cd build
# Configure the project
cmake ..
# Build the project
make
```
## Running the Application

### Configuration Files and Models
Edit the ***conf_infer_primary_yolov10n.txt*** file to change the ONNX model and labels file path.\
If you don't provide also a model-engine file, the application will generate and save for you. It will re-use the generated model subsequently, to avoid new builds.\

```bash
-- conf_infer_primary_yolov10n.txt --
# the labelfile must be simply a list of classes, not a dict output.
labelfile-path=./models/yolov10n_classes.txt
onnx-file=./models/yolov10n.onnx
# If you have a model-engine file, you can provide it here.
# if doesn't exist, the application will generate it for you with this name.
model-engine-file=./models/yolov10n.onnx_b1_gpu0_fp32.engine
# network-mode. You can use FP32 or FP16. 
## 0=FP32, 2=FP16 mode
network-mode=0
# Custom bbox parser configuration, must be as below
parse-bbox-func-name=NvDsParseCustomBoundingBox
# by default, the path will be ./build - change accordingly if you move the file
custom-lib-path=build/libyolov10n_bbox_parser.so
```
In the same file, you can also control the threshold for the pre-cluster bbox confidence and 
the NMS threshold of the post-processing.

```bash
[class-attrs-all]
pre-cluster-threshold=0.3
nms-iou-threshold=0.2
```

### Running the Application

Run the application with the command below. It will output a .mp4 file with the detected objects.
```bash
#go back to the app folder (cd .. if you are in the build folder)
#execute the application
./build/realtime_pipeline <video input file name> <output file name>
```