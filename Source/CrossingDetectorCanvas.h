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

#ifndef CROSSING_DETECTOR_CANVAS_H_INCLUDED
#define CROSSING_DETECTOR_CANVAS_H_INCLUDED

#include <VisualizerWindowHeaders.h>
#include "CrossingDetectorEditor.h"
#include "CrossingDetector.h"

/*
Canvas/visualizer contains:
- Threshold type selection - constant, adaptive, random, or channel (with parameters)
- Jump limiting toggle and max jump box
- Voting settings (pre/post event span and strictness)
- Event duration control

@see Visualizer
*/


// LookAndFeel class with radio-button-style ToggleButton
class RadioButtonLookAndFeel : public LookAndFeel_V2
{
    void drawTickBox(Graphics& g, Component& component, float x, float y, float w, float h,
        const bool ticked, const bool isEnabled,
        const bool isMouseOverButton, const bool isButtonDown) override;
};

/* Renders a rounded rectangular component behind and encompassing each group of
 * components added, with matching widths. Components of each group are not added
 * as children to the groupset or backgrounds; they are just used to position the backgrounds.
 * Each component passed in must already have a parent.
 */
class VerticalGroupSet : public Component
{
public:
    VerticalGroupSet(Colour backgroundColor = Colours::silver);
    VerticalGroupSet(const String& componentName, Colour backgroundColor = Colours::silver);
    ~VerticalGroupSet();

    void addGroup(std::initializer_list<Component*> components);

private:
    Colour bgColor;
    int leftBound;
    int rightBound;
    OwnedArray<DrawableRectangle> groups;
    static const int PADDING = 5;
    static const int CORNER_SIZE = 8;
};

// Visualizer window containing additional settings

class CrossingDetectorCanvas : public Visualizer,
                               public ComboBox::Listener,
                               public Label::Listener,
                               public Button::Listener
{
public:
    CrossingDetectorCanvas(GenericProcessor* n);
    ~CrossingDetectorCanvas();

    void refreshState() override;
    void update() override;
    void refresh() override;
    // void beginAnimation() override;
    // void endAnimation() override;

    void paint(Graphics& g) override;
    void resized() override;

    void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;
    void labelTextChanged(Label* labelThatHasChanged) override;
    void buttonClicked(Button* button) override;

private:
    ScopedPointer<Viewport> viewport;
    
    CrossingDetector* processor;
    CrossingDetectorEditor* editor;

    // Basic UI element creation methods. Always register "this" (the editor) as the listener,
    // but may specify a different Component in which to actually display the element.
    Label* createEditable(const String& name, const String& initialValue,
        const String& tooltip, juce::Rectangle<int> bounds);
    Label* createLabel(const String& name, const String& text, juce::Rectangle<int> bounds);

    /* Utilities for parsing entered values
    *  Ouput whether the label contained a valid input; if so, it is stored in *out
    *  and the label is updated with the parsed input. Otherwise, the label is reset
    *  to defaultValue.
    */
    
    static bool updateIntLabel(Label* label, int min, int max,
        int defaultValue, int* out);
    static bool updateFloatLabel(Label* label, float min, float max,
        float defaultValue, float* out);
    

    void initializeOptionsPanel();

    RadioButtonLookAndFeel rbLookAndFeel;

    // --- Canvas elements are managed by editor but invisible until visualizer is opened ----
    ScopedPointer<Component> optionsPanel;

    ScopedPointer<Label> optionsPanelTitle;
    
    /****** threshold section ******/

    ScopedPointer<Label> thresholdTitle;
    const static int threshRadioId = 1;
    ScopedPointer<VerticalGroupSet> thresholdGroupSet;

    ScopedPointer<ToggleButton> constantThreshButton;
    ScopedPointer<Label> constantThreshValue;

    // threshold randomization
    ScopedPointer<ToggleButton> randomizeButton;
    ScopedPointer<Label> minThreshLabel;
    ScopedPointer<Label> minThreshEditable;
    ScopedPointer<Label> maxThreshLabel;
    ScopedPointer<Label> maxThreshEditable;

    // threshold from channel
    ScopedPointer<ToggleButton> channelThreshButton;
    ScopedPointer<ComboBox> channelThreshBox;

    /******* criteria section *******/

    ScopedPointer<Label> criteriaTitle;
    ScopedPointer<VerticalGroupSet> criteriaGroupSet;

    // jump limiting
    ScopedPointer<ToggleButton> limitButton;
    ScopedPointer<Label> limitLabel;
    ScopedPointer<Label> limitEditable;
    ScopedPointer<Label> limitSleepLabel;
    ScopedPointer<Label> limitSleepEditable;

    // sample voting
    ScopedPointer<Label> votingHeader;
    
    ScopedPointer<Label> pastStrictLabel;
    ScopedPointer<Label> pastPctEditable;
    ScopedPointer<Label> pastPctLabel;
    ScopedPointer<Label> pastSpanEditable;
    ScopedPointer<Label> pastSpanLabel;

    ScopedPointer<Label> futureStrictLabel;
    ScopedPointer<Label> futurePctEditable;
    ScopedPointer<Label> futurePctLabel;
    ScopedPointer<Label> futureSpanLabel;
    ScopedPointer<Label> futureSpanEditable;

    ScopedPointer<Label> votingFooter;

    // buffer end mask
    ScopedPointer<ToggleButton> bufferMaskButton;
    ScopedPointer<Label> bufferMaskEditable;
    ScopedPointer<Label> bufferMaskLabel;

    /******** output section *******/

    ScopedPointer<Label> outputTitle;
    ScopedPointer<VerticalGroupSet> outputGroupSet;

    // event duration
    ScopedPointer<Label> durationLabel;
    ScopedPointer<Label> durationEditable;
    ScopedPointer<Label> durationUnit;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrossingDetectorCanvas);
};

#endif // CROSSING_DETECTOR_CANVAS_H_INCLUDED
