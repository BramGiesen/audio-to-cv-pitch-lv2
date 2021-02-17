# audio-to-cv-pitch-lv2

LV2 plugin that converts audio to CV pitch (1 volt per octave).
The pitch tracking only works with monophonic signals.

The pitch tracking is provided by the [aubio_module](https://github.com/GeertRoks/aubio_module) by Geert Roks, which is a wrapper for the aubio library.

The plugin is still work in progress but the basic functionality is already functional.

# Building

```
git submodule update --init --recursive
make
```
