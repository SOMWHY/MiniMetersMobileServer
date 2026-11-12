#include "QrCodeImage.h"

#include "third_party/qrcodegen/qrcodegen.hpp"

juce::Image QrCodeImage::create(const juce::String& payload, int scale) {
    const auto text = payload.toStdString();
    const auto code = qrcodegen::QrCode::encodeText(text.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
    const auto quietZone = 4;
    const auto imageSize = (code.getSize() + quietZone * 2) * scale;
    juce::Image image(juce::Image::RGB, imageSize, imageSize, true);

    juce::Graphics graphics(image);
    graphics.fillAll(juce::Colours::white);
    graphics.setColour(juce::Colours::black);
    for (int y = 0; y < code.getSize(); ++y) {
        for (int x = 0; x < code.getSize(); ++x) {
            if (code.getModule(x, y))
                graphics.fillRect((x + quietZone) * scale, (y + quietZone) * scale, scale, scale);
        }
    }
    return image;
}
