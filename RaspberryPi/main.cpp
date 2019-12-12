#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>

#include <linux/input.h>

#include "src/Engine.h"

//Global defines
#include <time.h>
#define PRINT_FRAMETIME false

int end_program = 0;

//Frame Timing
float frame_begin;
float frameEnd;
float frame_time;
//-------------------------

clock_t begin = clock();
bool programRunning = true;

int main (int argc, char *argv[])
{

  // command line arguments ( for qa and tests )
  bool run_tests = false;
  if (argc == 2) {
    if (strcmp(argv[1], "test") == 0 ) {
      run_tests = true;
      std::cout<< "Running in test mode." <<std::endl;
    }
  }

  bcm_host_init();
  //Start Engine
  engine = new Engine(run_tests);

  frame_begin = Util::ProgramTime();
  while (!end_program) {
    frame_time = (Util::ProgramTime() - frame_begin) / 100.;
    frame_begin = Util::ProgramTime();
    engine->Update();

    if (PRINT_FRAMETIME) {
      std::cout << "Frame Time: " << frame_time  << std::endl;
    }
    //detect dips below 55 fps 
    else if (frame_time > 0.018) { 
      std::cout << "Framerate Dip: " << frame_time  << std::endl;
    }

  }

  delete(engine);
  fflush(stdout);
  fprintf(stderr, "%s.\n", strerror(errno));
  return 0;
}
