/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 7 End-User License
   Agreement and JUCE Privacy Policy.

   End User License Agreement: www.juce.com/juce-7-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

AudioParameterChoice::AudioParameterChoice (const ParameterID& idToUse,
                                            const String& nameToUse,
                                            const StringArray& c,
                                            int def,
                                            const AudioParameterChoiceAttributes& attributes)
   : RangedAudioParameter (idToUse, nameToUse, attributes.getAudioProcessorParameterWithIDAttributes()),
     choices (c),
     range ([this]
            {
                NormalisableRange<float> rangeWithInterval { 0.0f, (float) choices.size() - 1.0f,
                                                             [] (float, float end, float v) { return jlimit (0.0f, end, v * end); },
                                                             [] (float, float end, float v) { return jlimit (0.0f, 1.0f, v / end); },
                                                             [] (float start, float end, float v) { return (float) roundToInt (juce::jlimit (start, end, v)); } };
                rangeWithInterval.interval = 1.0f;
                return rangeWithInterval;
            }()),
     value ((float) def),
     defaultValue (convertTo0to1 ((float) def)),
     stringFromIndexFunction (attributes.getStringFromValueFunction() != nullptr
                                  ? attributes.getStringFromValueFunction()
                                  : [this] (int index, int) { return choices [index]; }),
     indexFromStringFunction (attributes.getValueFromStringFunction() != nullptr
                                  ? attributes.getValueFromStringFunction()
                                  : [this] (const String& text) { return choices.indexOf (text); })
{
    jassert (choices.size() > 1); // you must supply an actual set of items to choose from!
}

AudioParameterChoice::~AudioParameterChoice()
{
    #if __cpp_lib_atomic_is_always_lock_free
     static_assert (std::atomic<float>::is_always_lock_free,
                    "AudioParameterChoice requires a lock-free std::atomic<float>");
    #endif
}

float AudioParameterChoice::getValue() const                             { return convertTo0to1 (value); }
void AudioParameterChoice::setValue (float newValue)                     { value = convertFrom0to1 (newValue); valueChanged (getIndex()); }
float AudioParameterChoice::getDefaultValue() const                      { return defaultValue; }
int AudioParameterChoice::getNumSteps() const                            { return choices.size(); }
bool AudioParameterChoice::isDiscrete() const                            { return true; }
float AudioParameterChoice::getValueForText (const String& text) const   { return convertTo0to1 ((float) indexFromStringFunction (text)); }
String AudioParameterChoice::getText (float v, int length) const         { return stringFromIndexFunction ((int) convertFrom0to1 (v), length); }
void AudioParameterChoice::valueChanged (int)                            {}

AudioParameterChoice& AudioParameterChoice::operator= (int newValue)
{
    if (getIndex() != newValue)
        setValueNotifyingHost (convertTo0to1 ((float) newValue));

    return *this;
}


//==============================================================================
//==============================================================================
#if JUCE_UNIT_TESTS

struct AudioParameterChoiceTests final : public UnitTest
{
    AudioParameterChoiceTests()
        : UnitTest ("AudioParameterChoice", UnitTestCategories::audioProcessorParameters)
    {}

    void runTest() override
    {
        beginTest ("Three options switches at the correct points");
        {
            AudioParameterChoice choice ({}, {}, { "a", "b", "c" }, {});

            choice.setValueNotifyingHost (0.0f);
            expectEquals (choice.getIndex(), 0);

            choice.setValueNotifyingHost (0.2f);
            expectEquals (choice.getIndex(), 0);

            choice.setValueNotifyingHost (0.3f);
            expectEquals (choice.getIndex(), 1);

            choice.setValueNotifyingHost (0.7f);
            expectEquals (choice.getIndex(), 1);

            choice.setValueNotifyingHost (0.8f);
            expectEquals (choice.getIndex(), 2);

            choice.setValueNotifyingHost (1.0f);
            expectEquals (choice.getIndex(), 2);
        }

        beginTest ("Out-of-bounds input");
        {
            AudioParameterChoice choiceParam ({}, {}, { "a", "b", "c" }, {});

            choiceParam.setValueNotifyingHost (-0.5f);
            expectEquals (choiceParam.getIndex(), 0);

            choiceParam.setValueNotifyingHost (1.5f);
            expectEquals (choiceParam.getIndex(), 2);
        }
    }

};

static AudioParameterChoiceTests audioParameterChoiceTests;

#endif

} // namespace juce
