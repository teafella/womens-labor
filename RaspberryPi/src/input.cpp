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
#include "src/hardware/button.h"
#include "src/io/midi.h"
#include "src/io/oscListener.h"
#include "src/io/oscSender.h"
#include "src/util.h"


//wiringPi includes
#define BASE 100
#define SPI_CHAN 0
#include <wiringPi.h>
#include <softPwm.h>
#include "mcp3004.h"

//lib
#include <lib/json.hpp>
// for convenience
using json = nlohmann::json;

#define MIDI_PORT 1 //Just leave this at one and the 1st midi device connected to PI will work


#define USE_LED_PWM false
#define USE_POT_LED_PWM false

//smoothing globals
#define SMOOTH_POT true
#define SMOOTH_CV true


//HARDWARE PINS!
#define MUX_ADDRESS_PINA 24//0
#define MUX_ADDRESS_PINB 25//2
#define MUX_ADDRESS_PINC 26//3

#define TRIG_A 20//29
#define TRIG_B 21//28

#define BUTTON_PIN_0 14
#define BUTTON_PIN_1 15
#define BUTTON_PIN_2 16

// int LED_PIN[3][3] = {{7, 21, 22}, {23, 24, 25}, {4, 5, 11}};
int LED_PIN[3][3] = {{0,1, 2}, {3, 4, 5}, {6,7,13}};

#define POT1_LED_PIN 18//26 // also 12?
#define POT3_LED_PIN 19//27

#define SOFTPWM_RANGE 100

#define NUM_MOD_STATES 2

#define SAMPLE_RATE 100 // a second (Hz)

//Debug
#define DEBUG_PANEL true
#define DEBUG_CVIN false

#define DEBUG_CV 1

#define SHOW_SAMPLETIME false
#define USE_ADC true

Input* Input::singleton_ = 0;


Input::Input(bool run_tests) {
	singleton_ = this;
	onButton = 0;
	onAltPot = 0;
	onCV = 0;
	onClock = 0;
	onModMatrix = 0;
	onPresetHold = 0;
	onPresetToggle = 0;
	adc_initialized_ = false;


	//initialize filter members
	// double gain = .001;
	// bool designedCorrectly = filter_model_.loPass(2680,  // fs
	//                          500,    // freq1
	//                          0,      // freq2. N/A for lowpass
	//                          4, 	    // filter order,
	//                          coeffs_, // coefficient array being filled
	//                          gain);   // overall gain
	// if (designedCorrectly == false) {
	// 	std::cout << "Input Filtering Parameters are Incorrect" << std::endl;
	// }
	// int stages = (int) coeffs_.size();
	// for (int x = 0; x < CV_COUNT; ++x) {
	// 	BiquadChain this_chain(stages);
	// 	cv_filters_.push_back(this_chain);
	// }
	// for (int x = 0; x < POT_COUNT; ++x) {
	// 	BiquadChain this_chain(stages);
	// 	pot_filters_.push_back(this_chain);
	// }

	// go through input possibilities
	// try to use Hardware ADC
	StartADCThread(run_tests);
	//setup OSC control
	// setupOSC();

	//try to restore last save state of control values
	//

	//setup MIDI Control
	setupMIDI(MIDI_PORT);
}

Input::~Input() {
	if (threadRunning) {
		threadRunning = false;

		inputThread.join();
		osc_thread_.join();
	}

	delete(myMIDI);
}

void Input::update() {

}

void Input::AddButtonCallback(std::function<void(int, int)> buttonCallback) {
	onButton = buttonCallback;

	// if (onButton) {
	// 	for(int x = 0 ; x <BUTTON_COUNT; ++x) {
	// 		onButton(x, buttons_[x]->GetState() );
	// 	}
	// }
	ResetButtonLED();
}

void Input::AddCVCallback(std::function<void(int, float)> cvCallback) {
	onCV = cvCallback;

	//callback on Current values to new target
	// if (onCV) {
	// 	for (int x = 0; x < CV_COUNT; ++x) {
	// 		onCV(x, getPot(x) + getCV(x));
	// 	}
	// }
}

void Input::AddAltPotCallback(std::function<void(int, int, float)> potCallback) {
	onAltPot = potCallback;

	//callback on Current values to new target
	// if (onAltPot) {
	// 	for (int x = 0; x < BUTTON_COUNT; ++x) {
	// 		for (int y = 0; y < POT_COUNT; ++y) {
	// 			onAltPot(x, y, GetAltPot(x, y));
	// 		}
	// 	}
	// }
}

void Input::AddModMatrixCallback(std::function<void(glm::mat3)> modCallback) {
	onModMatrix = modCallback;

	//callback on Current values to new target
	// if (onModMatrix) {
	// 	onModMatrix(button_mod_matrix_);
	// }

}

void Input::AddModMatrixValueCallback(std::function<void(int, int, float)> modValueCallback) {
	onModMatrixValue = modValueCallback;

	if (onModMatrixValue) {
		//callback on individual mod matrix values
		// for ( int x = 0; x < 3; ++x) { //glm mat 3 is always size of 3
		// 	for ( int y = 0; y < 3; ++y) {
		// 		onModMatrixValue(x, y, button_mod_matrix_[x][y]);
		// 	}
		// }
	}
}

void Input::AddPresetHoldCallback(std::function<void(int)> thisCallback) {
	onPresetHold = thisCallback;
}
void Input::AddPresetToggleCallback(std::function<void(int)> thisCallback) {
	onPresetToggle = thisCallback;
}

void Input::midiNoteCallback(NoteMessage* msg) {
	bool trig = msg->on;
	int this_channel = msg->channel;
	int this_note = msg->note;
	// std::cout << "note callback: " << trig << " " <<  this_note << std::endl;

	if (this_channel == 15) { // opz video channel
		if (trig) { //note on
			if (this_note % 12 == 0) {
				singleton_->ToggleButton(0);
			}
			else if (this_note % 12 == 2) {
				singleton_->ToggleButton(1);
			}
			else if (this_note % 12 == 4) {
				singleton_->ToggleButton(2);
			}
		}
		//note off
		else {

		}
	}

	delete msg;
}

void Input::midiCCCallback(int channel, int index, float value) {
	if (channel == 15) { // op-z video channel only for now
		if (index < 8) {
			singleton_->SetPot(index, value, true);
		}
		else if (index < 15) {
			//osc 1 menu
			singleton_->SetAltPot(0, index - 8, value );
			// std::cout << "ccing alt 1" << std::endl;
		}
		else if (index < 22) {
			//osc 2 menu
			singleton_->SetAltPot(2, index - 15, value );
		}
		else if (index < 29) {
			// master menu
			singleton_->SetAltPot(2, index - 22, value );
		}
	}
}

bool Input::setupMIDI(int port) {
	myMIDI = new Midi(port);
	myMIDI->setNoteMessageCallback(&midiNoteCallback);
	myMIDI->setCCMessageCallback(&midiCCCallback);
	return true;
}

bool Input::SendOSCTest() {
	OSCSender test_sender;
	// test_sender.send();
}

bool Input::setupOSC() {
	threadRunning = true;
	osc_thread_ = std::thread(&Input::readOSC, this);
	return true;
}

bool Input::readOSC() {
	try {
		MyPacketListener listener(this);
		UdpListeningReceiveSocket s(
		    IpEndpointName( IpEndpointName::ANY_ADDRESS, PORT ),
		    &listener );

		std::cout << "OSC: Listening for input on port " << PORT << "...\n";
		s.Run();
	}
	catch (const std::exception &exc) {
		std::cerr << "OSC: " << exc.what();
	}
	return true;
}

bool Input::InitADC() {
	double timeout = 0;
	clock_t init_start = clock();
	while (!adc_initialized_ && timeout < 5) {
		if (mcp3004Setup(BASE, SPI_CHAN)) {
			std::cout << "ADC: Initialized Sucessfully" << std::endl;
			adc_initialized_ = true;
		}
		timeout = double(clock() - init_start) / CLOCKS_PER_SEC;
	}
	if (timeout > 5) {
		std::cout << "ADC: Timed Out." << std::endl;
		adc_initialized_ = false;
	}
	return adc_initialized_;
}

bool Input::StartADCThread(bool run_tests) {
	//initialize wiringPi
	if (wiringPiSetup() == -1)
	{
		std::cout << "Input Error: WiringPi setup failure" << std::endl;
		return false;
	}

	SetupHardwarePins();

	//run hardware tests (for new units and QA)
	if (run_tests) {
		TestHardwareConfig();
	}


	//start input thread
	threadRunning = true;
	inputThread = std::thread(&Input::readADC, this);

	// sched_param sch_params;
	// sch_params.sched_priority = 5;
	// pthread_setschedparam(inputThread.native_handle(), SCHED_RR, &sch_params);


	return true;
}

void Input::TestHardwareConfig() { // Hypno Specific
	//check initialization
	if (USE_ADC == false) {
		std::cerr << "ADC Flag Disabled, Cannot Run Hardware Tests" << std::endl;
	}
	if (!adc_initialized_) {
		InitADC();
	}

	char user_input;

	//test LED Connections
	std::cout << "Testing LED Connections" << std::endl;

	setPotLEDState(0, false );
	setPotLEDState(1, false);

	std::cout << "All LEDs should be OFF" << std::endl;
	std::cin.ignore();

	setPotLEDState(0, true );
	setPotLEDState(1, true );

	std::cout << "Pot LEDs Should be ON" << std::endl;
	std::cin.ignore();

	for (int x = 0; x < RGB_LED_COUNT; ++x) {
		Input::SetLEDColor(x, glm::vec3(255, 0, 0));
	}

	std::cout << "All LEDs Should be lit in RED" << std::endl;
	std::cin.ignore();

	for (int x = 0; x < RGB_LED_COUNT; ++x) {
		Input::SetLEDColor(x, glm::vec3(0, 255, 0));
	}

	std::cout << "All LEDs Should be lit in GREEN" << std::endl;
	std::cin.ignore();

	for (int x = 0; x < RGB_LED_COUNT; ++x) {
		Input::SetLEDColor(x, glm::vec3(0, 0, 255));
	}

	std::cout << "All LEDs Should be lit in BLUE" << std::endl;
	std::cin.ignore();

	bool pass = false;
	int muxCounter = 0;
	while (muxCounter < 8) {
		while (pass == false) {
			int val = AnalogReadMuxedValue(muxCounter, BASE, MUX_ADDRESS_PINA, MUX_ADDRESS_PINB, MUX_ADDRESS_PINC );
			std::cout << "Turn pot " << muxCounter << " fully CW/UP." << "Current value:" << val << std::endl;
			if (val == 0) {
				pass = true;
			}
		}
		pass = false;
		while (pass == false) {
			int val = AnalogReadMuxedValue(muxCounter, BASE, MUX_ADDRESS_PINA, MUX_ADDRESS_PINB, MUX_ADDRESS_PINC );
			std::cout << "Turn pot " << muxCounter << " fully CCW/DOWN." << "Current value:" << val << std::endl;
			if (val == 1023) {
				pass = true;
			}
		}
		pass = false;
		++muxCounter;
	}
	std::cout << "Pot Tests Passed!!!"  << std::endl;

	std::cout << "Start CV Tests"  << std::endl;
	std::cout << "Unplug all CV ins and press enter." << std::endl;
	std::cin.ignore();
	for ( int chan = 1; chan < 8 ; chan++) {
		pass = false;
		while (pass == false) {
			int val = analogRead( BASE + chan);
			if (val == 512) {
				pass = true;
			}
			else {
				std::cout << "CV " << chan << " : " << val << std::endl;
			}
		}
		std::cout << "CV " << chan << " Passed!" << std::endl;
	}
	std::cout << "Unpluged CV Test Passed!" << std::endl;

	for ( int chan = 1; chan < 8 ; chan++) {
		pass = false;
		while (pass == false) {
			int val = analogRead( BASE + chan);
			std::cout << "Plug a dummy cable into cv " << chan << " Current Value:" << val << std::endl;
			if (val == 1023) {
				pass = true;
			}
		}
	}
	std::cout << "CV Tests Passed!"  << std::endl;

	// std::cin.ignore();
	std::cout << "All Tests Passed!"  << std::endl;
}

void Input::SetupHardwarePins() {
	//Setup Muxing pins
	pinMode (MUX_ADDRESS_PINA, OUTPUT) ;
	pinMode (MUX_ADDRESS_PINB, OUTPUT) ;
	pinMode (MUX_ADDRESS_PINC, OUTPUT) ;
	//Set Pulldowns
	pullUpDnControl (MUX_ADDRESS_PINA, PUD_DOWN);
	pullUpDnControl (MUX_ADDRESS_PINB, PUD_DOWN);
	pullUpDnControl (MUX_ADDRESS_PINC, PUD_DOWN);

	//Set Up Trigger inputs
	pinMode (TRIG_A, INPUT) ;
	pullUpDnControl (TRIG_A, PUD_OFF);

	pinMode (TRIG_B, INPUT) ;
	pullUpDnControl (TRIG_B, PUD_OFF);

	//Create Buttons
	buttons_[0] = std::unique_ptr<Button>(new Button(BUTTON_PIN_0));
	buttons_[1] = std::unique_ptr<Button>(new Button(BUTTON_PIN_1));
	buttons_[2] = std::unique_ptr<Button>(new Button(BUTTON_PIN_2));

	// pinMode (BUTTON_PIN_0, INPUT) ;
	// pinMode (BUTTON_PIN_1, INPUT) ;
	// pinMode (BUTTON_PIN_2, INPUT) ;
	// //Set Button Pulldowns
	// pullUpDnControl (BUTTON_PIN_0, PUD_DOWN);
	// pullUpDnControl (BUTTON_PIN_1, PUD_DOWN);
	// pullUpDnControl (BUTTON_PIN_2, PUD_DOWN);

	//Set Led Pins
	for ( int x = 0; x < 3; ++x ) {
		for ( int y = 0; y < 3; ++y) {
			if (USE_LED_PWM) {
				softPwmCreate (LED_PIN[x][y], SOFTPWM_RANGE, SOFTPWM_RANGE );
			}
			else {
				pinMode (LED_PIN[x][y], OUTPUT) ;
				pullUpDnControl (LED_PIN[x][y], PUD_OFF);
			}

		}
	}
	//Set Pot Led Pins
	if (USE_POT_LED_PWM) {
		softPwmCreate (POT1_LED_PIN, SOFTPWM_RANGE, SOFTPWM_RANGE);
		softPwmCreate (POT3_LED_PIN, SOFTPWM_RANGE, SOFTPWM_RANGE);
	}
	else {
		pinMode (POT1_LED_PIN, OUTPUT) ;
		pullUpDnControl (POT1_LED_PIN, PUD_OFF);
		pinMode (POT3_LED_PIN, OUTPUT) ;
		pullUpDnControl (POT3_LED_PIN, PUD_OFF);
	}

	//turn on pot leds
	setPotLEDState(0, true);
	setPotLEDState(1, true);
}

int Input::AnalogReadMuxedValue(int value_index, int adc_address, int mux_address_pinA, int mux_address_pinB, int mux_address_pinC ) {
	// mask out individual bits and set address pins correctly
	digitalWrite(mux_address_pinA, (bool)(value_index & 1));
	digitalWrite(mux_address_pinB, (bool)(value_index & 2));
	digitalWrite(mux_address_pinC, (bool)(value_index & 4));

	return analogRead(adc_address);
}

bool Input::readADC() {

	//counter for multiplexing ( this should give slightly more priority to sampling CV in theory)
	int muxCounter = 0;
	sample_begin_ = Util::ProgramTime();

	while (threadRunning) {
		//check ADC initalization
		if (USE_ADC && !adc_initialized_) {
			InitADC();
		}

		//main hardware reading routine

		if ( true ) {
			//read mcp3008
			for ( int chan = 0; chan < 8 ; chan++)
			{
				//Read in all the Pots
				//Channel 0 is Multiplexed with CD4051BE
				if ( chan  == 0 ) {
					while (muxCounter <= 7) {
						// go through one of the 8 addresses of the mutliplexer via counter
						int val = AnalogReadMuxedValue(muxCounter, BASE + chan, MUX_ADDRESS_PINA, MUX_ADDRESS_PINB, MUX_ADDRESS_PINC );
						float max_val = 1024.0;
						float smoothVal = val;


						std::cout << "Pot " << muxCounter << " : " << val << std::endl;

						if (SMOOTH_POT) {
							// float in_val = truncVal / max_val;
							// smoothVal = FilterInput(in_val, &pot_filters_[muxCounter]);

							//apply running avg after

							smoothVal = smooth(smoothVal, lastSmoothPot[muxCounter],  .05, .95 );
						}

						if (smoothVal > -1) {
							float thisVal = (((max_val - smoothVal) / max_val) - 0.017) * 1.017 ;


							//clamping
							if (thisVal > 1.) {
								thisVal = 1.;
							}
							else if (thisVal < 0) {
								thisVal = 0.;
							}

							SetPot(muxCounter, thisVal);

							lastRawPot[muxCounter] = lastSmoothPot[muxCounter];
							lastSmoothPot[muxCounter] = smoothVal;

							if (DEBUG_PANEL && !DEBUG_CVIN && DEBUG_CV == muxCounter) {
								std::cout << "Pot " << muxCounter << " : " << thisVal << std::endl;
							}
						}
						else {
							// if (DEBUG_PANEL && !DEBUG_CVIN && DEBUG_CV == muxCounter) {
							// 	std::cout << "Pot " << muxCounter << " : " << thisVal << std::endl;
							// }
						}
						//advance and reset muxcounter
						++muxCounter;
					}

				}

				else {
					//read in ADC values for CV Modulation
					int cvNum = chan - 1; //subtracts an index bc 1st chan is used for Pot Mux
					int val = analogRead( BASE + chan);
					float max_val = 1024.0;
					float smoothVal = val;

					if (SMOOTH_CV) {
						// if (cvNum == 3) {
						smoothVal = val / max_val;
						// in_val = FilterInput(in_val, &cv_filters_[cvNum] );

						//apply running avg after
						smoothVal = smooth(smoothVal, lastSmoothCV[cvNum],  .018, .97 );
					}
					if (smoothVal > -1) {
						float cvVal = ((smoothVal)  - .5) * 2. ;
						setCV(cvNum,  cvVal);

						if (DEBUG_PANEL && DEBUG_CVIN && DEBUG_CV == cvNum) {
							std::cout << "CV " << cvNum << " : " << cvVal << " Smooth Val: " << smoothVal << std::endl;
						}

						if (SMOOTH_CV) {
							lastRawCV[cvNum] = lastSmoothCV[cvNum];
							lastSmoothCV[cvNum] = smoothVal;
						}
					}

				}
			}

			if (muxCounter > 7) { muxCounter = 0;} //reset mux counter

			sample_end_ = Util::ProgramTime();
			float frame_time = ( sample_end_ - sample_begin_) / 100.; //in seconds
			sample_begin_ = sample_end_; //set this time as the beginning of the next frame

			last_sample_time_ = frame_time;

			if (SHOW_SAMPLETIME) {
				std::cout << "ADC Sample Time: " << last_sample_time_ << std::endl;
			}

			float sample_time_difference = .001 - last_sample_time_ ; //100Hz lock

			// std::cout<< "Sample Time Difference: " << sample_time_difference <<std::endl;
			usleep(sample_time_difference * 1000000);
			// std::this_thread::sleep_for(std::chrono::milliseconds(200));

		}

		ReadButtons();
		ReadTriggers();

	}



	return true;
}


void Input::ReadTriggers() {
	int readA = !digitalRead(TRIG_A);
	int readB = !digitalRead(TRIG_B);
	bool changed = false;

	// std::cout<< readA <<std::endl;


	//rising edge
	if (readA == 1 && readA != last_trig_readA_) {
		setLEDState(0, buttons_[0]->ToggleState());
		changed = true;
		if (onButton) {
			onButton(0, buttons_[0]->GetState());
		}
	}
	last_trig_readA_ = readA;

	if (readB == 1 && readB != last_trig_readB_) {
		setLEDState(2, buttons_[2]->ToggleState());
		changed = true;
		if (onButton) {
			onButton(1, buttons_[2]->GetState());
		}
	}

	last_trig_readB_ = readB;
}

void Input::ToggleButton(int index) {
	setLEDState(index, buttons_[index]->ToggleState() );
	if (onButton) {
		onButton(index, buttons_[0]->GetState());
	}
}

//Serial input - from arduino

//reads serial inputs from Arduino, make sure to setupSerial() before calling this function
bool Input::readSerial() {

	while (threadRunning) {
		char buff[0x1000];
		ssize_t rd = read(serialFd, buff, 100);
		if (rd != 0) {
			if (strchr(buff, '\n') != NULL) {
				char* tok;

				int index = -1;
				tok = strtok(buff, " ");
				if (tok != NULL) {

					index = atoi(tok);
					//std::cout<< "Index: "<< index <<std::endl;
				}
				else {
					//~ return false;
				}
				tok = strtok(NULL, "\n");

				if (tok != NULL) {
					if (index == 10) {
						//printf("Value: %s\n", tok);
					}
					int val = atoi(tok);
					if (val < 0) {
						val = 0;
					}
					else if (val > 1024) {
						val = 1024;
					}

					if (index >= 10 && index < 30) { //cv input

						setCV(index - 10, val / 1024.0);
					}

				}
			}


		}
	}

	return true;
}

bool Input::setupSerial() {
	const char *dev = "/dev/ttyUSB0";

	serialFd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
	fcntl(serialFd, F_SETFL, 0);
	if (serialFd == -1) {
		fprintf(stderr, "Cannot open %s: %s.\n", dev, strerror(errno));
		return false;
	}

	threadRunning = true;
	inputThread = std::thread(&Input::readSerial, this);

	return true;
}

void Input::SetPot(int index, float val, bool ext_control_change) {
	if (index >= POT_COUNT || index < 0) {
		return;
	}
	// std::cout << "index " << index << " " << val << std::endl;
	// std::cout << ext_control_change << std::endl;
	if (input_mutex_.try_lock()) {
		potIn[index] = val;
		input_mutex_.unlock();
		//

		bool alt_change = false;
		bool alt_held = false;
		float ALT_ACTIVATION_MARGIN = .025; //for alt-pot deadzone

		if ( !ext_control_change && mod_menu_index_ != -1) {
			//activate alt-pot when pot has moved past a deadzone
			if (!alt_pot_active_[mod_menu_index_][index] && abs(last_pot_[index] - val) > ALT_ACTIVATION_MARGIN ) {
				alt_pot_active_[mod_menu_index_][index] = true;

				//led position indicator off when alt_pot is active
				if (index == 0 || index == 3) {
					int led_index = 0;
					if (index == 3) {
						led_index = 1;
					}
					//turn off Pot LED since alt pot is active
					setPotLEDState(led_index, false);
				}
			}
			// 			//only make alt value change on an active alt-pot
			if (alt_pot_active_[mod_menu_index_][index]) {
				if (input_mutex_.try_lock()) {
					alt_pot_[mod_menu_index_][index] = val;
					input_mutex_.unlock();
					if (onAltPot)  { //this should only change when a pot is moved significantly
						onAltPot(mod_menu_index_, index, val);
					}
					alt_change = true;

					last_alt_pot_[index] = val;
				}
			}
		}
		else { //deactivate all alt-pots
			if (!ext_control_change) {
				for (int button_index = 0; button_index < BUTTON_COUNT; ++button_index) {
					if (alt_pot_active_[button_index][index]) {
						alt_pot_active_[button_index][index] = false;
					}
				}
			}

			if (onCV) {
				// std::cout<< index << ": P: " << val << " CV: " <<  getCV(index) <<std::endl;
				float RETURN_ACTIVATION_MARGIN = .08; //for alt-pot deadzone return
				float RETURN_CATCH_MARGIN = .035; //for color/gain deadzone catch

				bool RETURN_CATCH = false;
				if (index == 3 || index == 1) { // catch knob for gain and color
					RETURN_CATCH = true;
					// std::cout << "RETURN DISTANCE [" << index<< "] " << last_pot_[index]<< " - " <<  val  << " = " << (float)abs(last_pot_[index] - val) << ": "<< (abs(last_pot_[index] - val) < RETURN_CATCH_MARGIN) << std::endl;
				}


				bool real_position = false;
				//Pickup for Controls: snaps into position after return activation margin is crossed
				//this change should only be valid if the new position is within pickup margin of control
				if (!RETURN_CATCH && ( abs(last_alt_pot_[index] - val) > RETURN_ACTIVATION_MARGIN || (last_pot_[index] < 0.0 || last_alt_pot_[index] < 0.0)  ) ) {
					onCV(index, getCV(index) +  val);

					// if (index == DEBUG_CV) {
					// 	std::cout << "INDEX: " << index << "VALUE: " << val << " CV: " << getCV(index) << std::endl;
					// }
					//capture previous VALID regular pot
					last_pot_[index] = val;
					last_alt_pot_[index] = -1.0; //disable alt-pot

					real_position = true;
				}
				// Catching Pots (for sensitive controls such as gain and color)
				else if (RETURN_CATCH && abs(last_pot_[index] - val) < RETURN_CATCH_MARGIN || (last_pot_[index] < 0.0 || last_alt_pot_[index] < 0.0) ) {

					onCV(index, getCV(index) + val);

					last_pot_[index] = val;
					last_alt_pot_[index] = -1.0; //disable alt-pot

					real_position = true;
				}

				//led position indicator (only for the sliders with LEDs (0 & 3) )
				if (index == 0 || index == 2) {
					int led_index = 0;
					if (index == 3) {
						led_index = 1;
					}
					//light pot led
					setPotLEDState(led_index, real_position);
				}


			}

		}
	}

}

int Input::GetCVCount() {
	return CV_COUNT;
}

int Input::GetButtonCount() {
	return BUTTON_COUNT;
}

int Input::GetPotCount() {
	return POT_COUNT;
}

float Input::getPot(int index) {
	float ret = 0.0;
	if (index < POT_COUNT) {
		// Input::inputMutex.lock();
		ret = potIn[index];
		// Input::inputMutex.unlock();
	}
	return ret;
}

float Input::GetAltPot(int button_index, int pot_index) {
	float ret = 0.0;

	if (button_index < BUTTON_COUNT && pot_index < POT_COUNT) {
		input_mutex_.lock();
		ret = alt_pot_[button_index][pot_index];
		input_mutex_.unlock();
	}

	return ret;
}

void Input::SetAltPot(int button_index, int pot_index, float value) {
	if (button_index < BUTTON_COUNT && pot_index < POT_COUNT) {
		if (value < 0.) {
			if ( (button_index == 2 && pot_index == 2) || (button_index == 0 && pot_index == 6) || (button_index == 2 && pot_index == 7) ) {
				value = .5;
			}
			else {
				value = 0.;
			}
		}

		input_mutex_.lock();
		alt_pot_[button_index][pot_index] = value;
		input_mutex_.unlock();

		if (onAltPot)  { //this should only change when a pot is moved significantly
			onAltPot(button_index, pot_index, value);
		}
	}
}

bool Input::SetModMatrixValue(int x, int y, float value) {
	button_mod_matrix_[x][y] = value ;

	if (onModMatrix) {
		onModMatrix(button_mod_matrix_);
	}
	if (onModMatrixValue) {
		onModMatrixValue(x, y,  value);
	}

	return true;
}


void Input::setCV(int index, float val) {
	if (index == 1) {
		index = 3;
	}
	else if (index >= 3) {
		index += 1;
	}

	//read
	clock_t readtime = clock();
	// inputMutex.lock();
	cvIn[index] = val;
	lastCV[index].push_back(val);
	lastCV[index].erase(lastCV[index].begin());
	// inputMutex.unlock();
	//save time when the read was performed
	lastCVRead[index] = (float) readtime / (CLOCKS_PER_SEC);

	if (onCV) {
		onCV(index, getPot(index) +  val);
	}
}

float Input::getCV(int i) {
	float ret = 0.0;
	if (i < CV_COUNT) {
		// Input::inputMutex.lock();
		if (!LZX_MODE) {
			ret = cvIn[i];
		}
		else {
			ret = cvIn[i] * 5.;
		}
		// Input::inputMutex.unlock();
	}
	return ret;
}



std::vector<float> Input::getCVList(int index) {
	std::vector<float> ret;
	// Input::inputMutex.lock();
	ret = std::vector<float>(lastCV[index]);
	// Input::inputMutex.unlock();
	return ret;
}

// float Input::FilterInput(float in_val, BiquadChain* filter) { // simply applies filter designed in the contructor (dont forget to change the wonky gain)
// 	float output;
// 	int nItems = 1;
// 	filter->processBiquad(&in_val, &output, 1, nItems, &coeffs_[0]);
// 	output = output / 26.748586653; //5.4;//second order
// 	// if (cvNum == 3) {
// 	// 	std::cout << "Filter Output: " << output << " OG Smoothing: " << in_val <<  std::endl;
// 	// }
// 	output = (std::trunc(output * 1000.) / 1000.0);
// 	return output;
// }

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


void Input::ResetButtonLED() {
	for (int y = 0; y < BUTTON_COUNT; ++y) {
		setLEDState(y, buttons_[y]->GetState());
	}
}

void Input::SetButtonState(int index, int state) {
	buttons_[index]->SetState(state);
	setLEDState(index, state);
}


void Input::ReadButtons() { //assumes 3 buttons/RGB LEDs
	bool signal_change = false;
	bool buttons_states_changed = false;

	//Update All the buttons
	for (int x = 0; x < BUTTON_COUNT; ++x) {
		ButtonEvent event = buttons_[x]->Update();
		if (event != last_button_event_[x]) {
			buttons_states_changed = true;
		}
		last_button_event_[x] = event;
	}

	ButtonEvent event0 = last_button_event_[0];
	ButtonEvent event1 = last_button_event_[1];
	ButtonEvent event2 = last_button_event_[2];

	// DISPLAY CONTROL (menu leds)
	int held_count =  buttons_[0]->IsHeld() + buttons_[1]->IsHeld() + buttons_[2]->IsHeld();

	if (held_count == 1) {
		DisplayModMenu();
	}
	else if (held_count == 2) {
		DisplayPresetMenu();
	}
	else if (held_count == 3) { //Init Patch Button Combo
		//light up LED timer UI
		DisplayInitTimer();
		buttons_states_changed = true;
	}
	else {
		ResetButtonLED();
	}

	// else if (event0 != ButtonEvent::DOWN && event1 == ButtonEvent::DOWN && event2 == ButtonEvent::DOWN) {
	// 	ResetButtonLED();
	// }

	// if (event0 == ButtonEvent::DOWN && event1 == ButtonEvent::DOWN && event2 == ButtonEvent::DOWN) {
	// 	buttons_states_changed = true; // this overrride allows the loop to time while the init combo is pressed
	// }

	if (buttons_states_changed) { //only do this if the state changed in some way from last frame
		//check button states


		//This is basically a lookup table for all button menu states

		//NO EVENTS
		if (event0 == ButtonEvent::NONE && event1 == ButtonEvent::NONE && event2 == ButtonEvent::NONE) {
			//nothing happens reset all LEDs
			if (mod_menu_index_ != -1) {
				mod_menu_index_ = -1;
				preset_menu_index_ = -1;
				init_hold_time_ = -1.;
				ResetButtonLED();
			}
		}
		else { //something is changed
			signal_change = true; //make sure to message a state change later

			//mod menu display comes up only if you hold the button, otherwise macros are instant

			//simple UP events to switch mode
			if (event0 == ButtonEvent::UP && event1 == ButtonEvent::NONE && event2 == ButtonEvent::NONE) {
				setLEDState(0, buttons_[0]->ToggleState());
				if (onButton) {
					onButton(0, buttons_[0]->GetState());
				}
			}

			else if (event0 == ButtonEvent::NONE && event1 == ButtonEvent::UP && event2 == ButtonEvent::NONE) {
				setLEDState(1, buttons_[1]->ToggleState());
				if (onButton) {
					onButton(1, buttons_[1]->GetState());
				}
			}
			else if (event0 == ButtonEvent::NONE && event1 == ButtonEvent::NONE && event2 == ButtonEvent::UP) {
				setLEDState(2, buttons_[2]->ToggleState());
				if (onButton) {
					onButton(2, buttons_[2]->GetState());
				}
			}

			//mod menus
			else if ( event0 == ButtonEvent::DOWN && event1 == ButtonEvent::NONE && event2 == ButtonEvent::NONE ) {
				// mod menu index 1;
				EnterModMenu(0);
			}
			else if ( event0 == ButtonEvent::NONE && event1 == ButtonEvent::DOWN && event2 == ButtonEvent::NONE ) {
				// mod menu index 1;
				EnterModMenu(1);
			}
			else if ( event0 == ButtonEvent::NONE && event1 == ButtonEvent::NONE && event2 == ButtonEvent::DOWN ) {
				// mod menu index 1;
				EnterModMenu(2);
			}

			//Preset menu 0
			else if ( event0 == ButtonEvent::NONE && event1 == ButtonEvent::DOWN && event2 == ButtonEvent::DOWN ) {
				EnterPresetMenu(0);
			}
			//Preset menu 1
			else if ( event0 == ButtonEvent::DOWN && event1 == ButtonEvent::NONE && event2 == ButtonEvent::DOWN ) {
				EnterPresetMenu(1);
			}
			//Preset menu 2
			else if ( event0 == ButtonEvent::DOWN && event1 == ButtonEvent::DOWN && event2 == ButtonEvent::NONE ) {
				EnterPresetMenu(2);
			}

			//1st mod menu
			else if ( event0 == ButtonEvent::DOWN && event1 == ButtonEvent::UP && event2 == ButtonEvent::NONE ) {
				Input::ToggleModMenu(0, 1);
			}

			else if ( event0 == ButtonEvent::DOWN && event1 == ButtonEvent::NONE && event2 == ButtonEvent::UP ) {
				Input::ToggleModMenu(0, 2);
			}
			//2nd mod menu
			else if ( event0 == ButtonEvent::UP && event1 == ButtonEvent::DOWN && event2 == ButtonEvent::NONE ) {
				Input::ToggleModMenu(1, 0);
			}
			else if ( event0 == ButtonEvent::NONE && event1 == ButtonEvent::DOWN && event2 == ButtonEvent::UP) {
				Input::ToggleModMenu(1, 2);
			}
			//3rd mod menu
			else if ( event0 == ButtonEvent::UP && event1 == ButtonEvent::NONE && event2 == ButtonEvent::DOWN ) {
				Input::ToggleModMenu(2, 0);
			}
			else if ( event0 == ButtonEvent::NONE && event1 == ButtonEvent::UP && event2 == ButtonEvent::DOWN) {
				Input::ToggleModMenu(2, 1);
			}

			//preset menu conditions
			else if ( event0 == ButtonEvent::UP && event1 == ButtonEvent::DOWN && event2 == ButtonEvent::DOWN ) {
				TogglePresetMenu(0);
			}

			else if ( event0 == ButtonEvent::DOWN && event1 == ButtonEvent::UP && event2 == ButtonEvent::DOWN ) {
				TogglePresetMenu(1);
			}


			else if ( event0 == ButtonEvent::DOWN && event1 == ButtonEvent::DOWN && event2 == ButtonEvent::UP ) {
				TogglePresetMenu(2);
			}

			// Clear/Init Patch ( May need a time delay here )
			else if ( event0 == ButtonEvent::DOWN  && event1 == ButtonEvent::DOWN && event2 == ButtonEvent::DOWN ) {
				// if(buttons_[0]->IsHeld() && buttons_[1]->IsHeld() && buttons_[2]->IsHeld()){
				if (init_hold_time_ == -1.) {
					init_hold_time_ = Util::TimeInSeconds();
					// std::cout << "start hold init" << std::endl;
				}
				else if (init_hold_time_ > 0.) {

					int this_hold_time = Util::TimeInSeconds() - init_hold_time_;

					if (this_hold_time >= INIT_HOLD_TIME) {
						if (onPresetHold) {
							onPresetHold(preset_menu_index_);
						}
						init_hold_time_ = -2.;
					}
				}



				// }
			}


		}
	}
}

void Input::EnterModMenu(int index) {
	preset_menu_index_ = -1;
	mod_menu_index_ = index;
	init_hold_time_ = -1.;
}

void Input::DisplayModMenu() {
	for (int x = 0; x < RGB_LED_COUNT; ++x) {
		if (x == mod_menu_index_) {
			SetLEDColor(x, glm::vec3( 1.0 ));
		}
		else {
			setLEDState(x, button_mod_matrix_[mod_menu_index_][x]);
		}
	}
}

void Input::EnterPresetMenu(int index) {
	mod_menu_index_ = -1;
	preset_menu_index_ = index;
	init_hold_time_ = -1.;
}

void Input::DisplayPresetMenu() {
	//index represents led not held in preset menu (the other 2 buttons are held)
	//Set all held leds to white
	// std::cout << "Displaying Preset Menu: " << preset_menu_index_ << std::endl;
	for (int x = 0; x < RGB_LED_COUNT; ++x) {
		if (x == preset_menu_index_) {
			float this_preset_setting = preset_menu_[preset_menu_index_];
			if (this_preset_setting == 1.0) { //has preset saved
				SetLEDColor(x, glm::vec3(0., 1., 0.), false );
			}
			else {
				SetLEDColor(x, glm::vec3(1., 0., 0.), false );
			}
		}
		else {
			SetLEDColor(x, glm::vec3( 1.0 ));
		}
	}
}

void Input::DisplayInitTimer() {
	float this_hold_time = Util::TimeInSeconds() - init_hold_time_;
	if (this_hold_time <= INIT_HOLD_TIME ) {
		int leds_lit = (int)this_hold_time % ( (INIT_HOLD_TIME ) ) + 1;
		// std::cout << leds_lit << std::endl;
		if (leds_lit < 4) {
			SetLEDColor(0, glm::vec3(leds_lit) );
			SetLEDColor(1, glm::vec3(leds_lit - 1.) );
			SetLEDColor(2, glm::vec3(leds_lit - 2.) );
		}
		else {
			SetLEDColor(0, glm::vec3(1., 1., 0.) );
			SetLEDColor(1, glm::vec3(1., 1., 0.) );
			SetLEDColor(2, glm::vec3(1., 1., 0.) );
		}
	}
	else {
		//all green indicates success full save
		SetLEDColor(0, glm::vec3(0., 1., 0.) );
		SetLEDColor(1, glm::vec3(0., 1., 0.) );
		SetLEDColor(2, glm::vec3(0., 1., 0.) );
	}
}



void Input::ToggleModMenu(int mod_menu_index, int mod_destination_index ) {
	button_mod_matrix_[mod_menu_index][mod_destination_index] += 1;
	if (button_mod_matrix_[mod_menu_index][mod_destination_index] > 1) {
		button_mod_matrix_[mod_menu_index][mod_destination_index] = 0;
	}
	// std::cout<< "TOGGLED" << std::endl;
	DisplayModMenu();
	// setLEDState(mod_destination_index, button_mod_matrix_[mod_menu_index][mod_destination_index]);

	if (onModMatrix) {
		onModMatrix(button_mod_matrix_);
	}

	if (onModMatrixValue) {
		onModMatrixValue(mod_menu_index, mod_destination_index,  button_mod_matrix_[mod_menu_index][mod_destination_index]);
	}
}

void Input::TogglePresetMenu(int index) {
	// std::cout<< "Toggled Preset Menu: " << index << std::endl;
	if (index < 3) {
		if (preset_menu_[index] == 0.0) {
			preset_menu_[index] = 1.0;
		}
		else if (preset_menu_[index] == 1.0) {
			preset_menu_[index] = 0.0;
		}

		if (onPresetToggle) {
			onPresetToggle(index);
		}
	}
}

bool Input::ToggleLZXMode() {
	LZX_MODE = !LZX_MODE;
	return LZX_MODE;
}


bool Input::IsButtonHeld(int index) {
	return buttons_[index]->IsHeld();
}

void Input::setLEDState(int index, int state) {
	if (index < RGB_LED_COUNT) {
		setLEDState(LED_PIN[index][0], LED_PIN[index][1], LED_PIN[index][2], state);
	}
}

void Input::setLEDState(int pinR, int pinG, int pinB, int state) {
	state += 1;
	setLEDState(pinR, pinG, pinB, glm::vec3((float)(bool)(state & 1), (float)(bool)(state & 2), (float)(bool)(state & 4)) );
}

void Input::setLEDState(int pinR, int pinG, int pinB, glm::vec3 c) {
	// std::cout <<"Set LED color: " << c.r << " , " << c.g << " , " << c.b << std::endl;

	//dim leds
	// float clip = 0.25;
	// if( c.r > clip ) c.r = clip;
	// if( c.g > clip ) c.g = clip;
	// if( c.b > clip ) c.b = clip;
	c /= 4.; //dim leds

	//intensity is reversed due to hardware wiring (LED with common Anode)


	if (USE_LED_PWM) {
		c.r = 1.0 - c.r;
		c.g = 1.0 - c.g;
		c.b = 1.0 - c.b;

		softPwmWrite (pinR, c.r * SOFTPWM_RANGE);
		softPwmWrite (pinG, c.g * SOFTPWM_RANGE);
		softPwmWrite (pinB, c.b * SOFTPWM_RANGE);
	}
	else {
		digitalWrite(pinR, ! (c.r > 0) );
		digitalWrite(pinG, ! (c.g > 0));
		digitalWrite(pinB, ! (c.b > 0));
	}
}

void Input::SetLEDColor(int index, glm::vec3 color, bool freeze) {
	if (index < RGB_LED_COUNT) {
		color = (color * glm::vec3(.8)); //exagerate color on LEDs
		setLEDState(LED_PIN[index][0], LED_PIN[index][1], LED_PIN[index][2], color);

		//TODO: Implement UI going back to button states after a set freeze time ( Currently not in use )
		if (freeze) {
			//freeze LED readout for duration
			led_freeze_time_[index] = (float) clock() / (CLOCKS_PER_SEC);
		}
	}
}

void Input::setPotLEDState(int index, bool state ) {
	if (index == 0) {
		digitalWrite(POT1_LED_PIN, state);
	}
	else {
		digitalWrite(POT3_LED_PIN, state);
	}
}
void Input::setPotLEDState(int index, float intensity ) {
	if (USE_POT_LED_PWM) {
		if (index == 0) {
			softPwmWrite(POT1_LED_PIN, intensity * SOFTPWM_RANGE);
		}
		else {
			softPwmWrite(POT3_LED_PIN, intensity * SOFTPWM_RANGE);
		}
	}
	else {
		//not supported
	}
}


#endif
