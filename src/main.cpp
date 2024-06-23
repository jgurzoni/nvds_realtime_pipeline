/*
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gst/gst.h>
#include "pipeline.h"
#include <cuda_runtime.h>
#include <iostream>

void usage(const char *bin)
{
  g_printerr ("Usage: %s <h264_elementary_stream>\n", bin);
}

int main (int argc, char *argv[])
{

  /* Check input arguments */
  if (argc < 2) {
    usage(argv[0]);
    return -1;
  }
  char* input_stream = argv[1];

  /* Initialize CUDA */
  int current_device = -1;
  cudaGetDevice(&current_device);
  struct cudaDeviceProp prop;
  cudaGetDeviceProperties(&prop, current_device);
  // GST initialization
  gst_init (&argc, &argv);
  //Create pipeline
  Pipeline pipeline = Pipeline(input_stream);
  pipeline.create_elements();
  pipeline.run(static_cast<gchar*>(input_stream));

  return 0;
}