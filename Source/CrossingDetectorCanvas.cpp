/*
------------------------------------------------------------------

This file is part of a plugin for the Open Ephys GUI
Copyright (C) 2022 Open Ephys

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

#include "CrossingDetectorCanvas.h"
#include <cfloat> // FLT_MAX
#include <climits> // INT_MAX
#include <string> // stoi, stof

/*************** canvas (extra settings) *******************/

CrossingDetectorCanvas::CrossingDetectorCanvas (GenericProcessor* p)
    : Visualizer (p)
{
    processor = static_cast<CrossingDetector*> (p);
    editor = static_cast<CrossingDetectorEditor*> (processor->getEditor());
    initializeOptionsPanel();
    viewport = new Viewport();
    viewport->setViewedComponent (optionsPanel, false);
    viewport->setScrollBarsShown (true, true);
    viewport->setScrollBarThickness (12);
    addAndMakeVisible (viewport);
}

CrossingDetectorCanvas::~CrossingDetectorCanvas() {}

void CrossingDetectorCanvas::refreshState() {}

void CrossingDetectorCanvas::refresh() {}

void CrossingDetectorCanvas::paint (Graphics& g)
{
    g.fillAll (findColour (ThemeColours::componentBackground).darker());
}

void CrossingDetectorCanvas::resized()
{
    viewport->setBounds (0, 0, getWidth(), getHeight());
}

void CrossingDetectorCanvas::initializeOptionsPanel()
{
    optionsPanel = new Component ("CD Options Panel");
    // initial bounds, to be expanded
    juce::Rectangle<int> opBounds (0, 0, 1, 1);
    const int C_TEXT_HT = 25;
    const int LEFT_EDGE = 30;
    const int TAB_WIDTH = 25;

    juce::Rectangle<int> bounds;
    int xPos = LEFT_EDGE;
    int yPos = 15;

    optionsPanelTitle = new Label ("CDOptionsTitle", "Crossing Detector Additional Settings");
    optionsPanelTitle->setBounds (bounds = { xPos, yPos, 400, 50 });
    optionsPanelTitle->setFont (FontOptions ("Inter", "Bold", 20.0f));
    optionsPanel->addAndMakeVisible (optionsPanelTitle);
    opBounds = opBounds.getUnion (bounds);

    Font subtitleFont ("Inter", "Semi Bold", 18.0f);
    Font labelFont ("Inter", "Regular", 15.0f);

    /** ############## EVENT CRITERIA ############## */

    criteriaGroupSet = new VerticalGroupSet ("Event criteria controls", findColour (ThemeColours::componentBackground));
    optionsPanel->addAndMakeVisible (criteriaGroupSet, 0);

    xPos = LEFT_EDGE;
    yPos += 40;

    criteriaTitle = new Label ("criteriaTitle", "Event criteria");
    criteriaTitle->setBounds (bounds = { xPos, yPos, 200, 50 });
    criteriaTitle->setFont (subtitleFont);
    optionsPanel->addAndMakeVisible (criteriaTitle);
    opBounds = opBounds.getUnion (bounds);

    /* --------------- Jump limiting ------------------ */
    yPos += 45;

    limitButton = new ToggleButton ("Limit jump size across threshold (|X[k] - X[k-1]|)");
    limitButton->setBounds (bounds = { xPos, yPos, 420, C_TEXT_HT });
    limitButton->setToggleState ((bool) processor->getParameter ("use_jump_limit")->getValue(), dontSendNotification);
    limitButton->addListener (this);
    optionsPanel->addAndMakeVisible (limitButton);
    opBounds = opBounds.getUnion (bounds);

    limitLabel = new Label ("LimitL", "Maximum jump size:");
    limitLabel->setFont (labelFont);
    limitLabel->setBounds (bounds = { xPos += TAB_WIDTH, yPos += 30, 140, C_TEXT_HT });
    optionsPanel->addAndMakeVisible (limitLabel);
    opBounds = opBounds.getUnion (bounds);

    limitEditable = createEditable ("LimitE", String ((float) processor->getParameter ("jump_limit")->getValue()), "", bounds = { xPos += 150, yPos, 50, C_TEXT_HT });
    limitEditable->setEnabled (limitButton->getToggleState());
    optionsPanel->addAndMakeVisible (limitEditable);
    opBounds = opBounds.getUnion (bounds);

    xPos = LEFT_EDGE;
    limitSleepLabel = new Label ("LimitSL", "Sleep after artifact:");
    limitSleepLabel->setFont (labelFont);
    limitSleepLabel->setBounds (bounds = { xPos += TAB_WIDTH, yPos += 30, 140, C_TEXT_HT });
    optionsPanel->addAndMakeVisible (limitSleepLabel);
    opBounds = opBounds.getUnion (bounds);

    auto currStream = processor->getDataStream (processor->getSelectedStream());
    limitSleepEditable = createEditable ("LimitSE", String ((float) processor->getParameter ("jump_limit_sleep")->getValue()), "", bounds = { xPos += 150, yPos, 50, C_TEXT_HT });
    limitSleepEditable->setEnabled (limitButton->getToggleState());
    optionsPanel->addAndMakeVisible (limitSleepEditable);
    opBounds = opBounds.getUnion (bounds);

    criteriaGroupSet->addGroup ({ limitButton, limitLabel, limitEditable, limitSleepLabel, limitSleepEditable });

    /* --------------- Sample voting ------------------ */
    xPos = LEFT_EDGE;
    yPos += 40;

    votingHeader = new Label ("VotingHeadL", "Sample voting:");
    votingHeader->setFont (labelFont);
    votingHeader->setBounds (bounds = { xPos, yPos, 120, C_TEXT_HT });
    optionsPanel->addAndMakeVisible (votingHeader);
    opBounds = opBounds.getUnion (bounds);

    xPos += TAB_WIDTH;
    yPos += 30;

    pastStrictLabel = new Label ("PastStrictL", "Require");
    pastStrictLabel->setFont (labelFont);
    pastStrictLabel->setBounds (bounds = { xPos, yPos, 65, C_TEXT_HT });
    pastStrictLabel->setJustificationType (Justification::centredRight);
    optionsPanel->addAndMakeVisible (pastStrictLabel);
    opBounds = opBounds.getUnion (bounds);

    pastPctEditable = createEditable ("PastPctE", String (100 * (float) processor->getParameter ("past_strict")->getValue()), "", bounds = { xPos += 75, yPos, 35, C_TEXT_HT });
    optionsPanel->addAndMakeVisible (pastPctEditable);
    opBounds = opBounds.getUnion (bounds);

    pastPctLabel = new Label ("PastPctL", "% of the");
    pastPctLabel->setFont (labelFont);
    pastPctLabel->setBounds (bounds = { xPos += 35, yPos, 70, C_TEXT_HT });
    optionsPanel->addAndMakeVisible (pastPctLabel);
    opBounds = opBounds.getUnion (bounds);

    pastSpanEditable = createEditable ("PastSpanE", String ((int) processor->getParameter ("past_span")->getValue()), "", bounds = { xPos += 70, yPos, 45, C_TEXT_HT });
    optionsPanel->addAndMakeVisible (pastSpanEditable);
    opBounds = opBounds.getUnion (bounds);

    pastSpanLabel = new Label ("PastSpanL", "samples immediately before X[k-1]...");
    pastSpanLabel->setFont (labelFont);
    pastSpanLabel->setBounds (bounds = { xPos += 50, yPos, 260, C_TEXT_HT });
    optionsPanel->addAndMakeVisible (pastSpanLabel);
    opBounds = opBounds.getUnion (bounds);

    xPos = LEFT_EDGE + TAB_WIDTH;
    yPos += 30;

    futureStrictLabel = new Label ("FutureStrictL", "...and");
    futureStrictLabel->setFont (labelFont);
    futureStrictLabel->setBounds (bounds = { xPos, yPos, 65, C_TEXT_HT });
    futureStrictLabel->setJustificationType (Justification::centredRight);
    optionsPanel->addAndMakeVisible (futureStrictLabel);
    opBounds = opBounds.getUnion (bounds);

    futurePctEditable = createEditable ("FuturePctE", String (100 * (float) processor->getParameter ("future_strict")->getValue()), "", bounds = { xPos += 75, yPos, 35, C_TEXT_HT });
    optionsPanel->addAndMakeVisible (futurePctEditable);
    opBounds = opBounds.getUnion (bounds);

    futurePctLabel = new Label ("FuturePctL", "% of the");
    futurePctLabel->setFont (labelFont);
    futurePctLabel->setBounds (bounds = { xPos += 35, yPos, 70, C_TEXT_HT });
    optionsPanel->addAndMakeVisible (futurePctLabel);
    opBounds = opBounds.getUnion (bounds);

    futureSpanEditable = createEditable ("FutureSpanE", String ((int) processor->getParameter ("future_span")->getValue()), "", bounds = { xPos += 70, yPos, 45, C_TEXT_HT });
    optionsPanel->addAndMakeVisible (futureSpanEditable);
    opBounds = opBounds.getUnion (bounds);

    futureSpanLabel = new Label ("FutureSpanL", "samples immediately after X[k]...");
    futureSpanLabel->setFont (labelFont);
    futureSpanLabel->setBounds (bounds = { xPos += 50, yPos, 260, C_TEXT_HT });
    optionsPanel->addAndMakeVisible (futureSpanLabel);
    opBounds = opBounds.getUnion (bounds);

    xPos = LEFT_EDGE + TAB_WIDTH + 10;
    yPos += 30;

    votingFooter = new Label ("VotingFootL", "...to be on the correct side of the threshold.");
    votingFooter->setFont (labelFont);
    votingFooter->setBounds (bounds = { xPos, yPos, 350, C_TEXT_HT });
    optionsPanel->addAndMakeVisible (votingFooter);
    opBounds = opBounds.getUnion (bounds);

    criteriaGroupSet->addGroup ({ votingHeader,
                                  pastStrictLabel,
                                  pastPctEditable,
                                  pastPctLabel,
                                  pastSpanEditable,
                                  pastSpanLabel,
                                  futureStrictLabel,
                                  futurePctEditable,
                                  futurePctLabel,
                                  futureSpanEditable,
                                  futureSpanLabel,
                                  votingFooter });

    /* --------------- Buffer end mask ----------------- */
    xPos = LEFT_EDGE;
    yPos += 40;

    static const String bufferMaskTT =
        "Each time a new buffer of samples is received, the samples closer to the start have "
        "been waiting to be processed for longer than those at the end, but an event triggered "
        "from any of them will be handled by the rest of the chain at the same time. This adds "
        "some variance to the latency between data and reaction in a closed-loop scenario. Enable "
        "this option to just ignore any crossings before a threshold measured from the end of the buffer.";

    bufferMaskButton = new ToggleButton ("Ignore crossings ocurring >");
    bufferMaskButton->setBounds (bounds = { xPos, yPos, 225, C_TEXT_HT });
    bufferMaskButton->setToggleState ((bool) processor->getParameter ("use_buffer_end_mask")->getValue(), dontSendNotification);
    bufferMaskButton->addListener (this);
    bufferMaskButton->setTooltip (bufferMaskTT);
    optionsPanel->addAndMakeVisible (bufferMaskButton);
    opBounds = opBounds.getUnion (bounds);

    bufferMaskEditable = createEditable ("BufMaskE", String ((int) processor->getParameter ("buffer_end_mask")->getValue()), bufferMaskTT, bounds = { xPos += 225, yPos, 40, C_TEXT_HT });
    bufferMaskEditable->setEnabled (bufferMaskButton->getToggleState());
    optionsPanel->addAndMakeVisible (bufferMaskEditable);
    opBounds = opBounds.getUnion (bounds);

    bufferMaskLabel = new Label ("BufMaskL", "ms before the end of a buffer.");
    bufferMaskLabel->setFont (labelFont);
    bufferMaskLabel->setBounds (bounds = { xPos += 45, yPos, 250, C_TEXT_HT });
    bufferMaskLabel->setTooltip (bufferMaskTT);
    optionsPanel->addAndMakeVisible (bufferMaskLabel);
    opBounds = opBounds.getUnion (bounds);

    criteriaGroupSet->addGroup ({ bufferMaskButton, bufferMaskEditable, bufferMaskLabel });

    // some extra padding
    opBounds.setBottom (opBounds.getBottom() + 10);
    opBounds.setRight (opBounds.getRight() + 10);

    optionsPanel->setBounds (opBounds);
    criteriaGroupSet->setBounds (opBounds);
}

void CrossingDetectorCanvas::labelTextChanged (Label* labelThatHasChanged)
{
    if (labelThatHasChanged == pastPctEditable)
    {
        float newVal;
        float prevVal = (float) processor->getParameter ("past_strict")->getValue();
        if (updateFloatLabel (labelThatHasChanged, 0, 100, 100 * prevVal, &newVal))
        {
            processor->getParameter ("past_strict")->setNextValue (newVal / 100);
        }
    }
    else if (labelThatHasChanged == pastSpanEditable)
    {
        int newVal;
        int prevVal = (int) processor->getParameter ("past_span")->getValue();
        if (updateIntLabel (labelThatHasChanged, 0, INT_MAX, prevVal, &newVal))
        {
            processor->getParameter ("past_strict")->setNextValue (newVal);
        }
    }
    else if (labelThatHasChanged == futurePctEditable)
    {
        float newVal;
        float prevVal = (float) processor->getParameter ("future_strict")->getValue();
        if (updateFloatLabel (labelThatHasChanged, 0, 100, 100 * prevVal, &newVal))
        {
            processor->getParameter ("future_strict")->setNextValue (newVal / 100);
        }
    }
    else if (labelThatHasChanged == futureSpanEditable)
    {
        int newVal;
        int prevVal = (int) processor->getParameter ("future_span")->getValue();
        if (updateIntLabel (labelThatHasChanged, 0, INT_MAX, prevVal, &newVal))
        {
            processor->getParameter ("future_span")->setNextValue (newVal);
        }
    }

    // Event criteria editable labels
    else if (labelThatHasChanged == limitEditable)
    {
        float newVal;
        float prevVal = (float) processor->getParameter ("jump_limit")->getValue();
        if (updateFloatLabel (labelThatHasChanged, 0, FLT_MAX, prevVal, &newVal))
        {
            processor->getParameter ("jump_limit")->setNextValue (newVal);
        }
    }
    else if (labelThatHasChanged == limitSleepEditable)
    {
        float newVal;
        float prevVal = (float) processor->getParameter ("jump_limit_sleep")->getValue();
        if (updateFloatLabel (labelThatHasChanged, 0, FLT_MAX, prevVal, &newVal))
        {
            processor->getParameter ("jump_limit_sleep")->setNextValue (newVal);
        }
    }
    else if (labelThatHasChanged == bufferMaskEditable)
    {
        int newVal;
        int prevVal = (int) processor->getParameter ("buffer_end_mask")->getValue();
        if (updateIntLabel (labelThatHasChanged, 0, INT_MAX, prevVal, &newVal))
        {
            processor->getParameter ("buffer_end_mask")->setNextValue (newVal);
        }
    }
}

void CrossingDetectorCanvas::buttonClicked (Button* button)
{
    // Buttons for event criteria
    if (button == limitButton)
    {
        bool limitOn = button->getToggleState();
        limitEditable->setEnabled (limitOn);
        limitSleepEditable->setEnabled (limitOn);
        processor->getParameter ("use_jump_limit")->setNextValue (limitOn);
    }
    else if (button == bufferMaskButton)
    {
        bool bufMaskOn = button->getToggleState();
        bufferMaskEditable->setEnabled (bufMaskOn);
        processor->getParameter ("use_buffer_end_mask")->setNextValue (bufMaskOn);
    }
}

void CrossingDetectorCanvas::updateSettings()
{
}

/**************** private ******************/

Label* CrossingDetectorCanvas::createEditable (const String& name, const String& initialValue, const String& tooltip, juce::Rectangle<int> bounds)
{
    Label* editable = new Label (name, initialValue);
    editable->setEditable (true);
    editable->addListener (this);
    editable->setBounds (bounds);
    editable->setColour (Label::outlineColourId, findColour (ThemeColours::outline));

    if (tooltip.length() > 0)
    {
        editable->setTooltip (tooltip);
    }
    return editable;
}

/* Attempts to parse the current text of a label as an int between min and max inclusive.
*  If successful, sets "*out" and the label text to this value and and returns true.
*  Otherwise, sets the label text to defaultValue and returns false.
*/
bool CrossingDetectorCanvas::updateIntLabel (Label* label, int min, int max, int defaultValue, int* out)
{
    const String& in = label->getText();
    int parsedInt;
    try
    {
        parsedInt = std::stoi (in.toRawUTF8());
    }
    catch (const std::logic_error&)
    {
        label->setText (String (defaultValue), dontSendNotification);
        return false;
    }

    *out = jmax (min, jmin (max, parsedInt));

    label->setText (String (*out), dontSendNotification);
    return true;
}

// Like updateIntLabel, but for floats
bool CrossingDetectorCanvas::updateFloatLabel (Label* label, float min, float max, float defaultValue, float* out)
{
    const String& in = label->getText();
    float parsedFloat;
    try
    {
        parsedFloat = std::stof (in.toRawUTF8());
    }
    catch (const std::logic_error&)
    {
        label->setText (String (defaultValue), dontSendNotification);
        return false;
    }

    *out = jmax (min, jmin (max, parsedFloat));

    label->setText (String (*out), dontSendNotification);
    return true;
}

/************ VerticalGroupSet ****************/

VerticalGroupSet::VerticalGroupSet (Colour backgroundColor)
    : Component(), bgColor (backgroundColor), leftBound (INT_MAX), rightBound (INT_MIN)
{
}

VerticalGroupSet::VerticalGroupSet (const String& componentName, Colour backgroundColor)
    : Component (componentName), bgColor (backgroundColor), leftBound (INT_MAX), rightBound (INT_MIN)
{
}

VerticalGroupSet::~VerticalGroupSet() {}

void VerticalGroupSet::paint (Graphics& g)
{
    Colour themeBgColour = findColour (ThemeColours::componentBackground);

    if (bgColor != themeBgColour)
    {
        setBackgroundColour (themeBgColour);
    }
}

void VerticalGroupSet::addGroup (std::initializer_list<Component*> components)
{
    if (! getParentComponent())
    {
        jassertfalse;
        return;
    }

    DrawableRectangle* thisGroup;
    groups.add (thisGroup = new DrawableRectangle);
    addChildComponent (thisGroup);
    const Point<float> cornerSize (CORNER_SIZE, CORNER_SIZE);
    thisGroup->setCornerSize (cornerSize);
    thisGroup->setFill (bgColor);

    int topBound = INT_MAX;
    int bottomBound = INT_MIN;
    for (auto component : components)
    {
        Component* componentParent = component->getParentComponent();
        if (! componentParent)
        {
            jassertfalse;
            return;
        }
        int width = component->getWidth();
        int height = component->getHeight();
        juce::Point<int> positionFromItsParent = component->getPosition();
        juce::Point<int> localPosition = getLocalPoint (componentParent, positionFromItsParent);

        // update bounds
        leftBound = jmin (leftBound, localPosition.x - PADDING);
        rightBound = jmax (rightBound, localPosition.x + width + PADDING);
        topBound = jmin (topBound, localPosition.y - PADDING);
        bottomBound = jmax (bottomBound, localPosition.y + height + PADDING);
    }

    // set new background's bounds
    auto bounds = juce::Rectangle<float>::leftTopRightBottom (leftBound, topBound, rightBound, bottomBound);
    thisGroup->setRectangle (bounds);
    thisGroup->setVisible (true);

    // update all other children
    for (DrawableRectangle* group : groups)
    {
        if (group == thisGroup)
        {
            continue;
        }

        topBound = group->getPosition().y;
        bottomBound = topBound + group->getHeight();
        bounds = juce::Rectangle<float>::leftTopRightBottom (leftBound, topBound, rightBound, bottomBound);
        group->setRectangle (bounds);
    }
}

void VerticalGroupSet::setBackgroundColour (Colour newColour)
{
    bgColor = newColour;
    for (DrawableRectangle* group : groups)
    {
        group->setFill (bgColor);
    }
}