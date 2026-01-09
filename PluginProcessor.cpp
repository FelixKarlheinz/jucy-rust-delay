#include "PluginProcessor.h"

#include "PluginEditor.h"

//==============================================================================
AudioPluginProcessor::AudioPluginProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      valueTreeState(*this, nullptr, "Parameters", createParameterLayout()) {}

AudioPluginProcessor::~AudioPluginProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout
AudioPluginProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "delay", "Delay Time",
      juce::NormalisableRange<float>(0.0f, 3000.0f, 1.0f), 500.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "character", "Character",
      juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "feedback", "Feedback",
      juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "drywet", "Dry/Wet", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
      50.0f));

  return layout;
}

//==============================================================================
const juce::String AudioPluginProcessor::getName() const {
  return JucePlugin_Name;
}

bool AudioPluginProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool AudioPluginProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool AudioPluginProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double AudioPluginProcessor::getTailLengthSeconds() const { return 0.0; }

int AudioPluginProcessor::getNumPrograms() {
  return 1;  // NB: some hosts don't cope very well if you tell them there are 0
             // programs, so this should be at least 1, even if you're not
             // really implementing programs.
}

int AudioPluginProcessor::getCurrentProgram() { return 0; }

void AudioPluginProcessor::setCurrentProgram(int index) {
  juce::ignoreUnused(index);
}

const juce::String AudioPluginProcessor::getProgramName(int index) {
  juce::ignoreUnused(index);
  return {};
}

void AudioPluginProcessor::changeProgramName(int index,
                                             const juce::String& newName) {
  juce::ignoreUnused(index, newName);
}

//==============================================================================
void AudioPluginProcessor::prepareToPlay(double sampleRate,
                                         int samplesPerBlock) {
  currentSampleRate = sampleRate;

  const auto sampleRateInt = static_cast<size_t>(sampleRate);
  const auto delayTimeMs = valueTreeState.getRawParameterValue("delay")->load();
  const size_t delaySamples =
      static_cast<size_t>((delayTimeMs / 1000.0f) * sampleRate);

  const auto feedbackPercent =
      valueTreeState.getRawParameterValue("feedback")->load();
  const float feedbackAmount = feedbackPercent / 100.0f;

  leftChannelDelay = std::make_unique<rust::Box<Delay>>(
      create_delay(sampleRateInt, delaySamples, feedbackAmount));

  rightChannelDelay = std::make_unique<rust::Box<Delay>>(
      create_delay(sampleRateInt, delaySamples, feedbackAmount));

  dryBuffer.setSize(2, samplesPerBlock);
}

void AudioPluginProcessor::releaseResources() {
  if (leftChannelDelay) (*leftChannelDelay)->reset();

  if (rightChannelDelay) (*rightChannelDelay)->reset();

  dryBuffer.setSize(0, 0);
}

bool AudioPluginProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const {
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;

  return true;
}

void AudioPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);
  juce::ScopedNoDenormals noDenormals;

  const auto totalNumInputChannels = getTotalNumInputChannels();
  const auto totalNumOutputChannels = getTotalNumOutputChannels();

  for (auto channelIndex = totalNumInputChannels;
       channelIndex < totalNumOutputChannels; ++channelIndex)
    buffer.clear(channelIndex, 0, buffer.getNumSamples());

  if (!leftChannelDelay || !rightChannelDelay) return;

  const auto numSamples = buffer.getNumSamples();
  const auto numChannels =
      juce::jmin(totalNumInputChannels, totalNumOutputChannels);

  const auto delayTimeMs = valueTreeState.getRawParameterValue("delay")->load();
  const size_t delaySamples =
      static_cast<size_t>((delayTimeMs / 1000.0f) * currentSampleRate);

  const auto characterPercent =
      valueTreeState.getRawParameterValue("character")->load();
  const float characterAmount = characterPercent / 100.0f;

  const auto feedbackPercent =
      valueTreeState.getRawParameterValue("feedback")->load();
  const float feedbackAmount = feedbackPercent / 100.0f;

  const auto dryWetPercent =
      valueTreeState.getRawParameterValue("drywet")->load();
  const float wetGain = dryWetPercent / 100.0f;
  const float dryGain = 1.0f - wetGain;

  (*leftChannelDelay)->set_delay_samples(delaySamples);
  (*leftChannelDelay)->set_character(characterAmount);
  (*leftChannelDelay)->set_feedback(feedbackAmount);

  (*rightChannelDelay)->set_delay_samples(delaySamples);
  (*rightChannelDelay)->set_character(characterAmount);
  (*rightChannelDelay)->set_feedback(feedbackAmount);

  dryBuffer.setSize(numChannels, numSamples, false, false, true);

  for (int channel = 0; channel < numChannels; ++channel) {
    dryBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);
  }

  for (int channel = 0; channel < numChannels; ++channel) {
    auto* channelData = buffer.getWritePointer(channel);
    auto& delayProcessor =
        (channel == 0) ? *leftChannelDelay : *rightChannelDelay;

    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex) {
      const float inputSample = channelData[sampleIndex];
      const float wetSample = (*delayProcessor).process_sample(inputSample);
      const float drySample = dryBuffer.getSample(channel, sampleIndex);
      channelData[sampleIndex] = dryGain * drySample + wetGain * wetSample;
    }
  }
}

//==============================================================================
bool AudioPluginProcessor::hasEditor() const {
  return true;  // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginProcessor::createEditor() {
  return new AudioPluginEditor(*this);
}

//==============================================================================
void AudioPluginProcessor::getStateInformation(juce::MemoryBlock& destData) {
  auto parameterState = valueTreeState.copyState();
  std::unique_ptr<juce::XmlElement> xml(parameterState.createXml());
  copyXmlToBinary(*xml, destData);
}

void AudioPluginProcessor::setStateInformation(const void* data,
                                               int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));

  if (xmlState != nullptr &&
      xmlState->hasTagName(valueTreeState.state.getType()))
    valueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new AudioPluginProcessor();
}
