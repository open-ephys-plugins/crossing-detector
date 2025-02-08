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

#ifndef CROSSING_DETECTOR_EDITOR_H_INCLUDED
#define CROSSING_DETECTOR_EDITOR_H_INCLUDED

#include "CrossingDetector.h"
#include "ThresholdConfigPopup.h"
#include <VisualizerEditorHeaders.h>

/*
Editor (in signal chain) contains:
- Input channel selector
- Ouptput event channel selector
- Direction ("rising" and "falling") buttons
- Threshold control (and indicator when random or channel threshold is selected)
- Event timeout control

@see GenericEditor
*/

class CustomButton
    : public ParameterEditor,
      public Button::Listener
{
public:
    /** Constructor*/
    CustomButton (Parameter* param, String label);

    /** Destructor */
    ~CustomButton() {}

    /** Responds to button clicks*/
    void buttonClicked (Button* label) override;

    /** Ensures button state aligns with underlying parameter*/
    void updateView() override;

    /** Sets component layout*/
    void resized() override;

private:
    std::unique_ptr<UtilityButton> button;
};

class CrossingDetectorEditor
    : public VisualizerEditor,
      public Button::Listener
{
public:
    CrossingDetectorEditor (GenericProcessor* parentNode);
    ~CrossingDetectorEditor();

    Visualizer* createNewCanvas() override;

    /** Called when threshold type button is clicked */
    void buttonClicked (Button* button) override;

    void updateThresholdButtonText (const String& btnText);

    void selectedStreamHasChanged() override;

private:
    std::unique_ptr<UtilityButton> thresholdTypeButton;
    std::unique_ptr<Label> thresholdLabel;

    ThresholdConfigComponent* thresholdConfig;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CrossingDetectorEditor);
};

#endif // CROSSING_DETECTOR_EDITOR_H_INCLUDED
