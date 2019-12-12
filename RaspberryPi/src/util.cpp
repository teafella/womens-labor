#ifndef UTIL_CPP
#define UTIL_CPP

#include "util.h"

#include <cassert>
#include <iostream>
#include <fstream>
#include <unistd.h>


//UTILITY CLASS
//---------------
// Helpful functions for OpenGL Shenanigans,
// should be generally applicable functions that are relevant throughout the codebase
// (e.g. Color space conversions, opengl error checking funcitons etc.)

float Util::ProgramTime(){
  std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - CLOCK_BEGIN;
  return elapsed_seconds.count() * 20;
}


//will need to be changed for different OS compatilbility
float Util::TimeInSeconds(){
  return(float) clock() / (CLOCKS_PER_SEC); //Linux
}

std::string Util::GetExecutablePath() {
  char buf[255] = "";
  readlink("/proc/self/exe", buf, sizeof(buf) );
  std::string path = std::string(buf);
  int position = path.rfind ('/');
  path = path.substr(0, position) + "/";
  return path;
}


std::string Util::ReadFile(const char *filePath) {
  std::string line = "";
  std::ifstream in(filePath);
  if (!in.is_open()) {
    std::cerr << "Could not read file " << filePath << ". File does not exist." << std::endl;
    return "";
  }
  std::string content((std::istreambuf_iterator<char>(in)),
                      std::istreambuf_iterator<char>());

  in.close();
  return content;
}


float Util::charToAudioFloat(unsigned char c) {
  return ((float)c / 255. - 0.5) * 2.;
}


template <typename T>
T Util::clip(const T& n, const T& lower, const T& upper) {
  return std::max(lower, std::min(n, upper));
}

// // a basic weighted average filter:
// template <typename T>
// T Util::filter(const T& raw_value, float weight, const T& last_value) {
//   // run the filter
//   return weight * raw_value + (1.0-weight)*last_value;
// }


#endif