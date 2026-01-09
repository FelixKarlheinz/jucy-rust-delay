#include "PluginEditor.h"

#include "PluginProcessor.h"

AudioPluginEditor::AudioPluginEditor(AudioPluginProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p) {
  setSize(500, 300);

  setupSlider(delaySlider, delayLabel, "DELAY");
  setupSlider(characterSlider, characterLabel, "CHARACTER");
  setupSlider(feedbackSlider, feedbackLabel, "FEEDBACK");
  setupSlider(dryWetSlider, dryWetLabel, "DRY/WET");

  delayAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.getValueTreeState(), "delay", delaySlider);

  characterAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.getValueTreeState(), "character", characterSlider);

  feedbackAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.getValueTreeState(), "feedback", feedbackSlider);

  dryWetAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processorRef.getValueTreeState(), "drywet", dryWetSlider);
}

AudioPluginEditor::~AudioPluginEditor() {
  delaySlider.setLookAndFeel(nullptr);
  characterSlider.setLookAndFeel(nullptr);
  feedbackSlider.setLookAndFeel(nullptr);
  dryWetSlider.setLookAndFeel(nullptr);
}

void AudioPluginEditor::setupSlider(juce::Slider& slider, juce::Label& label,
                                    const juce::String& labelText) {
  slider.setLookAndFeel(&consoleLookAndFeel);
  slider.setSliderStyle(juce::Slider::LinearVertical);
  slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

  slider.setColour(juce::Slider::trackColourId, juce::Colour(30, 30, 30));
  slider.setColour(juce::Slider::backgroundColourId, juce::Colour(20, 20, 20));
  slider.setColour(juce::Slider::thumbColourId, juce::Colour(180, 180, 185));
  slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::black);
  slider.setColour(juce::Slider::textBoxBackgroundColourId,
                   juce::Colour(200, 200, 205));
  slider.setColour(juce::Slider::textBoxOutlineColourId,
                   juce::Colour(100, 100, 105));

  addAndMakeVisible(slider);

  label.setText(labelText, juce::dontSendNotification);
  label.setJustificationType(juce::Justification::centred);
  label.setColour(juce::Label::textColourId, juce::Colours::black);
  label.setFont(juce::FontOptions(13.0f, juce::Font::bold));
  addAndMakeVisible(label);
}

void AudioPluginEditor::paint(juce::Graphics& graphics) {
  juce::ColourGradient backgroundGradient(
      juce::Colour(200, 200, 205), 0.0f, 0.0f, juce::Colour(170, 170, 175),
      0.0f, static_cast<float>(getHeight()), false);
  graphics.setGradientFill(backgroundGradient);
  graphics.fillAll();

  graphics.setColour(juce::Colour(140, 140, 145));
  graphics.fillRect(0, 0, getWidth(), 2);
  graphics.fillRect(0, getHeight() - 2, getWidth(), 2);

  graphics.setColour(juce::Colour(220, 220, 225));
  graphics.fillRect(0, 2, getWidth(), 1);
  graphics.fillRect(0, getHeight() - 3, getWidth(), 1);

  auto titleBounds = getLocalBounds().removeFromTop(80);

  graphics.setColour(juce::Colours::black);
  auto titleFont = juce::FontOptions(48.0f, juce::Font::bold);
  graphics.setFont(titleFont);

  auto titleText = "JUCY RUST DELAY";
  graphics.drawText(titleText, titleBounds.withTrimmedTop(20).withHeight(45),
                    juce::Justification::centred, false);
}

void AudioPluginEditor::resized() {
  auto bounds = getLocalBounds();
  bounds.removeFromTop(100);
  bounds.removeFromBottom(10);
  bounds.reduce(40, 30);

  juce::FlexBox mainFlexBox;
  mainFlexBox.flexDirection = juce::FlexBox::Direction::row;
  mainFlexBox.justifyContent = juce::FlexBox::JustifyContent::spaceAround;

  mainFlexBox.items.add(juce::FlexItem(delaySlider)
                            .withFlex(1)
                            .withMargin(juce::FlexItem::Margin(0, 10, 0, 10)));
  mainFlexBox.items.add(juce::FlexItem(characterSlider)
                            .withFlex(1)
                            .withMargin(juce::FlexItem::Margin(0, 10, 0, 10)));
  mainFlexBox.items.add(juce::FlexItem(feedbackSlider)
                            .withFlex(1)
                            .withMargin(juce::FlexItem::Margin(0, 10, 0, 10)));
  mainFlexBox.items.add(juce::FlexItem(dryWetSlider)
                            .withFlex(1)
                            .withMargin(juce::FlexItem::Margin(0, 10, 0, 10)));

  mainFlexBox.performLayout(bounds);

  delayLabel.setBounds(delaySlider.getX(), delaySlider.getY() - 10,
                       delaySlider.getWidth(), 10);
  characterLabel.setBounds(characterSlider.getX(), characterSlider.getY() - 10,
                           characterSlider.getWidth(), 10);
  feedbackLabel.setBounds(feedbackSlider.getX(), feedbackSlider.getY() - 10,
                          feedbackSlider.getWidth(), 10);
  dryWetLabel.setBounds(dryWetSlider.getX(), dryWetSlider.getY() - 10,
                        dryWetSlider.getWidth(), 10);
}
