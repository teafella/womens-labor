#ifndef AUDIO_CPP
#define AUDIO_CPP


#include <stdio.h>
#include <math.h>
#include <iostream>
#include <cstring>
#include <time.h>
#include <vector>
#include <algorithm>

#include "audio.h"
#include "input.h"
#include "src/util.h"


class ScopedPaHandler
{
public:
	ScopedPaHandler()
		: _result(Pa_Initialize())
	{
	}
	~ScopedPaHandler()
	{
		if (_result == paNoError)
		{
			Pa_Terminate();
		}
	}

	PaError result() const { return _result; }

private:
	PaError _result;
};

Audio::Audio(): stream(0), left_phase(0), right_phase(0) {
	//dont do anything here ( unless you check for duplicate threads later)
	// createWaveTable();
	// //start the audio thread (basically just sleeps while audio runs in the background, not sure how to do this correctly)
	// thread = std::thread(&Audio::run, this );

}

Audio::Audio(Input* in) {
	inputs = in;
	spectrum_ = std::vector<float>(SPECTRUM_SIZE);
	mod_phases = std::vector<float>(SPECTRUM_SIZE);

	//set up callbacks
	in->SetOnCallback(std::bind(&Audio::SetOn, this, std::placeholders::_1));
	in->SetPitchCallback(std::bind(&Audio::SetPitch, this, std::placeholders::_1));
	in->SetSpectrumCallback(std::bind(&Audio::SetSpectrum, this, std::placeholders::_1, std::placeholders::_2));

	createWaveTable();

	//start the audio thread (basically just sleeps while audio runs in the background, not sure how to do this correctly)
	thread_ = std::thread(&Audio::run, this );
}

Audio::~Audio() {
	stop();
	close();
	thread_.join();
}

void Audio::createWaveTable() {
	/* initialise sinusoidal wavetable */ //replace this with something more interesting
	for ( int i = 0; i < TABLE_SIZE; i++ )
	{
		sine[i] = (float) sin(  (double)i / (double)TABLE_SIZE  * M_PI * 2. )  ;
	}
}

void Audio::loadByteWaveTable(unsigned char* table, int size) {
	loadedWaveTable = table;
	loadedWaveTableSize = size;
}


bool Audio::run() {
	//set up last frame buffer last write location

	ScopedPaHandler paInit;
	if ( paInit.result() != paNoError ) {
		fprintf( stderr, "An error occured while using the portaudio stream\n" );
		fprintf( stderr, "Error number: %d\n", paInit.result() );
		fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( paInit.result() ) );
		return false;
	}

	if (open(Pa_GetDefaultOutputDevice()))
	{
		printf("Playing a sin wave\n" );

		if (start()) {

			while (1) {};
		}
	}
	return true;

}



bool Audio::open(PaDeviceIndex index)
{
	PaStreamParameters outputParameters;

	outputParameters.device = index;
	if (outputParameters.device == paNoDevice) {
		return false;
	}

	const PaDeviceInfo* pInfo = Pa_GetDeviceInfo(index);
	if (pInfo != 0)
	{
		printf("Output device name: '%s'\r", pInfo->name);
	}

	outputParameters.channelCount = 2;       /* stereo output */
	outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
	outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	PaError err = Pa_OpenStream(
	                  &stream,
	                  NULL, /* no input */
	                  &outputParameters,
	                  SAMPLE_RATE,
	                  //paFramesPerBufferUnspecified,
	                  FRAMES_PER_BUFFER,
	                  0,      /* we won't output out of range samples so don't bother clipping them */
	                  &Audio::paCallback,
	                  this            /* Using 'this' for userData so we can cast to Audio* in paCallback method */
	              );

	if (err != paNoError)
	{
		/* Failed to open stream to device !!! */
		return false;
	}

	err = Pa_SetStreamFinishedCallback( stream, &Audio::paStreamFinished );

	if (err != paNoError)
	{
		Pa_CloseStream( stream );
		stream = 0;

		return false;
	}

	return true;
}

bool Audio::close()
{
	if (stream == 0)
		return false;

	PaError err = Pa_CloseStream( stream );
	stream = 0;

	return (err == paNoError);
}


bool Audio::start()
{
	if (stream == 0)
		return false;

	PaError err = Pa_StartStream( stream );
	begin = clock();

	return (err == paNoError);
}

bool Audio::stop()
{
	if (stream == 0)
		return false;

	PaError err = Pa_StopStream( stream );

	return (err == paNoError);
}

//port audio specific handlers
int Audio::paCallbackMethod(const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags)
{
	float *out = (float*)outputBuffer;

	//int thisTableSize = TABLE_SIZE;
	int thisTableSize = TABLE_SIZE;

	unsigned long i;

	(void) timeInfo; /* Prevent unused variable warnings. */
	(void) statusFlags;
	(void) inputBuffer;

	createWaveTable();

	float hertz = 65.41;//32.70; // C1

	float carrierOctaveRange = 8.;
	float modOctaveRange = 8.;

	float carrier_pitch = (pitch_/1800.0);
	float playSpeed = (thisTableSize / (float)SAMPLE_RATE * hertz) * (pow(2, (carrier_pitch * (carrierOctaveRange * 12.0)) / 12. ) ); //(quantized)don't cast if you want free frequency control

	std::vector<float> mod_speeds = std::vector<float>(SPECTRUM_SIZE);
	for ( int x = 0; x < SPECTRUM_SIZE; ++x){
		float this_ratio = GetSpectrum(x);
		mod_speeds[x] = playSpeed * (pow(2, (int)(this_ratio * (modOctaveRange * 12)) / 12. ) );
	}

	float NUM_MODULATORS = 3;

	if (thisTableSize != 0) {
		for ( i = 0; i < framesPerBuffer; i++ )
		{
			for ( int x = 0; x < NUM_MODULATORS; ++x){
				mod_phases[x] += mod_speeds[x];
				while ( mod_phases[x] >= thisTableSize ) mod_phases[x] -= thisTableSize;
			}
			
			carrier_phase += playSpeed;
			if (carrier_phase < 0) carrier_phase = 0;
			while ( carrier_phase >= thisTableSize ) carrier_phase -= thisTableSize;

			out_phase = carrier_phase;
			// for ( int x = 0; x < NUM_MODULATORS; ++x){
			// 	out_phase *= sine[ (int)mod_phases[x] ];
			// 	if (out_phase < 0) out_phase = 0;
			// 	while ( out_phase >= thisTableSize ) out_phase -= thisTableSize;
			// }
			
			if (on_) {
				*out++ = tanh(sine[(int)out_phase]);  /* left */
				float left = *out;
				*out++ = tanh(sine[(int)out_phase]);  /* right */
				float right = *out;
			}
			else{
				*out++ = 0;  /* left */
				float left = *out;
				*out++ = 0;  /* right */
				float right = *out;
			}

		}
	}
	else {
		std::cout << "WARNING: No Wavetable Loaded." << std::endl;
	}

	return paContinue;
}


void Audio::paStreamFinishedMethod()
{
	printf( "Stream Completed: %s\n", message );
}


void Audio::SetOn(bool val) {
	// std::cout<< "set on in audio: " << val << std::endl;
	on_ = val;
}
void Audio::SetPitch(unsigned int val) {
	// std::cout<< "set pitch in audio: " << val << std::endl;
	pitch_ = val;
}
void Audio::SetSpectrum(int index, float val) {
	if (spectrum_mutex_.try_lock()) {
		// std::cout<< "set spectrum in audio: "<< index<< " " << val << std::endl;
		spectrum_[index] = val;
		spectrum_mutex_.unlock();
	}
}


float Audio::GetSpectrum(int index){
	float ret = 0.0;
	if (spectrum_mutex_.try_lock()) {
		ret = spectrum_[index];
		spectrum_mutex_.unlock();
	}
	return ret;
}

#endif