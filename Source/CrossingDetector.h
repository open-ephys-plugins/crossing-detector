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

#ifndef CROSSING_DETECTOR_H_INCLUDED
#define CROSSING_DETECTOR_H_INCLUDED

#include <ProcessorHeaders.h>
#include "CircularArray.h"

/*
 * The crossing detector plugin is designed to read in one continuous channel c, and generate events on one events channel
 * when c crosses a certain value. There are various parameters to tweak this basic functionality, including:
 *  - whether to listen for crosses with a positive or negative slope, or either
 *  - how strictly to filter transient level changes, by adjusting the required number and percent of past and future samples to be above/below the threshold
 *  - the duration of the generated event
 *  - the minimum time to wait between events ("timeout")
 *  - whether to use a constant threshold, draw one randomly from a range for each event, or read thresholds from an input channel
 *
 * All ontinuous signals pass through unchanged, so multiple CrossingDetectors can be
 * chained together in order to operate on more than one channel.
 *
 * @see GenericProcessor
 */

/** Holds settings for one stream's crossing detector */
class CrossingDetectorSettings
{
public:
    /** Constructor -- sets default values*/
    CrossingDetectorSettings();

    /** Destructor*/
    ~CrossingDetectorSettings() { }

    /** Converts parameters specified in ms to samples, and updates the corresponding member variables. */
    void updateSampleRateDependentValues(int eventDuration, int timeout, int bufferEndMask);

    /* Crate a "turning-on" or "turning-off" event for a crossing.
     *  - bufferTs:       Timestamp of start of current buffer
     *  - crossingOffset: Difference betweeen time of actual crossing and bufferTs
     *  - bufferLength:   Number of samples in current buffer
     *  - threshold:      Threshold at the time of the crossing
     *  - crossingLevel:  Level of signal at the first sample after the crossing
     */
    TTLEventPtr createEvent(juce::int64 bufferTs, int crossingOffset, int bufferLength,
        float threshold, float crossingLevel, bool eventState);

    /** Parameters */

    int inputChannel; //Index of the inut channel
    int eventChannel;
    
    float sampleRate; // Pointer to the actual input channel
    int eventDurationSamp;
    int timeoutSamp;
    int bufferEndMaskSamp;

    EventChannel* eventChannelPtr;
    MetadataDescriptorArray eventMetadataDescriptors;
    TTLEventPtr turnoffEvent; // holds a turnoff event that must be added in a later buffer
};


enum ThresholdType { CONSTANT = 0, RANDOM, CHANNEL, NUM_THRESHOLDS };

class CrossingDetector : public GenericProcessor
{
    friend class CrossingDetectorCanvas;

public:
    CrossingDetector();
    ~CrossingDetector();

    bool hasEditor() const { return true; }
    AudioProcessorEditor* createEditor() override;

    void updateSettings() override;

    void process(AudioSampleBuffer& continuousBuffer) override;

    void setParameter(int parameterIndex, float newValue) override;
    /** Called when a parameter is updated*/
    void parameterValueChanged(Parameter* param) override;

    bool startAcquisition() override;
    bool stopAcquisition() override;

private:

    enum CrossParameter
    {
        THRESH_TYPE,
        CONST_THRESH,
        MIN_RAND_THRESH,
        MAX_RAND_THRESH,
        THRESH_CHAN,
        INPUT_CHAN,
        EVENT_CHAN,
        POS_ON,
        NEG_ON,
        EVENT_DUR,
        TIMEOUT,
        PAST_SPAN,
        PAST_STRICT,
        FUTURE_SPAN,
        FUTURE_STRICT,
        USE_JUMP_LIMIT,
        JUMP_LIMIT,
        JUMP_LIMIT_SLEEP,
        USE_BUF_END_MASK,
        BUF_END_MASK
    };

    // ---------------------------- PRIVATE FUNCTIONS ----------------------

    /*********** adaptive threshold *************/

    /* Use events created by the phase calculator to adapt threshold, if the threshold
     * mode is adaptive.
     */
    void handleTTLEvent(TTLEventPtr event) override;

    /* Calculates the equivalent value of the given float within the given circular range (2-element array).
     * (e.g. if range[0] == 0, returns the positive float equivalent of x % range[1])
     */
    static float toEquivalentInRange(float x, const float* range);

    /* Convert the first element of a binary event to a float, regardless of the type
     * (assumes eventPtr is not null)
     */
    static float floatFromBinaryEvent(BinaryEventPtr& eventPtr);

    // Returns whether the given event chan can be used to train an adaptive threshold.
    static bool isValidIndicatorChan(const EventChannel* eventInfo);

    /********** random threshold ***********/

    // Select a new random threshold using minThresh, maxThresh, and rng.
    float nextRandomThresh();
 
    /********** channel threshold ***********/

    /* Returns true if the given chanNum corresponds to an input
     * and that channel has the same source subprocessor as the
     * selected inputChannel, but is not equal to the inputChannel.
     */
    bool isCompatibleWithInput(int chanNum) const;

    // Returns a string to display in the threshold box when using a threshold channel
    static String toChannelThreshString(int chanNum);

    /*********  triggering ************/

    /* Whether there should be a trigger in the given direction (true = rising, float = falling),
     * given the current pastCounter and futureCounter and the passed values and thresholds
     * surrounding the point where a crossing may be.
     */
    bool shouldTrigger(bool direction, float preVal, float postVal, float preThresh, float postThresh);


    // ------ PARAMETERS ------------

    StreamSettings<CrossingDetectorSettings> settings;
    ThresholdType thresholdType;

    // if using constant threshold:
    float constantThresh;

    // if using random thresholds:
    float randomThreshRange[2];
    float currRandomThresh;

    // if using channel threshold:
    int thresholdChannel;

    bool posOn;
    bool negOn;

    int eventDuration; // in milliseconds
    int timeout; // milliseconds after an event onset when no more events are allowed.

    bool useBufferEndMask;
    int bufferEndMaskMs;

    /* Number of *additional* past and future samples to look at at each timepoint (attention span)
     * If futureSpan samples are not available to look ahead from a timepoint, the test is delayed until enough samples are available.
     * If it succeeds, the event occurs on the first sample in the buffer when the test occurs, but the "crossing point"
     * metadata field will contain the timepoint of the actual crossing.
     */
    int pastSpan;
    int futureSpan;

    // fraction of spans required to be above / below threshold
    float pastStrict;
    float futureStrict;

    // maximum absolute difference between x[k] and x[k-1] to trigger an event on x[k]
    bool useJumpLimit;
    float jumpLimit;
    float jumpLimitSleep;
    int jumpLimitElapsed;

    // ------ INTERNALS -----------

    // the next time at which the detector should be reenabled after a timeout period, measured in
    // samples past the start of the current processing buffer. Less than -numNext if there is no scheduled reenable (i.e. the detector is enabled).
    int sampToReenable;

     // counters for delay keeping track of voting samples
    int pastSamplesAbove;
    int futureSamplesAbove;

    // arrays to implement past/future voting
    CircularArray<float> inputHistory;
    CircularArray<float> thresholdHistory;

    Array<float> currThresholds;

    Value thresholdVal; // underlying value of the threshold label
    
    Random rng; // for random thresholds

    // full subprocessor ID of input channel (or 0 if none selected)
    juce::uint32 validSubProcFullID;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrossingDetector);
};

#endif // CROSSING_DETECTOR_H_INCLUDED
