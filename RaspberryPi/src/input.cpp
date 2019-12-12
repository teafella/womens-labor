#ifndef INPUT_CPP
#define INPUT_CPP

#include "input.h"

#include <iostream>
#include <iomanip>
#include <unistd.h>

#include <string.h>
#include <fstream>
#include <fcntl.h>
#include <cmath>
#include <memory>

//VIDOS classes
#include "src/osc/oscSender.h"
#include "src/util.h"

//wiringPi includes
#define BASE 100
#define SPI_CHAN 0
#include <wiringPi.h>
#include <softPwm.h>

const int SPECTRUM_SIZE = 18;

Input* Input::singleton_ = 0;

Input::Input(bool run_tests) {
	singleton_ = this;
	on_ = false;
	pitch_ = 0;
	spectrum_ = std::vector<float>(SPECTRUM_SIZE);
	//setup OSC control
	SetupSerial();

	SendOSCTest();

}

Input::~Input() {
	if (serial_thread_running_ == true) {
		serial_thread_running_ = false;
		serial_thread_.join();
	}
}

bool Input::SendOSCTest() {
	osc_sender_.send("/test", 123);
}


//Serial input - from arduino

//reads serial inputs from Arduino, make sure to setupSerial() before calling this function
bool Input::ReadSerial() {
	while (serial_thread_running_) {
		char buff[0x1000];
		ssize_t rd = read(serial_fd_, buff, 100);
		if (rd != 0) {
			if (strchr(buff, '\n') != NULL) {
				char* tok;
				int index = -1;
				tok = strtok(buff, " ");
				if (tok != NULL) {

					index = atoi(tok);
					std::cout<< "Index: "<< index <<std::endl;
				}
				else {
					//~ return false;
				}
				tok = strtok(NULL, "\n");

				if (tok != NULL) {
					printf("Value: %s\n", tok);
					int val = atoi(tok);
					if (val < 0) {
						val = 0;
					}
					else if (val > 1024) {
						val = 1024;
					}

					///set vars


				}
			}


		}
	}

	return true;
}

bool Input::SetupSerial() {
	const char *dev = "/dev/ttyUSB0";

	serial_fd_ = open(dev, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
	fcntl(serial_fd_, F_SETFL, 0);
	if (serial_fd_ == -1) {
		fprintf(stderr, "Cannot open %s: %s.\n", dev, strerror(errno));
		return false;
	}

	serial_thread_running_ = true;
	serial_thread_ = std::thread(&Input::ReadSerial, this);

	return true;
}


float Input::smooth(float in, float PrevSmoothVal, float deadzone_percent, double weight ) {
	//truncate in val 4 decimal places
	in = std::trunc(1000 * in) / 1000;

	float this_margin = PrevSmoothVal * deadzone_percent ; // .025//* 0.01; //  get 2% of the raw value.  Tune for lowest non-jitter value.

	/*
	 * Next we add (or subtract...) the 'standard' fixed value to the previous reading. (PotPrevVal needs to be declared outside the function so it persists.)
	 * Here's the twist: Since the jitter seems to be worse at high raw vals, we also add/subtract the 2% of total raw. Insignificantat on low
	 * raw vals, but enough to remove the jitter at raw >900 without wrecking linearity or adding 'lag', or slowing down the loop, etc.
	 */
	//deadzone creates a bigger zone where values are considered invalid, tune for lowest non-jitter value
	if (PrevSmoothVal != in) { //if its thesame value just ignore it
		if (in > PrevSmoothVal + (this_margin) || in < PrevSmoothVal - (this_margin)) { // a 'real' change in value? Tune the two numeric values for best results

			//average last 2 values ofr smoothing
			in = (PrevSmoothVal * weight) + (in * (1.0 - weight)) ;//- this_margin;

			return in;
		}
		else {
			// in = ( PrevRawVal * weight) + (PrevSmoothVal * (1.0 - weight));

			// return in;
		}
	}
	return -1;
}

#endif
