#include "plugin.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

// use a minimum value in order to keep cpu usage low
static constexpr const uint32_t kMinimumAubioBufferSize = 1024 + 256 + 128;
static constexpr const uint32_t kAubioHopSize = 1;

// static checks
static_assert(sizeof(smpl_t) == sizeof(float), "smpl_t is float");

// -----------------------------------------------------------------------

AudioToCVPitch::AudioToCVPitch()
    : Plugin(paramCount, 0, 0),
      pitchDetector(nullptr),
      detectedPitch(new_fvec(1))
{
    inputBufferSize = std::max(getBufferSize(), kMinimumAubioBufferSize) / kAubioHopSize;
    recreateAubioPitchDetector(getSampleRate());

    lastKnownPitchLinear = 0.0f;
    lastKnownPitchInHz = 0.0f;
    lastKnownPitchConfidence = 0.0f;

    sensitivity = 60.0f;
    threshold = 12.5f;
    octave = 0;
    holdOutputPitch = true;

    inputBuffer = new_fvec(inputBufferSize);
    inputBufferPos = 0;
}

AudioToCVPitch::~AudioToCVPitch()
{
    if (pitchDetector != nullptr)
        del_aubio_pitch(pitchDetector);

    del_fvec(inputBuffer);
    del_fvec(detectedPitch);
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
            parameter.ranges.def = 60.f;
            parameter.ranges.min = 0.1f;
            parameter.ranges.max = 100.f;
            break;
        case paramConfidenceThreshold:
            parameter.hints = kParameterIsAutomatable;
            parameter.name = "Confidence Threshold";
            parameter.symbol = "ConfidenceThreshold";
            parameter.unit = "%";
            parameter.ranges.def = 12.5f;
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 100.f;
            break;
        case paramTolerance:
            parameter.hints = kParameterIsAutomatable;
            parameter.name = "Tolerance";
            parameter.symbol = "Tolerance";
            parameter.unit = "%";
            parameter.ranges.def = 6.25f; // default is 0.15 for yin and 0.85 for yinfft
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 100.f;
            break;
        case paramOctave:
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger;
            parameter.name = "Octave";
            parameter.symbol = "Octave";
            parameter.ranges.def = -3;
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
        case paramTolerance:
            return aubio_pitch_get_tolerance(pitchDetector) * 100.f;
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
        case paramTolerance:
            aubio_pitch_set_tolerance(pitchDetector, value * 0.01f);
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
        std::memcpy(inputBuffer->data + inputBufferPos, inputs[0], sizeof(float)*numFrames);
    }
    else
    {
        // TODO replace with faster SSE/NEON multiply and assign
        for (uint32_t i = 0; i < numFrames; ++i)
            inputBuffer->data[inputBufferPos + i] = inputs[0][i] * sensitivity;
    }

    inputBufferPos += numFrames;

    float cvPitch, cvSignal;

    if (inputBufferPos >= inputBufferSize)
    {
        inputBufferPos -= inputBufferSize;

        aubio_pitch_do(pitchDetector, inputBuffer, detectedPitch);
        const float detectedPitchInHz = fvec_get_sample(detectedPitch, 0);
        const float pitchConfidence = aubio_pitch_get_confidence(pitchDetector);

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
    inputBufferSize = std::max(newBufferSize, kMinimumAubioBufferSize) / kAubioHopSize;

    del_fvec(inputBuffer);
    inputBuffer = new_fvec(inputBufferSize);
    inputBufferPos = 0;

    recreateAubioPitchDetector(getSampleRate());
}

void AudioToCVPitch::sampleRateChanged(double newSampleRate)
{
    recreateAubioPitchDetector(newSampleRate);
}

// -----------------------------------------------------------------------

void AudioToCVPitch::recreateAubioPitchDetector(double sampleRate)
{
    float tolerance;

    if (pitchDetector != nullptr)
    {
        tolerance = aubio_pitch_get_tolerance(pitchDetector);
        del_aubio_pitch(pitchDetector);
    }
    else
    {
        tolerance = 0.625f;
    }

    pitchDetector = new_aubio_pitch("yinfast", inputBufferSize, kAubioHopSize, sampleRate);
    DISTRHO_SAFE_ASSERT_RETURN(pitchDetector != nullptr,);

    aubio_pitch_set_silence(pitchDetector, -30.0f);
    aubio_pitch_set_tolerance(pitchDetector, tolerance);
    aubio_pitch_set_unit(pitchDetector, "Hz");
}

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new AudioToCVPitch();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
