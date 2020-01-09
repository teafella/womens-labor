#ifndef AUDIO_H
#define AUDIO_H

#include "lib/portaudio.h"
#include <thread>
#include <mutex>
#include <atomic>
#include <list>
#include <vector>
#include <time.h>

#define SAMPLE_RATE   (192000)
#define FRAMES_PER_BUFFER   (256)
#define TABLE_SIZE   (800)

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

#define RECORD_BUFFER  (2560)

// dont forget to sudo apt-get install libasound2-dev

class Input;

class Audio
{
public:
	Audio();
	Audio(Input* in);
	~Audio();

	bool open(PaDeviceIndex index);
	bool close();
	bool start();
	bool stop();
	bool run();

	void loadByteWaveTable(unsigned char* table, int size);

	//input get/set for callbacks
	void SetOn(bool val);
	void SetPitch(unsigned int val);
	void SetSpectrum(int index, float val);
	float GetSpectrum(int index);


private:
	std::thread thread_;
	void createWaveTable();

	int paCallbackMethod(const void *inputBuffer, void *outputBuffer,
	                     unsigned long framesPerBuffer,
	                     const PaStreamCallbackTimeInfo* timeInfo,
	                     PaStreamCallbackFlags statusFlags);

	/* This routine will be called by the PortAudio engine when audio is needed.
	** It may called at interrupt level on some machines so don't do anything
	** that could mess up the system like calling malloc() or free().
	*/
	static int paCallback( const void *inputBuffer, void *outputBuffer,
	                              unsigned long framesPerBuffer,
	                              const PaStreamCallbackTimeInfo* timeInfo,
	                              PaStreamCallbackFlags statusFlags,
	                              void *userData )
	{
		/* Here we cast userData to Sine* type so we can call the instance method paCallbackMethod, we can do that since
		   we called Pa_OpenStream with 'this' for userData */
		return ((Audio*)userData)->paCallbackMethod(inputBuffer, outputBuffer,
		        framesPerBuffer,
		        timeInfo,
		        statusFlags);
	}

	void paStreamFinishedMethod();
	/*
	* This routine is called by portaudio when playback is done.
	*/
	static void paStreamFinished(void* userData)
	{
		return ((Audio*)userData)->paStreamFinishedMethod();
	}

	Input* inputs;

	PaStream *stream;
	float sine[TABLE_SIZE];
	unsigned char* loadedWaveTable;
	int loadedWaveTableSize = 0;

	float carrier_phase;
	std::vector<float> mod_phases;
	float left_phase;
	float right_phase;
	float out_phase;
	char message[20];

	clock_t begin;

	//Input Params
	std::atomic<bool> on_;
	std::atomic<unsigned int> pitch_;
	std::vector<float> spectrum_;
	std::mutex spectrum_mutex_;

};

#endif