#ifndef DISTRHO_PLUGIN_SLPLUGIN_HPP_INCLUDED
#define DISTRHO_PLUGIN_SLPLUGIN_HPP_INCLUDED

#include "DistrhoPlugin.hpp"
#include "aubio_pitch.hpp"


START_NAMESPACE_DISTRHO

class AudioToCVPitch : public Plugin
{
public:
    enum Parameters
    {
        paramSensitivity = 0,
        paramOctave,
        paramCount
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
        return "Audio to CV pitch";
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

    void initParameter(uint32_t index, Parameter& parameter) override;

    // -------------------------------------------------------------------
    // Internal data

    float getParameterValue(uint32_t index) const override;
    void  setParameterValue(uint32_t index, float value) override;

    // -------------------------------------------------------------------
    // Process
    void activate() override;
    void deactivate() override;
    void midiNoteOn(uint8_t pitch, uint8_t velocity);
    void midiNoteOff(uint8_t pitch);
    void run(const float** inputs, float** outputs, uint32_t frames) override;

private:

    AubioModule *aubio;
    AubioPitch pitchDetector;

    float sensitivity;
    int   octave;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioToCVPitch)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif  // DISTRHO_PLUGIN_SLPLUGIN_HPP_INCLUDED
