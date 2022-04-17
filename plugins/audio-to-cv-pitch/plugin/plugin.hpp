#ifndef DISTRHO_PLUGIN_SLPLUGIN_HPP_INCLUDED
#define DISTRHO_PLUGIN_SLPLUGIN_HPP_INCLUDED

#include "DistrhoPlugin.hpp"

extern "C" {
#include <aubio.h>
}

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class AudioToCVPitch : public Plugin
{
public:
    enum Parameters
    {
        paramSensitivity = 0,
        paramConfidenceThreshold,
        paramTolerance,
        paramOctave,
        paramHoldOutputPitch,
        paramDetectedPitch,
        paramPitchConfidence,
        paramCount
    };

    enum Outputs
    {
        outputPitch,
        outputSignal
    };

    AudioToCVPitch();
    ~AudioToCVPitch();

protected:
    // -------------------------------------------------------------------
    // Information

    const char* getLabel() const noexcept override
    {
        return "AudioToCVPitch";
    }

    const char* getDescription() const override
    {
        return "This plugin converts a monophonic audio signal to CV pitch";
    }

    const char* getMaker() const noexcept override
    {
        return "BGSN";
    }

    const char* getHomePage() const override
    {
        return "http://bramgiesen.com";
    }

    const char* getLicense() const noexcept override
    {
        return "GPLv3.0";
    }

    uint32_t getVersion() const noexcept override
    {
        return d_version(1, 0, 8);
    }

    int64_t getUniqueId() const noexcept override
    {
        return d_cconst('C', 'S', 'D', 's');
    }

    // -------------------------------------------------------------------
    // Init

    void initAudioPort(bool input, uint32_t index, AudioPort& port) override;
    void initParameter(uint32_t index, Parameter& parameter) override;

    // -------------------------------------------------------------------
    // Internal data

    float getParameterValue(uint32_t index) const override;
    void  setParameterValue(uint32_t index, float value) override;

    // -------------------------------------------------------------------
    // Process

    void activate() override;
    void deactivate() override;
    void run(const float** inputs, float** outputs, uint32_t frames) override;
    void bufferSizeChanged(uint32_t newBufferSize) override;
    void sampleRateChanged(double newSampleRate) override;

private:
    aubio_pitch_t* pitchDetector;
    fvec_t* const detectedPitch;

    fvec_t* inputBuffer;
    uint32_t inputBufferPos;
    uint32_t inputBufferSize;

    float lastKnownPitchLinear;
    float lastKnownPitchInHz;
    float lastKnownPitchConfidence;

    float sensitivity;
    float threshold;
    int   octave;
    bool  holdOutputPitch;

    void recreateAubioPitchDetector(double sampleRate);

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioToCVPitch)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif  // DISTRHO_PLUGIN_SLPLUGIN_HPP_INCLUDED
