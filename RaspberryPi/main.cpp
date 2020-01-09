#include <iostream>
#include <linux/input.h>

//Global defines
#include <time.h>

//Internal includes
#include "src/util.h"
#include "src/audio/audio.h"
#include "src/input.h"

int main (int argc, char *argv[])
{

  // command line arguments
  bool run_tests = false;
  if (argc == 2) {
    if (strcmp(argv[1], "test") == 0 ) {
      run_tests = true;
      std::cout<< "Running in test mode. Test audio engine is on." <<std::endl;
    }
  }

  //Start Inputs
  Input* input = new Input(run_tests);

  //Start Audio Engine
  Audio* audio = new Audio(run_tests, input);

  // frame_begin = Util::ProgramTime();
  // while (!end_program) {
  //   frame_time = (Util::ProgramTime() - frame_begin) / 100.;
  //   frame_begin = Util::ProgramTime();

  //   if (PRINT_FRAMETIME) {
  //     std::cout << "Frame Time: " << frame_time  << std::endl;
  //   }
  //   //detect dips below 55 fps 
  //   else if (frame_time > 0.018) { 
  //     std::cout << "Framerate Dip: " << frame_time  << std::endl;
  //   }

  // }
  delete(audio);

  fflush(stdout);
  fprintf(stderr, "%s.\n", strerror(errno));
  return 0;
}
