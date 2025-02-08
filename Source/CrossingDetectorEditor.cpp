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

#include "CrossingDetectorEditor.h"
#include "CrossingDetectorCanvas.h"
#include <cfloat> // FLT_MAX
#include <climits> // INT_MAX
#include <string> // stoi, stof

CustomButton::CustomButton (Parameter* param, String label) : ParameterEditor (param)
{
    button = std::make_unique<UtilityButton> (label);
    button->addListener (this);
    button->setClickingTogglesState (true);
    button->setToggleState (param->getDefaultValue(), dontSendNotification);
    addAndMakeVisible (button.get());
    setBounds (0, 0, 80, 22);
}

void CustomButton::buttonClicked (Button*)
{
    param->setNextValue (button->getToggleState());
}

void CustomButton::updateView()
{
    if (param != nullptr)
        button->setToggleState (param->getValue(), dontSendNotification);
}

void CustomButton::resized()
{
    button->setBounds (0, 0, 80, 22);
}

CrossingDetectorEditor::CrossingDetectorEditor (GenericProcessor* parentNode)
    : VisualizerEditor (parentNode, "Crossing Detector", 310), thresholdConfig (nullptr)
{
    addSelectedChannelsParameterEditor (Parameter::STREAM_SCOPE, "channel", 15, 25);
    getParameterEditor ("channel")->setLayout (ParameterEditor::nameOnTop);
    getParameterEditor ("channel")->setSize (80, 40);

    addTtlLineParameterEditor (Parameter::STREAM_SCOPE, "ttl_out", 110, 25);
    getParameterEditor ("ttl_out")->setLayout (ParameterEditor::nameOnTop);
    getParameterEditor ("ttl_out")->setSize (80, 40);

    Parameter* customParam = getProcessor()->getParameter ("rising");
    addCustomParameterEditor (new CustomButton (customParam, "Rising"), 15, 73);

    customParam = getProcessor()->getParameter ("falling");
    addCustomParameterEditor (new CustomButton (customParam, "Falling"), 15, 95);

    addBoundedValueParameterEditor (Parameter::PROCESSOR_SCOPE, "event_duration", 110, 75);
    getParameterEditor ("event_duration")->setLayout (ParameterEditor::nameOnTop);
    getParameterEditor ("event_duration")->setSize (90, 40);

    thresholdLabel = std::make_unique<Label> ("ThresholdLabel", "Threshold");
    thresholdLabel->setBounds (210, 25, 90, 20);
    thresholdLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
    thresholdLabel->setJustificationType (Justification::centredLeft);
    addAndMakeVisible (thresholdLabel.get());

    thresholdTypeButton = std::make_unique<UtilityButton> ("Constant");
    thresholdTypeButton->setFont (FontOptions ("Inter", "Regular", 13.0f));
    thresholdTypeButton->addListener (this);
    thresholdTypeButton->setRadius (3.0f);
    thresholdTypeButton->setBounds (210, 45, 90, 20);
    addAndMakeVisible (thresholdTypeButton.get());

    addBoundedValueParameterEditor (Parameter::PROCESSOR_SCOPE, "timeout", 210, 75);
    getParameterEditor ("timeout")->setLayout (ParameterEditor::nameOnTop);
    getParameterEditor ("timeout")->setSize (90, 40);
}

CrossingDetectorEditor::~CrossingDetectorEditor() {}

Visualizer* CrossingDetectorEditor::createNewCanvas()
{
    CrossingDetectorCanvas* cdc = new CrossingDetectorCanvas (getProcessor());
    return cdc;
}

void CrossingDetectorEditor::selectedStreamHasChanged()
{
    CrossingDetector* processor = (CrossingDetector*) getProcessor();
    processor->setSelectedStream (getCurrentStream());
}

void CrossingDetectorEditor::buttonClicked (Button* button)
{
    if (button == thresholdTypeButton.get() && getCurrentStream() > 0)
    {
        CrossingDetector* processor = (CrossingDetector*) getProcessor();

        int selectedThreshold = (int) processor->getParameter ("threshold_type")->getValue();

        int thresholdChan = (int) processor->getDataStream (getCurrentStream())->getParameter ("threshold_chan")->getValue();

        thresholdConfig = new ThresholdConfigComponent (processor, selectedThreshold, thresholdChan);

        CallOutBox& myBox = CallOutBox::launchAsynchronously (std::unique_ptr<Component> (thresholdConfig),
                                                              button->getScreenBounds(),
                                                              nullptr);

        myBox.setDismissalMouseClicksAreAlwaysConsumed (true);

        return;
    }
}

void CrossingDetectorEditor::updateThresholdButtonText (const String& text)
{
    thresholdTypeButton->setLabel (text);
}
