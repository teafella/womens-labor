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

//ffmpeg
// #include <libavcodec/avcodec.h>
// #include <libavutil/opt.h>
// #include <libavutil/imgutils.h>

//internal includes
#include "src/util.h"
#include "lib/SOIL.h"

#include "src/Engine.h"

//Global defines
#include <time.h>
#define IMAGE_SIZE 128
#define IN_TEX_NAME "/images/pusheen.png"

#define PRINT_FRAMETIME false

int end_program = 0;

//Frame Timing
float frame_begin;
float frameEnd;
float frame_time;
//-------------------------

clock_t begin = clock();
bool programRunning = true;

Engine* engine;

//notes on Loading Textures with SOIL

// static void load_tex_images(CUBE_STATE_T *state)
// {
//   SOIL_free_image_data(state->inputImageTexBuf);
//   //SOIL LOADER
//   state->inputImageTexBuf = SOIL_load_image(( Util::GetExecutablePath() + std::string(IN_TEX_NAME)).c_str(), &state->buf_width, &state->buf_height, 0, SOIL_LOAD_RGB);

//   if ( 0 == state->inputImageTexBuf)
//   {
//     printf( "SOIL loading error: '%s'\n", SOIL_last_result() );
//   }

// }

  //initialize cv input texture
  // state->inputCVList = std::vector<GLushort>(RECORD_BUFFER + sqrt(RECORD_BUFFER) + 1 , 0);

  // Image Loading Example
  // load_tex_images(state);
  // glActiveTexture(GL_TEXTURE8);
  // glBindTexture(GL_TEXTURE_2D, state->tex[3]);
  // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, state->buf_width, state->buf_height, 0,
  //              GL_RGB, GL_UNSIGNED_BYTE, state->inputImageTexBuf);
  // glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
  // glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_LINEAR);
  // Util::check(__LINE__);


// static int get_mouse(CUBE_STATE_T *state, int *outx, int *outy)
// {
//   static int fd = -1;
//   const int width = state->screen_width, height = state->screen_height;
//   static int x = 800, y = 400;
//   const int XSIGN = 1 << 4, YSIGN = 1 << 5;
//   if (fd < 0) {
//     fd = open("/dev/input/mouse0", O_RDONLY | O_NONBLOCK);
//   }
//   if (fd >= 0) {
//     struct {char buttons, dx, dy; } m;
//     while (1) {
//       int bytes = read(fd, &m, sizeof m);
//       if (bytes < (int)sizeof m) goto _exit;
//       if (m.buttons & 8) {
//         break; // This bit should always be set
//       }
//       read(fd, &m, 1); // Try to sync up again
//     }
//     if (m.buttons & 3)
//       return m.buttons & 3;
//     x += m.dx;
//     y += m.dy;
//     if (m.buttons & XSIGN)
//       x -= 256;
//     if (m.buttons & YSIGN)
//       y -= 256;
//     if (x < 0) x = 0;
//     if (y < 0) y = 0;
//     if (x > width) x = width;
//     if (y > height) y = height;
//   }
// _exit:
//   if (outx) *outx = x;
//   if (outy) *outy = y;
//   return 0;
// }

// static const char *const evval[3] = {
//   "RELEASED",
//   "PRESSED ",
//   "REPEATED"
// };

// //Keyboard input
// int keyboardFd = -1;

// bool setupKeyboard() {
//   const char *dev = "/dev/input/by-id/usb-_USB_Keyboard-event-kbd";

//   keyboardFd = open(dev, O_RDONLY | O_NONBLOCK);
//   if (keyboardFd == -1) {
//     fprintf(stderr, "KEYBOARD: Cannot open keyboard at %s: %s.\n", dev, strerror(errno));
//     return false;
//   }
//   return true;
// }

// bool readKeyboard() {
//   struct input_event ev;
//   ssize_t n;

//   n = read(keyboardFd, &ev, sizeof ev);
//   if (n == (ssize_t) - 1) {
//     if (errno == EINTR)
//       return true;
//     else {
//       //just continue here
//       return true;
//     }
//   } else if (n != sizeof ev) {
//     errno = EIO;

//     return false;
//   }
//   if (ev.type == EV_KEY && ev.value >= 0 && ev.value <= 2)
//     printf("%s 0x%04x (%d)\n", evval[ev.value], (int)ev.code, (int)ev.code);

//   if (ev.code == KEY_ESC) {
//     return false;
//   }
//   return true;
// }

//==============================================================================


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
