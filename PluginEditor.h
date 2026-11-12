#pragma once
#include "LookAndFeel.h"
#include "PluginProcessor.h"
#include "QrCodeImage.h"
#include <thread>

class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer {
    MiniMetersLookAndFeel m_mm_look_and_feel;

public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::TextButton m_reset_button { "Reset" };
    juce::TextButton m_refresh_pairing_button { "Refresh pairing" };

    AudioPluginAudioProcessor& processorRef;
    juce::Image pairingQrCode;
    juce::String pairingPayload;

    void timerCallback() override;
    void updatePairingCode();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
