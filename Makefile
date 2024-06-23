################################################################################
# Copyright (c) 2020-2021, NVIDIA CORPORATION.  All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################

CUDA_VER?=12.2
ifeq ($(CUDA_VER),)
	$(error "CUDA_VER is not set")
endif

APP:= realtime_pipeline

TARGET_DEVICE = $(shell gcc -dumpmachine | cut -f1 -d -)

DS_SDK_ROOT:=/opt/nvidia/deepstream/deepstream

LIB_INSTALL_DIR?=$(DS_SDK_ROOT)/lib/

SRCS:= $(shell find src -name '*.cpp')  # Recursively find all cpp files

INCS:= $(shell find src -name '*.h')  # Recursively find all header files

PKGS:= gstreamer-1.0

OBJS:= $(SRCS:%.cpp=%.o)

CFLAGS = -Wall -I$(DS_SDK_ROOT)/sources/includes \
         -I /usr/local/cuda-$(CUDA_VER)/include \
         $(shell find src -type d -exec echo -I{} \;)  # Automatically add all subdirectories

CFLAGS+= `pkg-config --cflags $(PKGS)`

LIBS:= `pkg-config --libs $(PKGS)`

CC:= g++

LIBS+= -L$(LIB_INSTALL_DIR) -lnvdsgst_meta -lnvds_meta \
 -L/usr/local/cuda-$(CUDA_VER)/lib64/ -lcudart \
 -lcuda -Wl,-rpath,$(LIB_INSTALL_DIR)

DEBUG_DIR = debug
RELEASE_DIR = release
DEBUG_TARGET = $(DEBUG_DIR)/$(APP)
RELEASE_TARGET = $(RELEASE_DIR)/$(APP)

all: release

debug: CFLAGS += -g
debug: $(DEBUG_TARGET)

release: CFLAGS += -O2
release: $(RELEASE_TARGET)

$(DEBUG_TARGET): $(addprefix $(DEBUG_DIR)/, $(OBJS))
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)

$(RELEASE_TARGET): $(addprefix $(RELEASE_DIR)/, $(OBJS))
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)

$(DEBUG_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(RELEASE_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf $(DEBUG_DIR) $(RELEASE_DIR)

.PHONY: all clean debug release
