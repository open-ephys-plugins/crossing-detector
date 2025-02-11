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

#ifndef __THRESHOLDCONFIGPOPUP_H_A76FFF40__
#define __THRESHOLDCONFIGPOPUP_H_A76FFF40__

#include "CrossingDetector.h"

class CrossingDetectorEditor;

/**
*   Tabbed Component used to edit Threshold settings
*/
class ThresholdTabbedComponent : public TabbedComponent
{
public:
    /** Constructor */
    ThresholdTabbedComponent (CrossingDetector* processor, int selectedTab, int thresholdChan = -1);

    /** Destructor */
    ~ThresholdTabbedComponent();

    void currentTabChanged (int newCurrentTabIndex, const String& newCurrentTabName) override;

    void thresholdChannelChanged();

private:
    CrossingDetector* processor;

    OwnedArray<ParameterEditor> thresholdParamEditors;

    std::unique_ptr<Label> chanThreshLabel;
    std::unique_ptr<ComboBox> channelThreshBox;

    std::unique_ptr<ToggleButton> enabledButton;
};

/**
 * Popup window used to hold the ThresholdTabbedComponent
 */

class ThresholdConfigPopup : public PopupComponent
{
public:
    /** Constructor */
    ThresholdConfigPopup (Component* parent, CrossingDetector* processor, int selectedThreshold, int thresholdChan);

    /** Destructor */
    ~ThresholdConfigPopup() {}

    void updatePopup() override;

private:
    std::unique_ptr<ThresholdTabbedComponent> thresholdTabbedComponent;
    CrossingDetector* processor;
    Component* parentComponent;
};

#endif // __THRESHOLDCONFIGPOPUP_H_A76FFF40__
