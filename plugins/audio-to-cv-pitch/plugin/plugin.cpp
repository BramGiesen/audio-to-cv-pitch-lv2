#include "plugin.hpp"

START_NAMESPACE_DISTRHO


// -----------------------------------------------------------------------

AudioToCVPitch::AudioToCVPitch()
    : Plugin(paramCount, 0, 0)
{
    aubio = &pitchDetector;
    aubio->setBuffersize(getBufferSize());
    aubio->setHopfactor(8);
    aubio->setSamplerate(getSampleRate());
    pitchDetector.setPitchMethod("yinfast");
    pitchDetector.setSilenceThreshold(-30.0f);
    pitchDetector.setPitchOutput("Hz");

    sensitivity = 1.0;
    octave = 0;
}

AudioToCVPitch::~AudioToCVPitch()
{
}

// -----------------------------------------------------------------------
// Init

void AudioToCVPitch::initParameter(uint32_t index, Parameter& parameter)
{
    switch (index)
    {
        case paramSensitivity:
            parameter.hints = kParameterIsAutomable;
            parameter.name = "Sensitivity";
            parameter.symbol = "Sensitivity";
            parameter.ranges.def = 120.f;
            parameter.ranges.min = 0.1f;
            parameter.ranges.max = 3.f;
            break;
        case paramOctave:
            parameter.hints = kParameterIsAutomable | kParameterIsInteger;
            parameter.name = "Octave";
            parameter.symbol = "Octave";
            parameter.ranges.def = 0;
            parameter.ranges.min = -3;
            parameter.ranges.max = 3;
            break;
    }
}

// -----------------------------------------------------------------------
// Internal data

float AudioToCVPitch::getParameterValue(uint32_t index) const
{
    switch (index)
    {
        case paramSensitivity:
            return sensitivity;
        case paramOctave:
            return octave;
    }
}

void AudioToCVPitch::setParameterValue(uint32_t index, float value)
{
    switch (index)
    {
        case paramSensitivity:
            sensitivity = value;
            break;
        case paramOctave:
            octave = static_cast<int>(value);
            break;
    }
}

// -----------------------------------------------------------------------
// Process

void AudioToCVPitch::activate()
{
}

void AudioToCVPitch::deactivate()
{
}

void AudioToCVPitch::run(const float** inputs, float** outputs, uint32_t numFrames)
{
    float inputBuffer[1024];
    float *input;
    input = inputBuffer;

    for (unsigned f = 0; f < numFrames; f++) {
        input[f] = inputs[0][f] * sensitivity;
    }

    float detectedPitchInHz = aubio->process((float*)input);
    float linearPitch = (detectedPitchInHz > 0.0) ? (12*log2(detectedPitchInHz / 440.0) + 69.0) + (12 * octave) : 0.0;
    float cvPitch = ((float)linearPitch * (1/12.0f));

    //clip pitch between output range of 0 to 10 volt
    cvPitch = (cvPitch < 0.0) ? 0.0 : cvPitch;
    cvPitch = (cvPitch > 10.0) ? 10.0 : cvPitch;

    for (unsigned f = 0; f < numFrames; f++) {
        outputs[0][f] = cvPitch;
    }
}



// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new AudioToCVPitch();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
