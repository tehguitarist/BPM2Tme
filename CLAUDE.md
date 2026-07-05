# BPM2Time — Project Memory

BPM2Time is a lightweight AU/VST3 passthrough plugin (JUCE 8) that converts DAW tempo into
millisecond values for note divisions (1/128 to 1/1). Author/Company: Leigh Pierce.

## Quick reference

```
Build:   cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
AU:      cmake --build build --target BPM2Time_AU     (macOS only; auto-installs)
VST3:    cmake --build build --target BPM2Time_VST3
Release: gh workflow run release.yml
```

@.claude/rules/build.md

## Source layout

- `Source/PluginProcessor.{h,cpp}` — tempo sync, division math, `AudioProcessorValueTreeState`.
- `Source/PluginEditor.{h,cpp}` — UI (sync toggle, manual BPM slider, division buttons, ms readout).
