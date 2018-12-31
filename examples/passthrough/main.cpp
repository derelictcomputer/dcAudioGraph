//
// Created by Charlie Huguenard on 12/29/2018.
//

#include "../portaudio/include/portaudio.h"
#include "../../dcAudioGraph/Graph.h"

dc::AudioBuffer inBuf, outBuf;

int audioDeviceCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
    const float* inSamples = static_cast<const float*>(inputBuffer);
    float* outSamples = static_cast<float*>(outputBuffer);

    dc::Graph* graph = static_cast<dc::Graph*>(userData);
    inBuf.fromInterleaved(inSamples, framesPerBuffer, graph->getNumAudioInputs(), true);
    outBuf.fromInterleaved(outSamples, framesPerBuffer, graph->getNumAudioOutputs(), true);
    graph->process(inBuf, outBuf);
	outBuf.toInterleaved(outSamples, framesPerBuffer, graph->getNumAudioOutputs());
	return paContinue;
}

void printError(PaError error)
{
    fprintf(stderr, "An error occurred while using the PortAudio stream\n");
    fprintf(stderr, "Error number: %d\n", error);
    fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(error));
}

int main()
{
    PaError error = Pa_Initialize();
    if (error != paNoError)
    {
        printError(error);
        return error;
    }

    PaStreamParameters inputParams;
    inputParams.device = Pa_GetDefaultInputDevice();
    if (paNoDevice == inputParams.device)
    {
        fprintf(stderr,"Error: No default input device.\n");
        return 1;
    }
    inputParams.channelCount = 2;
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    PaStreamParameters outputParams;
    outputParams.device = Pa_GetDefaultOutputDevice();
    if (paNoDevice == outputParams.device)
    {
        fprintf(stderr,"Error: No default output device.\n");
        return 1;
    }
    outputParams.channelCount = 2;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    dc::Graph graph;
    graph.init(64, 48000);
    graph.setNumAudioInputs(2);
    graph.setNumAudioOutputs(2);
    dc::Module::connectAudio(graph.getAudioInput(), 0, graph.getAudioOutput(), 0);
    dc::Module::connectAudio(graph.getAudioInput(), 1, graph.getAudioOutput(), 1);

    PaStream* stream;
    error = Pa_OpenStream(&stream, &inputParams, &outputParams, 48000, 64, 0, audioDeviceCallback, &graph);
    if (error != paNoError)
    {
        printError(error);
        return error;
    }

    error = Pa_StartStream(stream);
    if (error != paNoError)
    {
        printError(error);
        return error;
    }

    printf("Press any key to stop\n");
    getchar();

    error = Pa_CloseStream(stream);
    if (error != paNoError)
    {
        printError(error);
        return error;
    }

    Pa_Terminate();
    return 0;
}
