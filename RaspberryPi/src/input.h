#ifndef INPUT_H
#define INPUT_H

#include <thread>
#include <pthread.h>
#include <mutex>
#include <functional>
#include <vector>
#include <memory>


//forward declartion
class Midi;
class NoteMessage;
class Button;

class Input
{
public:
	Input(bool run_tests = false);
	~Input();
	std::thread input_thread_;

	//not a strict singleton but gets last instatiated instance (im lazy)
	static Input* singleton_;

private:

	bool readADC();

	int serial_fd;

	//OSC
	bool setupOSC();
	bool readOSC();
	bool SendOSCTest();

	//SERIAL
	bool setupSerial();
	bool readSerial();

	static float smooth(float in, float PrevSmoothVal, float PrevRawVal, double weight = 0.5);
	// float FilterInput(float in, BiquadChain* filter);
};


#endif
