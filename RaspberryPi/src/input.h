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
#include "src/osc/oscListener.h"

#define SERIAL_PATH "/dev/ttyUSB0"
#define ALT_SERIAL_PATH "/dev/ttyUSB1"


const int SPECTRUM_SIZE = 18;

//forward declartion

class Input
{
public:
	Input(bool run_tests = false);
	~Input();
	std::thread serial_thread_;

	void SetOnCallback(std::function<void(bool)> callback);
	void SetPitchCallback(std::function<void(unsigned int)> callback);
	void SetSpectrumCallback(std::function<void(int, float)> callback);

	//not a strict singleton but gets last instatiated instance (im lazy)
	static Input* singleton_;

private:
	//state variables
	std::atomic<bool> on_;
	void SetOn(bool val);
	std::atomic<unsigned int> pitch_;
	void SetPitch(unsigned int val);
	std::vector<float> spectrum_;
	void SetSpectrum(int index, float val);

	//callbacks
	std::function<void(bool)> onOn = 0;
	std::function<void(unsigned int)>  onPitch = 0;
	std::function<void(int, float)>  onSpectrum = 0;


	int serial_fd_;
	std::atomic<bool> serial_thread_running_;

	//OSC
	OSCSender osc_sender_;
	std::thread osc_thread_;
	std::atomic<bool> osc_thread_running_;
	// OSCListener osc_listener_;
	void OnOSC(std::string, int);

	bool SendOSCTest();
	void ReadOSC();
	void InitOSC();

	//SERIAL
	bool SetupSerial();
	bool ReadSerial();

	static float smooth(float in, float PrevSmoothVal, float PrevRawVal, double weight = 0.5);
	// float FilterInput(float in, BiquadChain* filter);
};


#endif
