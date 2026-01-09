#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"

class ConsoleFaderLookAndFeel : public juce::LookAndFeel_V4 {
 public:
  void drawLinearSlider(juce::Graphics& graphics, int x, int y, int width,
                        int height, float sliderPos, float, float,
                        juce::Slider::SliderStyle style,
                        juce::Slider&) override {
    if (style == juce::Slider::LinearVertical) {
      const int grooveWidth = 8;
      const int grooveX = x + (width - grooveWidth) / 2;

      graphics.setColour(juce::Colour(20, 20, 20));
      graphics.fillRect(grooveX, y, grooveWidth, height);

      graphics.setColour(juce::Colour(40, 40, 40));
      graphics.drawRect(grooveX, y, grooveWidth, height, 1);

      const float faderWidth = 35;
      const float faderHeight = 25;
      const float faderX =
          static_cast<float>(x) + (static_cast<float>(width) - faderWidth) / 2;
      const float faderY = sliderPos - faderHeight / 2;

      graphics.setColour(juce::Colour(180, 180, 185));
      graphics.fillRect(static_cast<int>(faderX), static_cast<int>(faderY),
                        static_cast<int>(faderWidth),
                        static_cast<int>(faderHeight));

      graphics.setColour(juce::Colours::black);
      graphics.drawRect(static_cast<int>(faderX), static_cast<int>(faderY),
                        static_cast<int>(faderWidth),
                        static_cast<int>(faderHeight), 2);

      graphics.setColour(juce::Colour(200, 200, 205));
      graphics.drawLine(static_cast<float>(faderX + 4),
                        static_cast<float>(faderY + faderHeight / 2),
                        static_cast<float>(faderX + faderWidth - 4),
                        static_cast<float>(faderY + faderHeight / 2), 1.5f);
    }
  }
};

class AudioPluginEditor : public juce::AudioProcessorEditor {
 public:
  explicit AudioPluginEditor(AudioPluginProcessor& processor);
  ~AudioPluginEditor() override;

  void paint(juce::Graphics& graphics) override;
  void resized() override;

 private:
  AudioPluginProcessor& processorRef;

  ConsoleFaderLookAndFeel consoleLookAndFeel;

  juce::Slider delaySlider;
  juce::Slider feedbackSlider;
  juce::Slider dryWetSlider;
  juce::Slider characterSlider;

  juce::Label delayLabel;
  juce::Label feedbackLabel;
  juce::Label dryWetLabel;
  juce::Label characterLabel;

  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      delayAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      feedbackAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      dryWetAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      characterAttachment;

  void setupSlider(juce::Slider& slider, juce::Label& label,
                   const juce::String& labelText);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginEditor)
};
