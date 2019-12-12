#ifndef INPUT_H
#define INPUT_H

#include <thread>
#include <pthread.h>
#include <mutex>
#include <functional>
#include <vector>
#include <memory>
#include "lib/glm/glm.hpp"
#include "src/hardware/button.h"

// #include "src/dsp/Butterworth.h"

#define SWITCH_COUNT 3
#define RGB_LED_COUNT 3
#define CV_LIST_SIZE 400
#define CV_COUNT 11
#define POT_COUNT 8
#define BUTTON_COUNT 3

//forward declartion
class Midi;
class NoteMessage;
class Button;

class Input
{
public:
	Input(bool run_tests = false);
	~Input();

	void Init();
	void update();
	void AddPresetHoldCallback(std::function<void(int)> presetHoldCallback);
	void AddPresetToggleCallback(std::function<void(int)> presetToggleCallback);
	void AddButtonCallback(std::function<void(int, int)> buttonCallback);
	void AddCVCallback(std::function<void(int, float)> cvCallback);
	void AddAltPotCallback(std::function<void(int, int, float)> potCallback);
	void AddClockCallback(std::function<void(void)> cvCallback);
	void AddModMatrixCallback(std::function<void(glm::mat3)> modCallback);
	void AddModMatrixValueCallback(std::function<void(int, int, float)> modValueCallback);

	std::vector<float> getCVList(int index);

	int GetCVCount();
	int GetPotCount();
	int GetButtonCount();
	
	//shouldnt really be called unless debugging (not thread safe)
	void setCV (int index, float val); //will be overwritten by hardware input if present

	int getSwitch(int index);
	int getButtonState(int index);
	void SetButtonState(int index, int state);

	//Clock ( Cut from Hypno )
	// void WriteClock();
	// void WriteClock(bool val);
	// void Tick();
	// float GetClockScale();

	// LED Controll
	void SetLEDColor(int index, glm::vec3 color, bool freeze = false);
	void setPotLEDState(int index, bool state);
	void setPotLEDState(int index, float intensity);


	//button ext control
	void ToggleButton(int index);

	//Button Control
	bool IsButtonHeld(int index);

	std::thread inputThread;
	std::thread osc_thread_;
	std::thread save_thread_;
	std::thread clock_thread_;
	std::mutex input_mutex_;
	// std::mutex save_mutex;

	bool LZX_MODE = false;
	bool ToggleLZXMode();

	//not a strict singleton but gets last instatiated instance (im lazy)
	static Input* singleton_;

private:

	bool readADC();

	std::string savefile_name_ = "state";
	bool save_thread_running_ = false;

	//Acessors are provided but best to use the callback when possible
	//Get and Set methods should only be called from the inputThread 
	
	float getCV(int index);
	void  SetPot(int index, float val, bool ext_control_change = false);
	float getPot(int index);
	float GetAltPot(int button_index, int pot_index);
	void SetAltPot(int button_index, int pot_index, float value);
	bool SetModMatrixValue(int x, int y, float value);

	bool adc_initialized_ = false;

	int serialFd;
	float cvIn[CV_COUNT] = {0.0};
	float potIn[POT_COUNT] = {0.0};
	std::vector<float> lastCV[CV_COUNT] = {std::vector<float>(CV_LIST_SIZE, 0.0), std::vector<float>(CV_LIST_SIZE, 0.0), std::vector<float>(CV_LIST_SIZE, 0.0) , std::vector<float>(CV_LIST_SIZE, 0.0) , std::vector<float>(CV_LIST_SIZE, 0.0) , std::vector<float>(CV_LIST_SIZE, 0.0) , std::vector<float>(CV_LIST_SIZE, 0.0) , std::vector<float>(CV_LIST_SIZE, 0.0) };
	float lastCVRead[CV_COUNT] = {0.};
	float lastRawCV[CV_COUNT] = {255};
	float lastSmoothCV[CV_COUNT] = {255}; // set to middle value for bipolar (helps the smoother start correctly)
	int lastSmoothPot[POT_COUNT] = {0};
	float lastRawPot[CV_COUNT] = {0};

	//Clocking
	float clock_scale_ = 0.; //in Hz (per sec)
	float last_clock_scale_ = 1.;
	int last_trig_readA_ = 1.;
	int last_trig_readB_ = 1.;
	int last_clock_write_ = 0.;
	clock_t last_clock_tick_time_ = 0;

	//Callbacks
	std::function<void(int, int)> onButton = 0;
	std::function<void(int, int, float)> onAltPot = 0;
	std::function<void(int, float)> onCV = 0;
	std::function<void(int)> onPresetHold = 0;
	std::function<void(int)> onPresetToggle = 0;
	std::function<void(glm::mat3)> onModMatrix = 0;
	std::function<void(int, int, float)> onModMatrixValue = 0;
	std::function<void(void)> onClock = 0;

	//Buttons And Mod Menu
	std::unique_ptr<Button> buttons_[BUTTON_COUNT];
	ButtonEvent last_button_event_[BUTTON_COUNT];
	int mod_menu_index_ = -1;
	int preset_menu_index_ = -1;
	glm::vec3 preset_menu_ = glm::vec3(0.0);
	glm::mat4 button_mod_matrix_ = glm::mat4(0.0);
	//Read buttons helpers
	void EnterModMenu(int index);
	void DisplayModMenu();
	void EnterPresetMenu(int index);
	void DisplayPresetMenu();
	void DisplayInitTimer();

	void ToggleModMenu(int mod_menu_index, int mod_destination_index );
	void TogglePresetMenu(int index);

	
	int INIT_HOLD_TIME = 3;
	float init_hold_time_ = -1.;


	//Alternate Pot (with button modifiers/shift)
	float alt_pot_[BUTTON_COUNT][POT_COUNT] = {{-2.0}, {-2.0}, {-2.0}};
	float last_alt_pot_[POT_COUNT] = {-1.0};
	float last_pot_[POT_COUNT] = {-1.0};
	bool alt_pot_active_[BUTTON_COUNT][POT_COUNT] = {false};

	void ReadButtons();
	void ReadTriggers();


	//OSC
	bool setupOSC();
	bool readOSC();
	bool SendOSCTest();

	//MIDI
	Midi* myMIDI = 0;
	bool setupMIDI(int port);
	static void midiNoteCallback(NoteMessage* msg);
	static void midiCCCallback(int channel, int index, float value);

	//SERIAL
	bool setupSerial();
	bool readSerial();

	//LED
	void setLEDState(int index, int state);
	void setLEDState(int pinR, int pinG, int pinB, int state);
	void setLEDState(int pinR, int pinG, int pinB, glm::vec3 c);
	void ResetButtonLED();

	float led_freeze_duration_ = 2.; //in seconds
	float led_freeze_time_[RGB_LED_COUNT] = {0.0};

	bool InitADC();
	bool StartADCThread(bool run_tests = false);

	void SetupHardwarePins();
	
	int AnalogReadMuxedValue(int value_index, int adc_address, int mux_address_pinA, int mux_address_pinB, int mux_address_pinC );
	static float smooth(float in, float PrevSmoothVal, float PrevRawVal, double weight = 0.5);
	// float FilterInput(float in, BiquadChain* filter);

	//testing and qa
	void TestHardwareConfig();

	bool threadRunning = false;

	//filter vars
	// std::vector<Biquad> coeffs_;  // array of biquad filters for every input (for this case, array size = 4 )
	// Butterworth filter_model_;
	// std::vector<BiquadChain> pot_filters_;
	// std::vector<BiquadChain> cv_filters_;

	float sample_begin_;
	float sample_end_;
	double last_sample_time_;
	float max_filter_out_ = 0.;

	//Save Timing 
	float save_time;
	float save_begin;
	bool save_parameters = 1;


};


#endif
