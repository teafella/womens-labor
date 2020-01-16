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
#include "src/osc/oscListener.h"
#include <lib/oscpack/ip/PacketListener.h>
#include "src/util.h"

//wiringPi includes
#define BASE 100
#define SPI_CHAN 0
#include <wiringPi.h>
#include <softPwm.h>

Input* Input::singleton_ = 0;

Input::Input(bool run_tests) {
	singleton_ = this;
	on_ = false;
	pitch_ = 0;
	spectrum_ = std::vector<float>(SPECTRUM_SIZE);
	//setup OSC control
	SetupSerial();
	<<< <<< < HEAD
	if (run_tests) {
		InitOSC();
	}
	== == == =
	    InitOSC();

	>>> >>> > a295da93f32c7c26f5946420c230bad1b4ee1198
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
		// std::cout<< "Read Serial: "<<  buff <<std::endl ;
		if (rd != 0) {
			if (strchr(buff, '\n') != NULL) {
				char* tok;
				int index = -1;

				tok = strtok(buff, " ");
				if (tok != NULL) {

					index = atoi(tok);
					// std::cout<< "Index: "<< index ;
				}
				else {
					//~ return false;
				}
				tok = strtok(NULL, "\n");

				if (tok != NULL) {
					// printf(" Value: %s\n", tok);
					// std::cout<<std::endl;
					int val = atoi(tok);

					///set vars
					// 0 is on off for note
					if (index == 0) {
						SetOn(val);
					}

					if (index == 1) {
						SetPitch(val);
					}

					if (index >= 2  && index < 19) {
						float spec_val = atof(tok) / 200.;
						SetSpectrum(index - 2, spec_val );
					}
				}

			}

		}

		// std::cout<< "Finished Reading serial: " << rd << std::endl ;
	}

	return true;
}

void Input::InitOSC() {
	SendOSCTest();
	osc_thread_running_ = true;
	osc_thread_ = std::thread(&Input::ReadOSC, this);

}

void Input::ReadOSC() {
	try {
		MyPacketListener listener;
		UdpListeningReceiveSocket s(
		    IpEndpointName( IpEndpointName::ANY_ADDRESS, PORT ),
		    &listener );

		listener.SetOSCCallback(std::bind(&Input::OnOSC, this, std::placeholders::_1, std::placeholders::_2));

		std::cout << "OSC: Listening for input on port " << PORT << "...\n";
		s.Run();
	}
	catch (const std::exception &exc) {
		std::cerr << "OSC: " << exc.what();
	}
}

void Input::OnOSC(std::string address, int val) {
	// std::cout << "got some osc" << std::endl;
	if (address.compare("/pitch") == 0) {
		//set pitch without sending osc
		pitch_ = val;
		if (onPitch) {
			onPitch(val);
		}
	}
}

void Input::SetOn(bool val) {
	on_ = val;
	// std::cout<< "Set on: " << on_ << std::endl;
	osc_sender_.send("/on", val);
	if (onOn) {
		onOn(val);
	}
}

void Input::SetPitch(unsigned int val) {
	pitch_ = val;
	// std::cout<< "Set pitch: " << pitch_ << std::endl;
	osc_sender_.send("/pitch", val);
	if (onPitch) {
		onPitch(val);
	}
}

void Input::SetSpectrum(int index, float val) {
	// make sure val is being converted correctly from float here
	spectrum_[index] = val;
	osc_sender_.send("/spectrum", index, val);
	if (onSpectrum) {
		onSpectrum(index, val);
	}
}

void Input::SetOnCallback(std::function<void(bool)> callback) {
	onOn = callback;
}
void Input::SetPitchCallback(std::function<void(unsigned int)> callback) {
	onPitch = callback;
}
void Input::SetSpectrumCallback(std::function<void(int, float)> callback) {
	onSpectrum = callback;
}

bool Input::SetupSerial() {
	const char *dev = SERIAL_PATH;
	const char *alt_dev = ALT_SERIAL_PATH;

	serial_fd_ = open(dev, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
	fcntl(serial_fd_, F_SETFL, 0);
	if (serial_fd_ == -1) {
		fprintf(stderr, "Cannot open %s: %s.\n Trying Alt Path\n", dev, strerror(errno));
		serial_fd_ = open(alt_dev, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
		fcntl(serial_fd_, F_SETFL, 0);
		if (serial_fd_ == -1) {
			fprintf(stderr, "Cannot open %s: %s.\n", dev, strerror(errno));
			return false;
		}
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
