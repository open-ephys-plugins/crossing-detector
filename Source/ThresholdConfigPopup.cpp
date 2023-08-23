/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2014 Open Ephys

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

#include "ThresholdConfigPopup.h"

#include <stdio.h>

ThresholdConfigComponent::ThresholdConfigComponent(CrossingDetector* processor_, int selectedTab, int thresholdChan) 
    : TabbedComponent(TabbedButtonBar::TabsAtTop),
      processor(processor_)
{
    setName("Threshold Type");

    setSize(240, 75);
    setTabBarDepth(30);
    getTabbedButtonBar().setLookAndFeel(&tabLookNFeel);

    /* Constant Threshold */
    Component* constThreshComp = new Component("Constant Threshold Component");
    constThreshComp->setBounds(0, 0, 240, 45);

    TextBoxParameterEditor* threshEditor = new TextBoxParameterEditor(processor->getParameter("constant_threshold"), 25, 220);
    threshEditor->setLayout(ParameterEditor::Layout::nameOnLeft);
    thresholdParamEditors.add(threshEditor);
    constThreshComp->addAndMakeVisible(threshEditor);
    threshEditor->setTopLeftPosition(10, 10);
    addTab("Constant", findColour(ThemeColors::editorGradientColorId1), constThreshComp, true);
    

    /* Random Threshold */
    Component* randomThresh = new Component("Random Threshold");
    randomThresh->setBounds(0, 0, 240, 90);

    TextBoxParameterEditor* minRandEditor = new TextBoxParameterEditor(processor->getParameter("min_random_threshold"), 22, 220);
    minRandEditor->setLayout(ParameterEditor::Layout::nameOnLeft);
    thresholdParamEditors.add(minRandEditor);

    TextBoxParameterEditor* maxRandEditor = new TextBoxParameterEditor(processor->getParameter("max_random_threshold"), 22, 220);
    thresholdParamEditors.add(maxRandEditor);
    maxRandEditor->setLayout(ParameterEditor::Layout::nameOnLeft);

    randomThresh->addAndMakeVisible(minRandEditor);
    minRandEditor->setTopLeftPosition(10, 10);
    randomThresh->addAndMakeVisible(maxRandEditor);
    maxRandEditor->setTopLeftPosition(10, 50);

    addTab("Random", Colours::grey, randomThresh, true);

    /* Reference Channel Threshold */
    Component* chanThreshComp = new Component("Channel Threshold");
    chanThreshComp->setBounds(0, 0, 240, 45);

    chanThreshLabel = std::make_unique<Label>("Channel Label", "Reference Channel");
    chanThreshLabel->setFont(Font("Arial", "Regular", int(0.75*22)));
    chanThreshLabel->setBounds(10, 10, 120, 25);
    chanThreshComp->addAndMakeVisible(chanThreshLabel.get());
  
    // update channel threshold combo box
    int numChans = 0;
    uint16 selStreamId = processor->getSelectedStream();
    auto stream = processor->getDataStream(selStreamId);

    numChans = stream->getChannelCount();

    channelThreshBox = std::make_unique<ComboBox>("channelSelection");
    channelThreshBox->setBounds(140, 12, 90, 22);
    channelThreshBox->setTooltip(
        "Only channels from the same stream as the input (but not the input itself) "
        "can be selected.");
    channelThreshBox->onChange = [this] { thresholdChannelChanged(); };
    chanThreshComp->addAndMakeVisible(channelThreshBox.get());


    for (int chan = 1; chan <= numChans; ++chan)
    {
        if(processor->isCompatibleWithInput(chan - 1))
        {
            String chanName =  stream->getContinuousChannels()[chan - 1]->getName();
            channelThreshBox->addItem(chanName, chan);
            if (thresholdChan == (chan - 1))
            {
                channelThreshBox->setSelectedId(chan, dontSendNotification);
            }
        }
    }

    if (channelThreshBox->getSelectedId() == 0 && channelThreshBox->getNumItems() > 0)
    {
        channelThreshBox->setSelectedItemIndex(0, sendNotification);
    }

    addTab("Channel", Colours::grey, chanThreshComp, true);

    setCurrentTabIndex(selectedTab, false);
}


ThresholdConfigComponent::~ThresholdConfigComponent()
{
    getTabbedButtonBar().setLookAndFeel(nullptr);
}

void ThresholdConfigComponent::currentTabChanged(int newCurrentTabIndex, const String &newCurrentTabName)
{
    if(newCurrentTabIndex == ThresholdType::CONSTANT)
    {
        setSize(240, 75);
        setTabBackgroundColour(0, findColour(ThemeColors::editorGradientColorId1));
        setTabBackgroundColour(1, Colours::darkgrey);
        setTabBackgroundColour(2, Colours::darkgrey);
    }
    else if(newCurrentTabIndex == ThresholdType::RANDOM)
    {
        setSize(240, 120);
        setTabBackgroundColour(0, Colours::darkgrey);
        setTabBackgroundColour(1, findColour(ThemeColors::editorGradientColorId1));
        setTabBackgroundColour(2, Colours::darkgrey);
    }
    else if(newCurrentTabIndex == ThresholdType::CHANNEL)
    {
        setSize(240, 75);
        setTabBackgroundColour(0, Colours::darkgrey);
        setTabBackgroundColour(1, Colours::darkgrey);
        setTabBackgroundColour(2, findColour(ThemeColors::editorGradientColorId1));
    }

    processor->getParameter("threshold_type")->setNextValue(newCurrentTabIndex);
}

void ThresholdConfigComponent::thresholdChannelChanged()
{
    auto currStream = processor->getDataStream(processor->getSelectedStream());
    currStream->getParameter("threshold_chan")->setNextValue(channelThreshBox->getSelectedId() - 1);
}
