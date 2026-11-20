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

    addAndMakeVisible(m_copy_pairing_button);
    m_copy_pairing_button.onClick = [&]() {
        juce::SystemClipboard::copyTextToClipboard(processorRef.getPairingPayload());
    };

    addAndMakeVisible(m_forget_devices_button);
    m_forget_devices_button.onClick = [&]() {
        // Revokes every phone that paired with an earlier QR code. If the data
        // folder is read-only the phones are only forgotten for this session,
        // so say so instead of claiming they are gone.
        if (!processorRef.forgetPairedDevices()) {
            juce::NativeMessageBox::showMessageBoxAsync(
                juce::MessageBoxIconType::WarningIcon,
                "Could not forget paired phones",
                "The settings folder is read-only, so the stored device tokens could "
                "not be removed. Paired phones stay forgotten only until the plugin "
                "restarts; fix the folder permissions and try again.");
        }
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
    g.setFont(m_mm_look_and_feel.minimeters_font.withHeight(26));
    g.drawText("MiniMetersMobile", 24, 12, getWidth() - 150, 32, juce::Justification::left);

    g.setFont(m_mm_look_and_feel.minimeters_font.withHeight(15));
    g.setColour(juce::Colour(180, 195, 215));
    g.drawText(juce::String(ProjectInfo::versionString), 24, 44, 120, 20, juce::Justification::left);

    g.setColour(juce::Colours::white);
    g.drawImageWithin(pairingQrCode, 24, 72, 220, 220, juce::RectanglePlacement::centred);

    g.setFont(m_mm_look_and_feel.minimeters_font.withHeight(16));
    auto info = juce::String("Scan to connect\n\nAddress: ") + processorRef.getLanAddress()
        + ":8422\nPath: /ws\n\nToken:\n" + processorRef.getPairingToken()
        + "\n\nExpires in: " + juce::String(processorRef.getPairingSecondsRemaining()) + "s";
    g.drawFittedText(info, juce::Rectangle<int> { 280, 66, getWidth() - 305, 180 },
                     juce::Justification::left, 10);

    // The status lines used to sit under the button row and were clipped by it.
    // They now live on the right-hand side of the panel, above the buttons.
    g.setColour(juce::Colour(180, 195, 215));
    g.setFont(m_mm_look_and_feel.minimeters_font.withHeight(15));
    const auto status = processorRef.server_state == processorRef.StatePrimary
        ? "Audio source: primary"
        : "Audio source: another instance";
    const auto status_area = juce::Rectangle<int> { 380, 252, getWidth() - 404, 20 };
    g.drawText(status, status_area, juce::Justification::right, true);
    g.drawText("Network: " + juce::String(processorRef.isNetworkRunning() ? "listening" : "unavailable")
                   + "  Clients: " + juce::String(static_cast<int>(processorRef.getConnectedClientCount()))
                   + "  Paired: " + juce::String(static_cast<int>(processorRef.getPairedDeviceCount())),
               status_area.withY(274),
               juce::Justification::right,
               true);
}

void AudioPluginAudioProcessorEditor::resized() {
    m_reset_button.setBounds(juce::Rectangle<int>(getWidth() - 100, 16, 75, 28));
    m_refresh_pairing_button.setBounds(juce::Rectangle<int>(280, 315, 180, 30));
    m_copy_pairing_button.setBounds(juce::Rectangle<int>(470, 315, 160, 30));
    m_forget_devices_button.setBounds(juce::Rectangle<int>(640, 315, 140, 30));
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
