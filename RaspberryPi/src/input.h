#ifndef INPUT_H
#define INPUT_H

#include <thread>
#include <pthread.h>
#include <mutex>
#include <functional>
#include <vector>
#include <memory>
#include <atomic>

#include "src/osc/oscSender.h"

//forward declartion

class Input
{
public:
	Input(bool run_tests = false);
	~Input();
	std::thread serial_thread_;

	//not a strict singleton but gets last instatiated instance (im lazy)
	static Input* singleton_;

private:
	//state variables
	std::atomic<bool> on_;
	std::atomic<float> pitch_;
	std::vector<float> spectrum_;


	int serial_fd_;
	std::atomic<bool> serial_thread_running_;

	//OSC
	OSCSender osc_sender_;
	bool SendOSCTest();

	//SERIAL
	bool SetupSerial();
	bool ReadSerial();

	static float smooth(float in, float PrevSmoothVal, float PrevRawVal, double weight = 0.5);
	// float FilterInput(float in, BiquadChain* filter);
};


#endif
