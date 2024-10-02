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
*   Popup window used to edit Spike Channel settings
*/
class ThresholdConfigComponent : public TabbedComponent
{

public:
    
    /** Constructor */
    ThresholdConfigComponent(CrossingDetector* processor, int selectedTab, int thresholdChan = -1);

    /** Destructor */
    ~ThresholdConfigComponent();

    void currentTabChanged (int newCurrentTabIndex, const String &newCurrentTabName) override;

    void thresholdChannelChanged();

private:
    CrossingDetector* processor;

    OwnedArray<ParameterEditor> thresholdParamEditors;
    
    std::unique_ptr<Label> chanThreshLabel;
    std::unique_ptr<ComboBox> channelThreshBox;

    std::unique_ptr<ToggleButton> enabledButton;
};


#endif  // __THRESHOLDCONFIGPOPUP_H_A76FFF40__
