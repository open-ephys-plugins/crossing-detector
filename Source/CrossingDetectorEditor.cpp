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
#include <string>  // stoi, stof
#include <climits> // INT_MAX
#include <cfloat>  // FLT_MAX


CustomButton::CustomButton(Parameter* param, StringRef label) : ParameterEditor(param)
{
    button = std::make_unique<UtilityButton>(label, Font("Fira Code", "Regular", 14.0f));
    button->addListener(this);
    button->setClickingTogglesState(true);
    addAndMakeVisible(button.get());
    setBounds(0, 0, 60, 18);
}

CrossingDetectorEditor::CrossingDetectorEditor(GenericProcessor* parentNode)
    : VisualizerEditor(parentNode, "Crossing Detector", 205)
{

    /* ------------- Top row (channels) ------------- */
    int xPos = 12;
    int yPos = 25;

    addSelectedChannelsParameterEditor("Channel", xPos, yPos);

    addComboBoxParameterEditor("Out", xPos + 70, yPos);

    /* ------------ Middle row (conditions) -------------- */
    xPos = 20;
    const int Y_MID = yPos + 48;
    const int Y_GAP = 2;
    const int Y_POS_UPPER = Y_MID - (18 + Y_GAP / 2);
    const int Y_POS_LOWER = Y_MID + Y_GAP / 2;

    Parameter* customParam = getProcessor()->getParameter("Rising");
    addCustomParameterEditor(new CustomButton(customParam, "Rising"), xPos, Y_POS_UPPER);

    customParam = getProcessor()->getParameter("Falling");
    addCustomParameterEditor(new CustomButton(customParam, "Falling"), xPos, Y_POS_LOWER);

    addTextBoxParameterEditor("Timeout (ms)", xPos + 70, Y_POS_UPPER);
    // addTextBoxParameterEditor("Threshold", xPos + 70, Y_POS_UPPER);

    /* --------- Bottom row (timeout) ------------- */
    xPos = 30;
    yPos = Y_MID + 24;

    addTextBoxParameterEditor("Threshold", xPos, yPos);
}

CrossingDetectorEditor::~CrossingDetectorEditor() {}

Visualizer* CrossingDetectorEditor::createNewCanvas()
{
    canvas = std::make_unique<CrossingDetectorCanvas>(getProcessor());
    return canvas.get();
}
