#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "rust_delay.h"

class AudioPluginProcessor : public juce::AudioProcessor {
 public:
  AudioPluginProcessor();
  ~AudioPluginProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

  void processBlock(juce::AudioBuffer<float>& buffer,
                    juce::MidiBuffer& midiMessages) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String& newName) override;

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  juce::AudioProcessorValueTreeState& getValueTreeState() {
    return valueTreeState;
  }

 private:
  juce::AudioProcessorValueTreeState valueTreeState;
  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  std::unique_ptr<rust::Box<Delay>> leftChannelDelay;
  std::unique_ptr<rust::Box<Delay>> rightChannelDelay;

  juce::AudioBuffer<float> dryBuffer;
  double currentSampleRate = 44100.0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginProcessor)
};
