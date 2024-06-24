
#include <gst/gst.h>
#include <glib.h>
#include <stdio.h>
#include "gstnvdsmeta.h"
#include <cuda_runtime_api.h>

#include <functional>
#include <iostream>

#define INFER_PGIE_CONFIG_FILE "conf_infer_primary_yolov10n.txt"

/* The muxer output resolution must be set if the input streams will be of
 * different resolution. The muxer will scale all the input frames to this
 * resolution. */
#define MUXER_OUTPUT_WIDTH 1280
#define MUXER_OUTPUT_HEIGHT 720

class Pipeline {
private:
  GstElement *pipeline = NULL, *source = NULL, *qtdemux = NULL, 
        *h264parser = NULL, *decoder = NULL, *streammux = NULL, 
        *primary_detector = NULL, *nvvidconv = NULL, *nvosd = NULL, 
        *encoder = NULL, *parser = NULL, *muxer = NULL, *sink = NULL;
  GMainLoop *loop = NULL;
  GstBus *bus = NULL;
  guint bus_watch_id;
  GstPad *nvvidconv_sink_pad = NULL;
  gchar *input_stream = NULL;
  gchar *output_file = NULL;
  const gchar *infer_pgie_config_file = INFER_PGIE_CONFIG_FILE;
  guint muxer_output_width = MUXER_OUTPUT_WIDTH;
  guint muxer_output_height = MUXER_OUTPUT_HEIGHT;
  gint frame_number;
  bool is_init = false;

public:
  Pipeline();
  ~Pipeline();
  void run(gchar* input_stream, gchar* output_file);
  int Init();
private:
  int create_elements(void);
  void setup_bus_handler(void);
  GstPadProbeReturn nvvidconv_sink_pad_buffer_probe(GstPad* pad, GstPadProbeInfo* info, gpointer u_data);
};

