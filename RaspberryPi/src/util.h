#ifndef UTIL_H
#define UTIL_H
#include <string>
#include <chrono>

static auto CLOCK_BEGIN = std::chrono::system_clock::now();

class Util {
public:
	static float ProgramTime(); // in ms
	static float TimeInSeconds();

	//filesystem stuff
	static std::string GetExecutablePath();
	static std::string ReadFile(const char *filePath);

	float charToAudioFloat(unsigned char c);

	template <typename T> static T clip(const T& n, const T& lower, const T& upper);
	template <typename T> inline static T filter(const T& raw_value, float weight, const T& last_value, const T& dead_zone = 0) {
		// run the filter
		return weight * raw_value + (1.0 - weight) * last_value;
	}
};

#endif