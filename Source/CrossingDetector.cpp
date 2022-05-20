/*
------------------------------------------------------------------

This file is part of a plugin for the Open Ephys GUI
Copyright (C) 2017 Translational NeuroEngineering Laboratory, MGH

------------------------------------------------------------------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "CrossingDetector.h"
#include "CrossingDetectorEditor.h"

#include <cmath> // for ceil, floor

/** ------------- Crossing Detector Stream Settings --------------- */

CrossingDetectorSettings::CrossingDetectorSettings() :
    inputChannel(0),
    eventChannel(0),
    sampleRate(0.0f),
    eventChannelPtr(nullptr),
    turnoffEvent(nullptr)
{
     // make the event-related metadata descriptors
    eventMetadataDescriptors.add(new MetadataDescriptor(MetadataDescriptor::INT64, 1, "Crossing Point",
        "Time when threshold was crossed", "crossing.point"));
    eventMetadataDescriptors.add(new MetadataDescriptor(MetadataDescriptor::FLOAT, 1, "Crossing level",
        "Voltage level at first sample after crossing", "crossing.level"));
    eventMetadataDescriptors.add(new MetadataDescriptor(MetadataDescriptor::FLOAT, 1, "Threshold",
        "Monitored voltage threshold", "crossing.threshold"));
    eventMetadataDescriptors.add(new MetadataDescriptor(MetadataDescriptor::UINT8, 1, "Direction",
        "Direction of crossing: 1 = rising, 0 = falling", "crossing.direction"));
}

void CrossingDetectorSettings::updateSampleRateDependentValues(
                                int eventDuration,
                                int timeout,
                                int bufferEndMask)
{
    eventDurationSamp = int(std::ceil(eventDuration * sampleRate / 1000.0f));
    timeoutSamp = int(std::floor(timeout * sampleRate / 1000.0f));
    bufferEndMaskSamp = int(std::ceil(bufferEndMask * sampleRate / 1000.0f));
}

TTLEventPtr CrossingDetectorSettings::createEvent(juce::int64 bufferTs, int crossingOffset,
    int bufferLength, float threshold, float crossingLevel, bool eventState)
{
    // Construct metadata array
    // The order has to match the order the descriptors are stored in createEventChannels.
    MetadataValueArray mdArray;

    int mdInd = 0;
    MetadataValue* crossingPointVal = new MetadataValue(*eventMetadataDescriptors[mdInd++]);
    crossingPointVal->setValue(bufferTs + crossingOffset);
    mdArray.add(crossingPointVal);

    MetadataValue* crossingLevelVal = new MetadataValue(*eventMetadataDescriptors[mdInd++]);
    crossingLevelVal->setValue(crossingLevel);
    mdArray.add(crossingLevelVal);

    MetadataValue* threshVal = new MetadataValue(*eventMetadataDescriptors[mdInd++]);
    threshVal->setValue(threshold);
    mdArray.add(threshVal);

    MetadataValue* directionVal = new MetadataValue(*eventMetadataDescriptors[mdInd++]);
    directionVal->setValue(static_cast<juce::uint8>(crossingLevel > threshold));
    mdArray.add(directionVal);

    // Create event
    if(eventState)
    {
        int sampleNumOn = std::max(crossingOffset, 0);
        juce::int64 eventTsOn = bufferTs + sampleNumOn;
        TTLEventPtr eventOn = TTLEvent::createTTLEvent(eventChannelPtr, eventTsOn,
            eventChannel, true, mdArray);
        // addEvent(eventOn, sampleNumOn);
        return  eventOn;
    }
    else
    {
        int sampleNumOff = std::max(crossingOffset, 0) + eventDurationSamp;
        juce::int64 eventTsOff = bufferTs + sampleNumOff;
        TTLEventPtr eventOff = TTLEvent::createTTLEvent(eventChannelPtr, eventTsOff,
            eventChannel, false, mdArray);

        return eventOff;
    }
}


/** ------------- Crossing Detector Processor --------------- */

CrossingDetector::CrossingDetector()
    : GenericProcessor      ("Crossing Detector")
    , thresholdType         (CONSTANT)
    , constantThresh        (0.0f)
    , thresholdChannel      (-1)
    , validSubProcFullID    (0)
    , posOn                 (true)
    , negOn                 (false)
    , eventDuration         (5)
    , timeout               (1000)
    , useBufferEndMask      (false)
    , bufferEndMaskMs       (3)
    , pastStrict            (1.0f)
    , pastSpan              (0)
    , futureStrict          (1.0f)
    , futureSpan            (0)
    , useJumpLimit          (false)
    , jumpLimit             (5.0f)
    , jumpLimitSleep        (0)
    , jumpLimitElapsed      (jumpLimitSleep)
    , sampToReenable        (pastSpan + futureSpan + 1)
    , pastSamplesAbove      (0)
    , futureSamplesAbove    (0)
    , inputHistory          (pastSpan + futureSpan + 2)
    , thresholdHistory      (pastSpan + futureSpan + 2)
{
    randomThreshRange[0] = -180.0f;
    randomThreshRange[1] = 180.0f;
    thresholdVal = constantThresh;

    addSelectedChannelsParameter(Parameter::STREAM_SCOPE, "Channel", "The input channel to analyze", 1);

    StringArray outputChans;
    for (int chan = 1; chan <= 16; chan++)
        outputChans.add(String(chan));

    addCategoricalParameter(Parameter::STREAM_SCOPE, "Out", "Event output channel", outputChans, 1);

    addBooleanParameter(Parameter::GLOBAL_SCOPE, "Rising", 
                        "Trigger events when past samples are below and future samples are above the threshold",
                        posOn);
    
    addBooleanParameter(Parameter::GLOBAL_SCOPE, "Falling", 
                        "Trigger events when past samples are above and future samples are below the threshold",
                        negOn);
    
    addIntParameter(Parameter::GLOBAL_SCOPE, "Timeout (ms)", "Minimum length of time between consecutive events",
                    timeout, 0, 100000);
    
    StringArray thresholdNames;
    for (int i = 0; i < NUM_THRESHOLDS; i++)
    {
        thresholdNames.add(String(i));
    }

    addIntParameter(Parameter::GLOBAL_SCOPE, "threshold_type", "Type of Threshold to use", thresholdType, 0, 2);

    addStringParameter(Parameter::GLOBAL_SCOPE, "threshold_value", "Threshold Value set on the basis of type",
                    String(constantThresh));

    addFloatParameter(Parameter::GLOBAL_SCOPE, "constant_threshold", "Constant threshold value",
                    constantThresh, 0.0f, 100000.0f, 1.0f);
    
    addFloatParameter(Parameter::GLOBAL_SCOPE, "min_random_threshold", "Minimum random threshold value",
                    randomThreshRange[0], -10000.0f, 10000.0f, 0.1f);

    addFloatParameter(Parameter::GLOBAL_SCOPE, "max_random_threshold", "Maximum random threshold value",
                    randomThreshRange[1], -10000.0f, 10000.0f, 0.1f);

    addIntParameter(Parameter::STREAM_SCOPE, "threshold_chan", "Threshold reference channel", 0, 0, getTotalContinuousChannels() -1);
}

CrossingDetector::~CrossingDetector() {}

AudioProcessorEditor* CrossingDetector::createEditor()
{
    editor = std::make_unique<CrossingDetectorEditor>(this);
    return editor.get();
}

void CrossingDetector::updateSettings()
{
    settings.update(getDataStreams());

    for(auto stream : getDataStreams())
    {
        settings[stream->getStreamId()]->sampleRate =  stream->getSampleRate();
        
        EventChannel* ttlChan;
        EventChannel::Settings ttlChanSettings{
            EventChannel::Type::TTL,
            "Crossing detector output",
            "Triggers whenever the input signal crosses a voltage threshold.",
            "crossing.event",
            getDataStream(stream->getStreamId())
        };

        // event-related metadata!
        for (auto desc : settings[stream->getStreamId()]->eventMetadataDescriptors)
        {
            ttlChan->addEventMetadata(desc);
        }

        settings[stream->getStreamId()]->eventChannelPtr = ttlChan;
        eventChannels.add(ttlChan);
    }
}

void CrossingDetector::process(AudioSampleBuffer& continuousBuffer)
{
    // loop through the streams
    for (auto stream : getDataStreams())
    {

        if ((*stream)["enable_stream"])
        {
            CrossingDetectorSettings* settingsModule = settings[stream->getStreamId()];

            if (settingsModule->inputChannel < 0
                || settingsModule->inputChannel >= continuousBuffer.getNumChannels()
                || !settingsModule->eventChannelPtr)
            {
                jassertfalse;
                return;
            }

            int nSamples = getNumSamplesInBlock(stream->getStreamId());
            int globalChanIndex = stream->getContinuousChannels()[settingsModule->inputChannel]->getGlobalIndex();
            const float* const rp = continuousBuffer.getReadPointer(globalChanIndex);
            juce::int64 startTs = getFirstSampleNumberForBlock(stream->getStreamId());

            // turn off event from previous buffer if necessary
            int turnoffOffset = settingsModule->turnoffEvent ? 
                                jmax(0, (int)(settingsModule->turnoffEvent->getSampleNumber() - startTs)) : -1;
            if (turnoffOffset >= 0 && turnoffOffset < nSamples)
            {
                addEvent(settingsModule->turnoffEvent, turnoffOffset);
                settingsModule->turnoffEvent = nullptr;
            }

            const ThresholdType currThreshType = thresholdType;

            // store threshold for each sample of current buffer
            if (currThresholds.size() < nSamples)
            {
                currThresholds.resize(nSamples);
            }
            float* const pThresh = currThresholds.getRawDataPointer();
            const float* const rpThreshChan = currThreshType == CHANNEL
                ? continuousBuffer.getReadPointer(thresholdChannel)
                : nullptr;

            // define lambdas to access history values more easily
            auto inputAt = [=](int index)
            {
                return index < 0 ? inputHistory[index] : rp[index];
            };

            auto thresholdAt = [&, this](int index)
            {
                return index < 0 ? thresholdHistory[index] : pThresh[index];
            };

            // loop over current buffer and add events for newly detected crossings
            for (int i = 0; i < nSamples; ++i)
            {
                // state to keep constant during each iteration
                bool currPosOn = posOn;
                bool currNegOn = negOn;

                // get and save threshold for this sample
                switch (currThreshType)
                {
                case CONSTANT:
                case RANDOM:
                    pThresh[i] = currRandomThresh;
                    break;

                case CHANNEL:
                    pThresh[i] = rpThreshChan[i];
                    break;
                }

                int indCross = i - futureSpan;

                // update pastSamplesA`bove and futureSamplesAbove
                if (pastSpan > 0)
                {
                    int indLeaving = indCross - 2 - pastSpan;
                    if (inputAt(indLeaving) > thresholdAt(indLeaving))
                    {
                        pastSamplesAbove--;
                    }

                    int indEntering = indCross - 2;
                    if (inputAt(indEntering) > thresholdAt(indEntering))
                    {
                        pastSamplesAbove++;
                    }
                }

                if (futureSpan > 0)
                {
                    int indLeaving = indCross;
                    if (inputAt(indLeaving) > thresholdAt(indLeaving))
                    {
                        futureSamplesAbove--;
                    }

                    int indEntering = indCross + futureSpan; // (== i)
                    if (inputAt(indEntering) > thresholdAt(indEntering))
                    {
                        futureSamplesAbove++;
                    }
                }

                if (indCross < sampToReenable ||
                    (useBufferEndMask && nSamples - indCross > settingsModule->bufferEndMaskSamp))
                {
                    // can't trigger an event now
                    continue;
                }

                float preVal = inputAt(indCross - 1);
                float preThresh = thresholdAt(indCross - 1);
                float postVal = inputAt(indCross);
                float postThresh = thresholdAt(indCross);


                // check whether to trigger an event
                if (currPosOn && shouldTrigger(true, preVal, postVal, preThresh, postThresh) ||
                    currNegOn && shouldTrigger(false, preVal, postVal, preThresh, postThresh))
                {
                    // create and add ON event
                    TTLEventPtr onEvent = settingsModule->
                        createEvent(startTs, indCross, nSamples, postThresh, postVal, true);
                    addEvent(onEvent, std::max(indCross, 0));

                    // create OFF event
                    int sampleNumOff = std::max(indCross, 0) + settingsModule->eventDurationSamp;
                    TTLEventPtr offEvent = settingsModule->
                        createEvent(startTs, indCross, nSamples, postThresh, postVal, true);
                    
                    // Add or schedule turning-off event
                    // We don't care whether there are other turning-offs scheduled to occur either in
                    // this buffer or later. The abilities to change event duration during acquisition and for
                    // events to be longer than the timeout period create a lot of possibilities and edge cases,
                    // but overwriting turnoffEvent unconditionally guarantees that this and all previously
                    // turned-on events will be turned off by this "turning-off" if they're not already off.
                    if (sampleNumOff <= nSamples)
                    {
                        // add off event now
                        addEvent(offEvent, sampleNumOff);
                    }
                    else
                    {
                        // save for later
                        settingsModule->turnoffEvent = offEvent;
                    }
                    
                    // update sampToReenable
                    sampToReenable = indCross + 1 + settingsModule->timeoutSamp;

                    // if using random thresholds, set a new threshold
                    if (currThreshType == RANDOM)
                    {
                        currRandomThresh = nextRandomThresh();
                        thresholdVal = currRandomThresh;
                    }
                }
            }

            // update inputHistory and thresholdHistory
            inputHistory.enqueueArray(rp, nSamples);
            thresholdHistory.enqueueArray(pThresh, nSamples);

            // shift sampToReenable so it is relative to the next buffer
            sampToReenable = jmax(0, sampToReenable - nSamples);
        }
    }
}

void CrossingDetector::parameterValueChanged(Parameter* param)
{

}

// all new values should be validated before this function is called!
void CrossingDetector::setParameter(int parameterIndex, float newValue)
{
    switch (parameterIndex)
    {
    case THRESH_TYPE:
        thresholdType = static_cast<ThresholdType>(static_cast<int>(newValue));

        switch (thresholdType)
        {
        case CONSTANT:
            thresholdVal = constantThresh;
            break;

        case RANDOM:
            // get new random threshold
            currRandomThresh = nextRandomThresh();
            thresholdVal = currRandomThresh;
            break;

        case CHANNEL:
            jassert(isCompatibleWithInput(thresholdChannel));
            thresholdVal = toChannelThreshString(thresholdChannel);
            break;
        }

        break;

    case CONST_THRESH:
        constantThresh = newValue;
        break;

    case MIN_RAND_THRESH:
        randomThreshRange[0] = newValue;
        currRandomThresh = nextRandomThresh();
        if (thresholdType == RANDOM)
        {
            thresholdVal = currRandomThresh;
        }
        break;

    case MAX_RAND_THRESH:
        randomThreshRange[1] = newValue;
        currRandomThresh = nextRandomThresh();
        if (thresholdType == RANDOM)
        {
            thresholdVal = currRandomThresh;
        }
        break;

    case THRESH_CHAN:
        jassert(isCompatibleWithInput(static_cast<int>(newValue)));
        thresholdChannel = static_cast<int>(newValue);
        if (thresholdType == CHANNEL)
        {
            thresholdVal = toChannelThreshString(thresholdChannel);
        }
        break;

    case INPUT_CHAN:
        inputChannel = static_cast<int>(newValue);
        validSubProcFullID = getSubProcFullID(inputChannel);
   
        // make sure available threshold channels take into account new input channel
        static_cast<CrossingDetectorEditor*>(getEditor())->updateChannelThreshBox();

        // update signal chain, since the event channel metadata has to get updated.
        CoreServices::updateSignalChain(getEditor());

        break;

    case EVENT_CHAN:
        eventChannel = static_cast<int>(newValue);
        break;

    case POS_ON:
        posOn = static_cast<bool>(newValue);
        break;

    case NEG_ON:
        negOn = static_cast<bool>(newValue);
        break;

    case EVENT_DUR:
        eventDuration = static_cast<int>(newValue);
        updateSampleRateDependentValues();
        break;

    case TIMEOUT:
        timeout = static_cast<int>(newValue);
        updateSampleRateDependentValues();
        break;

    case PAST_SPAN:
        pastSpan = static_cast<int>(newValue);
        sampToReenable = pastSpan + futureSpan + 1;

        inputHistory.reset();
        inputHistory.resize(pastSpan + futureSpan + 2);
        thresholdHistory.reset();
        thresholdHistory.resize(pastSpan + futureSpan + 2);

        // counters must reflect current contents of inputHistory and thresholdHistory
        pastSamplesAbove = 0;
        futureSamplesAbove = 0;
        break;

    case PAST_STRICT:
        pastStrict = newValue;
        break;

    case FUTURE_SPAN:
        futureSpan = static_cast<int>(newValue);
        sampToReenable = pastSpan + futureSpan + 1;

        inputHistory.reset();
        inputHistory.resize(pastSpan + futureSpan + 2);
        thresholdHistory.reset();
        thresholdHistory.resize(pastSpan + futureSpan + 2);

        // counters must reflect current contents of inputHistory and thresholdHistory
        pastSamplesAbove = 0;
        futureSamplesAbove = 0;
        break;

    case FUTURE_STRICT:
        futureStrict = newValue;
        break;

    case USE_JUMP_LIMIT:
        useJumpLimit = static_cast<bool>(newValue);
        break;

    case JUMP_LIMIT:
        jumpLimit = newValue;
        break;

    case JUMP_LIMIT_SLEEP:
		jumpLimitSleep = newValue * getDataChannel(0)->getSampleRate();
        break;

    case USE_BUF_END_MASK:
        useBufferEndMask = static_cast<bool>(newValue);
        break;

    case BUF_END_MASK:
        bufferEndMaskMs = static_cast<int>(newValue);
        updateSampleRateDependentValues();
        break;
    }
}

bool CrossingDetector::startAcquisition()
{
    jumpLimitElapsed = jumpLimitSleep;

    for(auto stream : getDataStreams())
    {
        settings[stream->getStreamId()]->
            updateSampleRateDependentValues(eventDuration, timeout, bufferEndMaskMs);
    }

    return isEnabled;
}

bool CrossingDetector::stopAcquisition()
{
    // set this to pastSpan so that we don't trigger on old data when we start again.
    sampToReenable = pastSpan + futureSpan + 1;    
    // cancel any pending turning-off per stream
    for(auto stream : getDataStreams())
    {
        settings[stream->getStreamId()]->turnoffEvent = nullptr;
    }
    
    return true;
}

// ----- private functions ------

void CrossingDetector::handleTTLEvent(TTLEventPtr event)
{
    // jassert(indicatorChan > -1);
    // const EventChannel* adaptChanInfo = getEventChannel(indicatorChan);
    // jassert(isValidIndicatorChan(adaptChanInfo));
    // if (event == adaptChanInfo && thresholdType == ADAPTIVE && !adaptThreshPaused)
    // {
    //     // get error of received event
    //     BinaryEventPtr binaryEvent = BinaryEvent::deserializeFromMessage(event, eventInfo);
    //     if (binaryEvent == nullptr)
    //     {
    //         return;
    //     }
    //     float eventValue = floatFromBinaryEvent(binaryEvent);
    //     float eventErr = errorFromTarget(eventValue);

    //     // update state
    //     currLRDivisor += decayRate;
    //     double currDecayingLR = currLearningRate - currMinLearningRate;
    //     currLearningRate = currDecayingLR / currLRDivisor + currMinLearningRate;

    //     // update threshold
    //     constantThresh -= currLearningRate * eventErr;
    //     if (useAdaptThreshRange)
    //     {
    //         constantThresh = toThresholdInRange(constantThresh);
    //     }
    //     thresholdVal = constantThresh;
    // }
}

float CrossingDetector::toEquivalentInRange(float x, const float* range)
{
    if (!range)
    {
        jassertfalse;
        return x;
    }
    float top = range[1], bottom = range[0];
    if (x <= top && x >= bottom)
    {
        return x;
    }
    float rangeSize = top - bottom;
    jassert(rangeSize >= 0);
    if (rangeSize == 0)
    {
        return bottom;
    }

    float rem = fmod(x - bottom, rangeSize);
    return rem > 0 ? bottom + rem : bottom + rem + rangeSize;
}

float CrossingDetector::floatFromBinaryEvent(BinaryEventPtr& eventPtr)
{
    jassert(eventPtr != nullptr);
    const void *ptr = eventPtr->getBinaryDataPointer();
    switch (eventPtr->getBinaryType())
    {
    case EventChannel::INT8_ARRAY:
        return static_cast<float>(*(static_cast<const juce::int8*>(ptr)));
    case EventChannel::UINT8_ARRAY:
        return static_cast<float>(*(static_cast<const juce::uint8*>(ptr)));
    case EventChannel::INT16_ARRAY:
        return static_cast<float>(*(static_cast<const juce::int16*>(ptr)));
    case EventChannel::UINT16_ARRAY:
        return static_cast<float>(*(static_cast<const juce::uint16*>(ptr)));
    case EventChannel::INT32_ARRAY:
        return static_cast<float>(*(static_cast<const juce::int32*>(ptr)));
    case EventChannel::UINT32_ARRAY:
        return static_cast<float>(*(static_cast<const juce::uint32*>(ptr)));
    case EventChannel::INT64_ARRAY:
        return static_cast<float>(*(static_cast<const juce::int64*>(ptr)));
    case EventChannel::UINT64_ARRAY:
        return static_cast<float>(*(static_cast<const juce::uint64*>(ptr)));
    case EventChannel::FLOAT_ARRAY:
        return *(static_cast<const float*>(ptr));
    case EventChannel::DOUBLE_ARRAY:
        return static_cast<float>(*(static_cast<const double*>(ptr)));
    default:
        jassertfalse;
        return 0;
    }
}

bool CrossingDetector::isValidIndicatorChan(const EventChannel* eventInfo)
{
    // EventChannel::EventChannelTypes type = eventInfo->getChannelType();
    // bool isBinary = type >= EventChannel::BINARY_BASE_VALUE && type < EventChannel::INVALID;
    // bool isNonempty = eventInfo->getLength() > 0;
    // return isBinary && isNonempty;
}

float CrossingDetector::nextRandomThresh()
{
    float range = randomThreshRange[1] - randomThreshRange[0];
    return randomThreshRange[0] + range * rng.nextFloat();
}

String CrossingDetector::toChannelThreshString(int chanNum)
{
    return "<chan " + String(chanNum + 1) + ">";
}

bool CrossingDetector::shouldTrigger(bool direction, float preVal, float postVal,
    float preThresh, float postThresh)
{
    jassert(pastSamplesAbove >= 0 && futureSamplesAbove >= 0);
    // check jumpLimit
    if (useJumpLimit && abs(postVal - preVal) >= jumpLimit)
    {
        jumpLimitElapsed = 0;
        return false;
    }

    if (jumpLimitElapsed <= jumpLimitSleep)
    {
        jumpLimitElapsed++;
        return false;
    }

    // number of samples required before and after crossing threshold
    int pastSamplesNeeded = pastSpan ? static_cast<int>(ceil(pastSpan * pastStrict)) : 0;
    int futureSamplesNeeded = futureSpan ? static_cast<int>(ceil(futureSpan * futureStrict)) : 0;
    
    // four conditions for the event
    bool preSat = direction != (preVal > preThresh);
    bool postSat = direction == (postVal > postThresh);
    bool pastSat = (direction ? pastSpan - pastSamplesAbove : pastSamplesAbove) >= pastSamplesNeeded;
    bool futureSat = (direction ? futureSamplesAbove : futureSpan - futureSamplesAbove) >= futureSamplesNeeded;
    
    return preSat && postSat && pastSat && futureSat;
}
