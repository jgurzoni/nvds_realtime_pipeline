/*
 * Copyright (c) 2020-2023, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gst/gst.h>
#include <glib.h>
#include <stdio.h>
#include "gstnvdsmeta.h"
#include <cuda_runtime_api.h>

#include <functional>
#include <iostream>
#include "pipeline.h"
//#include "bus_call.h"

#define MAX_DISPLAY_LEN 64

#define PGIE_CLASS_ID_PERSON 0

/* Muxer batch formation timeout, for e.g. 40 millisec. Should ideally be set
 * based on the fastest source's framerate. */
#define MUXER_BATCH_TIMEOUT_USEC 40000

#define PRIMARY_DETECTOR_UID 1


gboolean
bus_call (GstBus * bus, GstMessage * msg, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;
  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_ERROR:{
      gchar *debug;
      GError *error;
      gst_message_parse_error (msg, &error, &debug);
      g_printerr ("ERROR from element %s: %s\n",
          GST_OBJECT_NAME (msg->src), error->message);
      if (debug)
        g_printerr ("Error details: %s\n", debug);
      g_free (debug);
      g_error_free (error);
      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }
  return TRUE;
}

// Primary constructor
Pipeline::Pipeline(const char* input_stream){
    this->input_stream = (gchar*)input_stream;
  }

// Destructor
Pipeline::~Pipeline(){
    /* clean up nicely */
    gst_element_set_state (pipeline, GST_STATE_NULL);
    g_print ("Deleting pipeline\n");
    gst_object_unref (GST_OBJECT (pipeline));
    g_source_remove (bus_watch_id);
    g_main_loop_unref (loop);
  }

int Pipeline::Init(){
  // Initialize the class, setting up the pipeline. 
  //Must be called before running the pipeline
    int rslt = create_elements();
    if (rslt == 0)
      is_init = true;
    
    return rslt;
}

int Pipeline::create_elements(void){
  /* Standard GStreamer initialization */
  this->loop = g_main_loop_new (NULL, FALSE);

  /* Create GStreamer elements */
    this->pipeline = gst_pipeline_new ("pipeline");
    this->source = gst_element_factory_make ("filesrc", "file-source");
    this->h264parser = gst_element_factory_make ("h264parse", "h264-parser");
    this->decoder = gst_element_factory_make ("nvv4l2decoder", "nvv4l2-decoder");
    this->streammux = gst_element_factory_make ("nvstreammux", "stream-muxer");
    this->primary_detector = gst_element_factory_make ("nvinfer", "primary-nvinference-engine");
    this->nvvidconv = gst_element_factory_make ("nvvideoconvert", "nvvideo-converter");
    this->nvosd = gst_element_factory_make ("nvdsosd", "nv-onscreendisplay");
    //elements for the save to file sink
    this->encoder = gst_element_factory_make ("nvv4l2h264enc", "h264-encoder");
    this->parser = gst_element_factory_make ("h264parse", "h264-parser2");
    this->muxer = gst_element_factory_make ("qtmux", "qt-muxer");
    this->sink = gst_element_factory_make ("filesink", "file-sink");

    if (!pipeline || !source || !h264parser || !decoder || !streammux || !primary_detector ||
        !nvvidconv || !nvosd || !encoder || !parser || !muxer || !sink) {
        g_printerr ("One element could not be created. Exiting.\n");
        return -1;
    }

    /* Set element properties */
  
    g_object_set (G_OBJECT (streammux), "width", this->muxer_output_width, "height",
        this->muxer_output_height, "batch-size", 1, "batched-push-timeout",
        MUXER_BATCH_TIMEOUT_USEC, NULL);
    g_object_set (G_OBJECT (primary_detector), "config-file-path",
        this->infer_pgie_config_file, "unique-id", PRIMARY_DETECTOR_UID, NULL);
    g_object_set (G_OBJECT (sink), "location", "output.mp4", NULL);

  
  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  GstPad *sinkpad, *srcpad;
  gchar pad_name_sink[16] = "sink_0";
  gchar pad_name_src[16] = "src";

  sinkpad = gst_element_request_pad_simple (streammux, pad_name_sink);
  if (!sinkpad) {
    g_printerr ("Streammux request sink pad failed. Exiting.\n");
    return -1;
  }
  srcpad = gst_element_get_static_pad (decoder, pad_name_src);
  if (!srcpad) {
    g_printerr ("Decoder request src pad failed. Exiting.\n");
    return -1;
  }
  if (gst_pad_link (srcpad, sinkpad) != GST_PAD_LINK_OK) {
      g_printerr ("Failed to link decoder to stream muxer. Exiting.\n");
      return -1;
  }

  gst_object_unref (sinkpad);
  gst_object_unref (srcpad);

  /* Add all elements into the pipeline */
    gst_bin_add_many (GST_BIN (pipeline), source, h264parser, decoder, streammux,
        primary_detector, nvvidconv, nvosd, encoder, parser, muxer, sink, NULL);

    /* Link the elements together */
    if (!gst_element_link_many (source, h264parser, decoder, streammux, primary_detector,
        nvvidconv, nvosd, encoder, parser, muxer, sink, NULL)) {
        g_printerr ("Elements could not be linked. Exiting.\n");
        return -1;
    }

  /* Lets add probe to get informed of the meta data generated, we add probe to
   * the sink pad of the nvvideoconvert element, since by that time, the buffer would have
   * had got all the metadata. */
  
  nvvidconv_sink_pad = gst_element_get_static_pad (nvvidconv, "sink");
  if (!nvvidconv_sink_pad)
    g_print ("Unable to get sink pad\n");
  else{
    // Register the lambda function as a callback
    // This should resolve the need for a static probe callback function and allow us to see this within it
    gst_pad_add_probe(nvvidconv_sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
    +[](GstPad* pad, GstPadProbeInfo* info, gpointer user_data) -> GstPadProbeReturn {
        auto* self = static_cast<Pipeline*>(user_data);
        return self->nvvidconv_sink_pad_buffer_probe(pad, info, user_data);
    }, this, nullptr);


  }
  return 0;

}

void Pipeline::run(gchar* input_stream){
    if (!is_init){
      g_printerr ("Pipeline not initialized. Must run Init first.\n");
      return;
    }
    
    /* Set the input stream to the source element */
    g_object_set (G_OBJECT (source), "location", input_stream, NULL);
    
    this->frame_number = 0;
    
    /* Set the pipeline to "playing" state */
    g_print ("Now playing: %s\n", input_stream);
    gst_element_set_state (pipeline, GST_STATE_PLAYING);

    /* Wait till pipeline encounters an error or EOS */
    g_print ("Running...\n");
    g_main_loop_run (loop);
    g_print ("Returned, stopping playback\n");
    // Stop pipeline
    gst_element_set_state (pipeline, GST_STATE_NULL);
    // Wait for state change to complete
    gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
  }

GstPadProbeReturn
Pipeline::nvvidconv_sink_pad_buffer_probe (GstPad * pad, GstPadProbeInfo * info,
    gpointer u_data){
    // This is the callback function for the probe
    // This is where we will add the logic to count the number of people and other classes
    // and display the count on the screen

    GstBuffer *buf = (GstBuffer *) info->data;
    NvDsObjectMeta *obj_meta = NULL;
    guint other_count = 0;
    guint person_count = 0;
    NvDsMetaList * l_frame = NULL;
    NvDsMetaList * l_obj = NULL;
    NvDsDisplayMeta *display_meta = NULL;

    NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta (buf);

    for (l_frame = batch_meta->frame_meta_list; l_frame != NULL;
      l_frame = l_frame->next) {
        NvDsFrameMeta *frame_meta = (NvDsFrameMeta *) (l_frame->data);
        int offset = 0;
        for (l_obj = frame_meta->obj_meta_list; l_obj != NULL;
                l_obj = l_obj->next) {
            obj_meta = (NvDsObjectMeta *) (l_obj->data);
            
            /* Check that the class id is that of persons. */
            if (obj_meta->class_id == PGIE_CLASS_ID_PERSON){
              person_count++;
            }else{
              other_count++;
            }

        }
        display_meta = nvds_acquire_display_meta_from_pool(batch_meta);
        NvOSD_TextParams *txt_params  = &display_meta->text_params[0];
        display_meta->num_labels = 1;
        txt_params->display_text = static_cast<char*>(g_malloc0(MAX_DISPLAY_LEN));
        offset = snprintf(txt_params->display_text, MAX_DISPLAY_LEN, "Person = %d ", person_count);
        offset += snprintf(txt_params->display_text + offset , MAX_DISPLAY_LEN, "Noise Classes = %d ", other_count);

        /* Now set the offsets where the string should appear */
        txt_params->x_offset = 10;
        txt_params->y_offset = 12;

        /* Font , font-color and font-size */
        txt_params->font_params.font_name = const_cast<char*>("Serif");
        txt_params->font_params.font_size = 10;
        txt_params->font_params.font_color.red = 1.0;
        txt_params->font_params.font_color.green = 1.0;
        txt_params->font_params.font_color.blue = 1.0;
        txt_params->font_params.font_color.alpha = 1.0;

        /* Text background color */
        txt_params->set_bg_clr = 1;
        txt_params->text_bg_clr.red = 0.0;
        txt_params->text_bg_clr.green = 0.0;
        txt_params->text_bg_clr.blue = 0.0;
        txt_params->text_bg_clr.alpha = 1.0;

        nvds_add_display_meta_to_frame(frame_meta, display_meta);
    }

    //every n frames, print the count
    if (frame_number % 100 == 0){
      g_print ("Frame Number = %d Noise Count = %d Person Count = %d\n",
              frame_number, other_count, person_count);
    }
    frame_number++;
    return GST_PAD_PROBE_OK;
}

