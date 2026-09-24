#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>

class TraversalState;

class TraversalParameters : private juce::Timer
{
public:

    enum class Setting { Multiplier, Channel, Transpose, Velocity, Total };

    static constexpr int   slotCount        = 16;
    static constexpr int   settingCount     = static_cast<int>(Setting::Total);
    static constexpr int   pollRateHz       = 30;
    static constexpr int   parameterVersion = 1;
    static constexpr float changeTolerance  = 1.0e-5f;

    static constexpr std::array<const char*, settingCount> settingIds   { "multiplier", "channel", "transpose", "velocity" };
    static constexpr std::array<const char*, settingCount> settingNames { "Multiplier", "Channel", "Transpose", "Velocity" };

    TraversalParameters(juce::AudioProcessorValueTreeState& parameterState, TraversalState& traversals);

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:

    void timerCallback() override;

    TraversalState& traversals;

    std::array<juce::Identifier, settingCount>                        properties;
    std::array<juce::RangedAudioParameter*, slotCount * settingCount> parameters {};
    std::array<float, slotCount * settingCount>                       lastSeenValues {};
};
