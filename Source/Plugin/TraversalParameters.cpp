#include "TraversalParameters.h"
#include "../Graph/TraversalState.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include "../Util/NodeInfo.h"

#include <cmath>

TraversalParameters::TraversalParameters(juce::AudioProcessorValueTreeState& parameterState, TraversalState& traversals)
    : traversals(traversals),
      properties { ValueTreeIdentifiers::TempoMultiplier,
                   ValueTreeIdentifiers::TraversalChannel,
                   ValueTreeIdentifiers::TraversalTranspose,
                   ValueTreeIdentifiers::TraversalVelocity }
{
    for (int slot = 0; slot < slotCount; ++slot) {
        for (int setting = 0; setting < settingCount; ++setting) {
            const juce::String parameterId = "traversal" + juce::String(slot + 1) + "_" + settingIds[static_cast<std::size_t>(setting)];
            const std::size_t  index       = static_cast<std::size_t>(slot * settingCount + setting);

            parameters[index]     = parameterState.getParameter(parameterId);
            lastSeenValues[index] = parameters[index]->getValue();
        }
    }

    startTimerHz(pollRateHz);
}

juce::AudioProcessorValueTreeState::ParameterLayout TraversalParameters::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    juce::NormalisableRange<float> multiplierRange { static_cast<float>(minimumTraversalMultiplier),
                                                     static_cast<float>(RTtraversal::maximumTempoMultiplier) };
    multiplierRange.setSkewForCentre(static_cast<float>(TraversalState::defaultTempoMult));

    for (int slot = 0; slot < slotCount; ++slot) {
        const juce::String traversalNumber = juce::String(slot + 1);
        const juce::String idPrefix        = "traversal" + traversalNumber + "_";
        const juce::String namePrefix      = "Traversal " + traversalNumber + " ";

        auto group = std::make_unique<juce::AudioProcessorParameterGroup>("traversal" + traversalNumber, "Traversal " + traversalNumber, "|");

        group->addChild(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { idPrefix + settingIds[static_cast<std::size_t>(Setting::Multiplier)], parameterVersion },
            namePrefix + settingNames[static_cast<std::size_t>(Setting::Multiplier)],
            multiplierRange,
            static_cast<float>(TraversalState::defaultTempoMult)));

        group->addChild(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID { idPrefix + settingIds[static_cast<std::size_t>(Setting::Channel)], parameterVersion },
            namePrefix + settingNames[static_cast<std::size_t>(Setting::Channel)],
            minimumMidiChannel,
            maximumMidiChannel,
            TraversalState::defaultChannel));

        group->addChild(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID { idPrefix + settingIds[static_cast<std::size_t>(Setting::Transpose)], parameterVersion },
            namePrefix + settingNames[static_cast<std::size_t>(Setting::Transpose)],
            minimumTraversalTranspose,
            maximumTraversalTranspose,
            TraversalState::defaultTranspose));

        group->addChild(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { idPrefix + settingIds[static_cast<std::size_t>(Setting::Velocity)], parameterVersion },
            namePrefix + settingNames[static_cast<std::size_t>(Setting::Velocity)],
            juce::NormalisableRange<float> { 0.0f, 1.0f },
            static_cast<float>(TraversalState::defaultVelocity)));

        layout.add(std::move(group));
    }

    return layout;
}

void TraversalParameters::timerCallback()
{
    for (int slot = 0; slot < slotCount; ++slot) {
        juce::ValueTree traversalData = traversals.map.getChildWithProperty(ValueTreeIdentifiers::TraversalId, slot + 1);

        for (int setting = 0; setting < settingCount; ++setting) {
            const std::size_t           index     = static_cast<std::size_t>(slot * settingCount + setting);
            const juce::Identifier&     property  = properties[static_cast<std::size_t>(setting)];
            juce::RangedAudioParameter& parameter = *parameters[index];

            if (traversalData.isValid()) {
                float treeValue = parameter.getDefaultValue();

                if (traversalData.hasProperty(property)) {
                    treeValue = parameter.convertTo0to1(static_cast<float>(static_cast<double>(traversalData.getProperty(property))));
                }

                if (std::abs(treeValue - lastSeenValues[index]) > changeTolerance) {
                    lastSeenValues[index] = treeValue;

                    parameter.beginChangeGesture();
                    parameter.setValueNotifyingHost(treeValue);
                    parameter.endChangeGesture();

                    continue;
                }
            }

            const float hostValue = parameter.getValue();

            if (std::abs(hostValue - lastSeenValues[index]) <= changeTolerance) {
                continue;
            }

            lastSeenValues[index] = hostValue;

            if (!traversalData.isValid()) {
                continue;
            }

            if (auto* integerParameter = dynamic_cast<juce::AudioParameterInt*>(&parameter)) {
                traversalData.setProperty(property, integerParameter->get(), nullptr);
            }
            else {
                traversalData.setProperty(property, static_cast<double>(parameter.convertFrom0to1(hostValue)), nullptr);
            }
        }
    }
}
