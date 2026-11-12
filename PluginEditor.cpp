#include "PluginEditor.h"
#include "BinaryData.h"
#include "MiniMetersOpener.h"
#include "PluginProcessor.h"
#include <JuceHeader.h>
#include <atomic>
#include <chrono>
#include <thread>

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , processorRef(p) {
    juce::ignoreUnused(processorRef);
    setLookAndFeel(&m_mm_look_and_feel);
    addAndMakeVisible(m_reset_button);
    m_reset_button.onClick = [&]() {
        processorRef.ipc_setup();
        repaint();
    };

    addAndMakeVisible(m_refresh_pairing_button);
    m_refresh_pairing_button.onClick = [&]() {
        processorRef.refreshPairingToken();
        repaint();
    };

    setSize(800, 350);
    updatePairingCode();
    startTimerHz(1);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {
    processorRef.editor_ptr = nullptr;
    stopTimer();
    setLookAndFeel(nullptr);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(24, 30, 40));
    g.setColour(juce::Colours::white);
    g.setFont(m_mm_look_and_feel.minimeters_font.withHeight(24));
    g.drawText("MiniMetersMobile", 24, 14, getWidth() - 150, 30, juce::Justification::left);

    g.setFont(m_mm_look_and_feel.minimeters_font.withHeight(13));
    g.setColour(juce::Colour(180, 195, 215));
    g.drawText(juce::String(ProjectInfo::versionString), 24, 43, 120, 18, juce::Justification::left);

    g.setColour(juce::Colours::white);
    g.drawImageWithin(pairingQrCode, 24, 72, 220, 220, juce::RectanglePlacement::centred);

    auto info = juce::String("Scan to connect\n\nAddress: ") + processorRef.getLanAddress()
        + ":8422\nPath: /ws\n\nToken:\n" + processorRef.getPairingToken();
    g.drawFittedText(info, juce::Rectangle<int> { 280, 72, getWidth() - 305, 190 },
                     juce::Justification::left, 8);

    g.setColour(juce::Colour(180, 195, 215));
    const auto status = processorRef.server_state == processorRef.StatePrimary
        ? "Audio source: primary"
        : "Audio source: another instance";
    g.drawText(status, 280, 280, getWidth() - 305, 24, juce::Justification::left);
    g.drawText("Network: " + juce::String(processorRef.isNetworkRunning() ? "listening" : "unavailable")
                   + "  Clients: " + juce::String(static_cast<int>(processorRef.getConnectedClientCount())),
               280,
               300,
               getWidth() - 305,
               24,
               juce::Justification::left);
}

void AudioPluginAudioProcessorEditor::resized() {
    m_reset_button.setBounds(juce::Rectangle<int>(getWidth() - 100, 16, 75, 28));
    m_refresh_pairing_button.setBounds(juce::Rectangle<int>(280, 315, 180, 30));
}

void AudioPluginAudioProcessorEditor::timerCallback() {
    processorRef.serviceNetwork();
    updatePairingCode();
    repaint();
}

void AudioPluginAudioProcessorEditor::updatePairingCode() {
    const auto newPayload = processorRef.getPairingPayload();
    if (newPayload != pairingPayload) {
        pairingPayload = newPayload;
        pairingQrCode = QrCodeImage::create(pairingPayload);
    }
}
