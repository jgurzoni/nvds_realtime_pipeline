/*
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gst/gst.h>
#include "pipeline.h"
#include <cuda_runtime.h>
#include <iostream>
#include <thread>

void usage(const char *bin)
{
  g_printerr ("Usage: %s <input_stream> <output_file>\n", bin);
}

int main (int argc, char *argv[])
{

  /* Check input arguments */
  if (argc < 3) {
    usage(argv[0]);
    return -1;
  }
  gchar* input_stream = static_cast<gchar*>(argv[1]);
  gchar* output_file = static_cast<gchar*>(argv[2]);

  // GST initialization
  gst_init (&argc, &argv);
  //Create pipeline
  Pipeline pipeline = Pipeline();
  //Initialize pipeline
  pipeline.Init();
  //Run pipeline
  /* Creates a new thread for pipeline - 
  The use of threading here is just to show that multiple pipeline instances can be run*/
  std::thread pipelineThread([input_stream = input_stream, output_file = output_file, &pipeline]() {
    // Run the pipeline
    pipeline.run(input_stream, output_file);
  });
  // Join the pipeline thread
  pipelineThread.join();
  
  return 0;
}