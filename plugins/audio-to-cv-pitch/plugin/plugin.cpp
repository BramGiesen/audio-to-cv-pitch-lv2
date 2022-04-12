#include "plugin.hpp"

START_NAMESPACE_DISTRHO

// use a minimum value in order to keep cpu usage low
static constexpr const uint32_t kMinimumAubioBufferSize = 2048;

// -----------------------------------------------------------------------

AudioToCVPitch::AudioToCVPitch()
    : Plugin(paramCount, 0, 0),
      aubio(pitchDetector)
{
    aubio.setHopfactor(1);
    aubio.setSamplerate(getSampleRate());

    inputBufferSize = std::max(getBufferSize(), kMinimumAubioBufferSize);
    aubio.setBuffersize(inputBufferSize);

    pitchDetector.setPitchMethod("yinfast");
    pitchDetector.setSilenceThreshold(-30.0f);
    pitchDetector.setPitchOutput("Hz");

    lastKnownPitchLinear = 0.0f;
    lastKnownPitchInHz = 0.0f;
    lastKnownPitchConfidence = 0.0f;

    sensitivity = 1.0f;
    threshold = 0.0f;
    octave = 0;
    holdOutputPitch = true;

    inputBufferPos = 0;
    inputBuffer = new float[inputBufferSize];
}

AudioToCVPitch::~AudioToCVPitch()
{
    delete[] inputBuffer;
}

// -----------------------------------------------------------------------
// Init

void AudioToCVPitch::initAudioPort(bool input, uint32_t index, AudioPort& port)
{
    if (input)
        return Plugin::initAudioPort(input, index, port);

    switch (index)
    {
        case outputPitch:
            port.name   = "Pitch Out";
            port.symbol = "PitchOut";
            port.hints  = kAudioPortIsCV | kCVPortHasPositiveUnipolarRange | kCVPortHasScaledRange;
            break;
        case outputSignal:
            port.name   = "Signal Out";
            port.symbol = "SignalOut";
            port.hints  = kAudioPortIsCV | kCVPortHasPositiveUnipolarRange | kCVPortHasScaledRange;
            break;
    }
}

void AudioToCVPitch::initParameter(uint32_t index, Parameter& parameter)
{
    switch (index)
    {
        case paramSensitivity:
            parameter.hints = kParameterIsAutomatable;
            parameter.name = "Sensitivity";
            parameter.symbol = "Sensitivity";
            parameter.unit = "%";
            parameter.ranges.def = 4.f;
            parameter.ranges.min = 0.1f;
            parameter.ranges.max = 100.f;
            break;
        case paramConfidenceThreshold:
            parameter.hints = kParameterIsAutomatable;
            parameter.name = "Confidence Threshold";
            parameter.symbol = "ConfidenceThreshold";
            parameter.unit = "%";
            parameter.ranges.def = 0.f;
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 100.f;
            break;
        case paramOctave:
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger;
            parameter.name = "Octave";
            parameter.symbol = "Octave";
            parameter.ranges.def = 0;
            parameter.ranges.min = -4;
            parameter.ranges.max = 4;
            break;
        case paramHoldOutputPitch:
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger | kParameterIsBoolean;
            parameter.name = "Hold output pitch";
            parameter.symbol = "HoldOutputPitch";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 1;
            break;
        case paramDetectedPitch:
            parameter.hints = kParameterIsAutomatable | kParameterIsOutput;
            parameter.name = "Detected Pitch";
            parameter.symbol = "DetectedPitch";
            parameter.unit = "Hz";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 22050;
            break;
        case paramPitchConfidence:
            parameter.hints = kParameterIsAutomatable | kParameterIsOutput;
            parameter.name = "Pitch Confidence";
            parameter.symbol = "PitchConfidence";
            parameter.unit = "%";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 100;
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
        case paramConfidenceThreshold:
            return threshold * 100.f;
        case paramOctave:
            return octave;
        case paramHoldOutputPitch:
            return holdOutputPitch ? 1.0f : 0.0f;
        case paramDetectedPitch:
            return lastKnownPitchInHz;
        case paramPitchConfidence:
            return lastKnownPitchConfidence * 100.f;
        default:
            return 0.0f;
    }
}

void AudioToCVPitch::setParameterValue(uint32_t index, float value)
{
    switch (index)
    {
        case paramSensitivity:
            sensitivity = value;
            break;
        case paramConfidenceThreshold:
            threshold = value * 0.01f;
            break;
        case paramOctave:
            octave = static_cast<int>(value + 0.5f); // round up
            break;
        case paramHoldOutputPitch:
            holdOutputPitch = value > 0.5f;
            break;
    }
}

// -----------------------------------------------------------------------
// Process

void AudioToCVPitch::activate()
{
    inputBufferPos = 0;
}

void AudioToCVPitch::deactivate()
{
}

void AudioToCVPitch::run(const float** inputs, float** outputs, uint32_t numFrames)
{
    if (d_isEqual(sensitivity, 1.0f))
    {
        std::memcpy(inputBuffer + inputBufferPos, inputs[0], sizeof(float)*numFrames);
    }
    else
    {
        // TODO replace with faster SSE/NEON multiply and assign
        for (uint32_t i = 0; i < numFrames; ++i)
            inputBuffer[inputBufferPos + i] = inputs[0][i] * sensitivity;
    }

    inputBufferPos += numFrames;

    float cvPitch, cvSignal;

    if (inputBufferPos >= inputBufferSize)
    {
        inputBufferPos -= inputBufferSize;

        const float detectedPitchInHz = aubio.process(inputBuffer);
        const float pitchConfidence = pitchDetector.getPitchConfidence();

        if (detectedPitchInHz > 0.f && pitchConfidence >= threshold)
        {
            const float linearPitch = (12.f * log2f(detectedPitchInHz / 440.f) + 69.f) + (12.f * octave);
            lastKnownPitchLinear = cvPitch = std::max(0.f, std::min(10.f, linearPitch * (1.f/12.f)));
            lastKnownPitchInHz = detectedPitchInHz;
            cvSignal = 1.f;
        }
        else if (holdOutputPitch)
        {
            cvPitch = lastKnownPitchLinear;
            cvSignal = 0.f;
        }
        else
        {
            lastKnownPitchInHz = lastKnownPitchLinear = cvPitch = 0.0f;
            cvSignal = 0.f;
        }

        lastKnownPitchConfidence = pitchConfidence;
    }
    else
    {
        cvPitch = lastKnownPitchLinear;
        cvSignal = cvPitch > 0.0f ? 1.0f : 0.0f;
    }

    // TODO replace with faster SSE/NEON value assign
    for (uint32_t i = 0; i < numFrames; ++i) {
        outputs[outputPitch][i] = cvPitch;
        outputs[outputSignal][i] = cvSignal;
    }
}

void AudioToCVPitch::bufferSizeChanged(uint32_t newBufferSize)
{
    inputBufferSize = std::max(newBufferSize, kMinimumAubioBufferSize);
    aubio.setBuffersize(inputBufferSize);

    delete[] inputBuffer;
    inputBuffer = new float[inputBufferSize];
}

void AudioToCVPitch::sampleRateChanged(double newSampleRate)
{
    aubio.setSamplerate(newSampleRate);
}

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new AudioToCVPitch();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
