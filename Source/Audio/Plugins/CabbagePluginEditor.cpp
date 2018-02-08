/*
  Copyright (C) 2016 Rory Walsh

  Cabbage is free software; you can redistribute it
  and/or modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  Cabbage is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU General Public
  License along with Csound; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
  02111-1307 USA
*/
#include "CabbagePluginEditor.h"

class CabbageCheckbox;

//==============================================================================
CabbagePluginEditor::CabbagePluginEditor (CabbagePluginProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      lookAndFeel(),
#ifdef Cabbage_IDE_Build
      layoutEditor (processor.cabbageWidgets),
#endif
      mainComponent()
{
    setName ("PluginEditor");
    setSize (400, 300);
    setLookAndFeel (&lookAndFeel);

    createEditorInterface (processor.cabbageWidgets);

    addAndMakeVisible (mainComponent);
    mainComponent.setInterceptsMouseClicks (false, true);

#ifdef Cabbage_IDE_Build
    addAndMakeVisible (layoutEditor);
    layoutEditor.setTargetComponent (&mainComponent);
    layoutEditor.updateFrames();
    layoutEditor.setEnabled (false);
    layoutEditor.toFront (false);
    layoutEditor.setInterceptsMouseClicks (true, true);
#endif
}

CabbagePluginEditor::~CabbagePluginEditor()
{
    popupPlants.clear();
    components.clear();
    setLookAndFeel (nullptr);
}

void CabbagePluginEditor::resized()
{
#ifdef Cabbage_IDE_Build
    layoutEditor.setBounds (getLocalBounds());
#endif
    mainComponent.setBounds (getLocalBounds());
}

//======================================================================================================
void CabbagePluginEditor::mouseMove (const MouseEvent& e)
{
    handleMouseMovement (e);
}

void CabbagePluginEditor::mouseDrag (const MouseEvent& e)
{
    handleMouseMovement (e);
}

void CabbagePluginEditor::mouseDown (const MouseEvent& e)
{
    handleMouseClicks (e, true);
}

void CabbagePluginEditor::mouseUp (const MouseEvent& e)
{
    handleMouseClicks (e, false);
}

void CabbagePluginEditor::handleMouseMovement (const MouseEvent& e)
{
    int x = e.eventComponent->getTopLevelComponent()->getMouseXYRelative().x;
    int yOffset = (CabbageUtilities::getTarget() == CabbageUtilities::TargetTypes::IDE ? 27 : 0 );
    int y = e.eventComponent->getTopLevelComponent()->getMouseXYRelative().y - yOffset; //27 is the height of the standalone window frame
    sendChannelDataToCsound (CabbageIdentifierIds::mousex, x);
    sendChannelDataToCsound (CabbageIdentifierIds::mousey, y);
}

void CabbagePluginEditor::handleMouseClicks (const MouseEvent& e, bool isMousePressed)
{
    if (e.mods.isLeftButtonDown())
        sendChannelDataToCsound (CabbageIdentifierIds::mousedownleft, (isMousePressed == true ? 1 : 0));
    else if (e.mods.isRightButtonDown())
        sendChannelDataToCsound (CabbageIdentifierIds::mousedownright, (isMousePressed == true ? 1 : 0));
    else if (e.mods.isMiddleButtonDown())
        sendChannelDataToCsound (CabbageIdentifierIds::mousedownlmiddle, (isMousePressed == true ? 1 : 0));
}
//==============================================================================
void CabbagePluginEditor::createEditorInterface (ValueTree widgets)
{
    components.clear();

    for (int widget = 0; widget < widgets.getNumChildren(); widget++)
    {
        const String widgetType = widgets.getChild (widget).getProperty (CabbageIdentifierIds::type).toString();

        if (widgetType == CabbageWidgetTypes::form)
            setupWindow (widgets.getChild (widget));
        else
        {
            insertWidget (widgets.getChild (widget));
        }
    }
    
    lookAndFeelChanged();
}

//======================================================================================================
void CabbagePluginEditor::setupWindow (ValueTree widgetData)
{
    instrumentName = CabbageWidgetData::getStringProp (widgetData, CabbageIdentifierIds::caption);
    setName (CabbageWidgetData::getStringProp (widgetData, CabbageIdentifierIds::caption));
    const int width = CabbageWidgetData::getNumProp (widgetData, CabbageIdentifierIds::width);
    const int height = CabbageWidgetData::getNumProp (widgetData, CabbageIdentifierIds::height);
    const String backgroundColourString = CabbageWidgetData::getStringProp (widgetData, CabbageIdentifierIds::colour);
    lookAndFeel.setDefaultFont(CabbageWidgetData::getStringProp (widgetData, CabbageIdentifierIds::typeface));

    backgroundColour = Colour::fromString (backgroundColourString);
    mainComponent.setColour (backgroundColour);
    setSize (width, height);
    repaint();
}
//======================================================================================================
void CabbagePluginEditor::addNewWidget (String widgetType, Point<int> position, bool isCustomPlant)
{

    if(isCustomPlant == false)
    {
        StringArray csdArray = processor.getCurrentCsdFileAsStringArray();
        const String widgetTreeIdentifier = "newlyAddedWidget";
        ValueTree newWidget(widgetTreeIdentifier);

        CabbageWidgetData::setWidgetState(newWidget, widgetType, newlyAddedWidgetIndex);
        CabbageWidgetData::setStringProp(newWidget, CabbageIdentifierIds::csdfile,
                                         processor.getCsdFile().getFullPathName());
        newWidget.setProperty(CabbageIdentifierIds::top, position.getY(), 0);
        newWidget.setProperty(CabbageIdentifierIds::left, position.getX(), 0);

        processor.cabbageWidgets.addChild(newWidget, -1, 0);

        setCurrentlySelectedComponents(
                StringArray(CabbageWidgetData::getStringProp(newWidget, CabbageIdentifierIds::name)));

        insertWidget(newWidget);
        updateLayoutEditorFrames();

        sendChangeMessage();    //update code in editor

        newlyAddedWidgetIndex++;
    }
    else
    {
        changeMessage = widgetType;
        customPlantPosition = position;
        sendChangeMessage();    //update code in editor
    }
}

//======================================================================================================
void CabbagePluginEditor::insertWidget (ValueTree cabbageWidgetData)
{
    const String widgetType = cabbageWidgetData.getProperty (CabbageIdentifierIds::type).toString();

    if (widgetType == CabbageWidgetTypes::checkbox)
        insertCheckbox (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::combobox)
        insertComboBox (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::image)
        insertImage (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::line)
        insertLine (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::rslider
             || widgetType == CabbageWidgetTypes::vslider
             || widgetType == CabbageWidgetTypes::hslider)
        insertSlider (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::label)
        insertLabel (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::groupbox)
        insertGroupBox (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::keyboard)
        insertMIDIKeyboard (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::csoundoutput)
        insertCsoundOutputConsole (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::nslider)
        insertNumberSlider (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::textbox)
        insertTextBox (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::texteditor)
        insertTextEditor (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::encoder)
        insertEncoder (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::soundfiler)
        insertSoundfiler (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::button)
        insertButton (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::filebutton)
        insertFileButton (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::infobutton)
        insertInfoButton (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::signaldisplay)
        insertSignalDisplay (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::gentable)
        insertGenTable (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::xypad)
        insertXYPad (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::eventsequencer)
        insertStringSequencer (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::hmeter || widgetType == CabbageWidgetTypes::vmeter)
        insertMeter (cabbageWidgetData);

    else if (widgetType == CabbageWidgetTypes::hrange
             || widgetType == CabbageWidgetTypes::vrange)
        insertRangeSlider (cabbageWidgetData);

}

void CabbagePluginEditor::insertCheckbox (ValueTree cabbageWidgetData)
{
    CabbageCheckbox* checkbox;
    components.add (checkbox = new CabbageCheckbox (cabbageWidgetData));
    checkbox->addListener (this);
    addToEditorAndMakeVisible (checkbox, cabbageWidgetData);
    addMouseListenerAndSetVisibility (checkbox, cabbageWidgetData);
}

void CabbagePluginEditor::insertComboBox (ValueTree cabbageWidgetData)
{
    CabbageComboBox* combobox;
    components.add (combobox = new CabbageComboBox (cabbageWidgetData, this));

    if (CabbageWidgetData::getStringProp (cabbageWidgetData, CabbageIdentifierIds::filetype).contains ("snaps")
        || CabbageWidgetData::getStringProp (cabbageWidgetData, CabbageIdentifierIds::channeltype).contains ("string"))
        combobox->addListener (combobox);
    else
        combobox->addListener (this);

    addToEditorAndMakeVisible (combobox, cabbageWidgetData);
    addMouseListenerAndSetVisibility (combobox, cabbageWidgetData);
}

void CabbagePluginEditor::insertRangeSlider (ValueTree cabbageWidgetData)
{
    CabbageRangeSlider* rangeSlider;
    components.add (rangeSlider = new CabbageRangeSlider (cabbageWidgetData, this));
    rangeSlider->getSlider().addListener (this);
    addToEditorAndMakeVisible (rangeSlider, cabbageWidgetData);
    addMouseListenerAndSetVisibility (rangeSlider, cabbageWidgetData);
}

void CabbagePluginEditor::insertLabel (ValueTree cabbageWidgetData)
{
    CabbageLabel* label;
    components.add (label = new CabbageLabel (cabbageWidgetData, this));
    addToEditorAndMakeVisible (label, cabbageWidgetData);
    addMouseListenerAndSetVisibility (label, cabbageWidgetData);
}

void CabbagePluginEditor::insertStringSequencer (ValueTree cabbageWidgetData)
{
    CabbageEventSequencer* stringSeq;
    components.add (stringSeq = new CabbageEventSequencer (cabbageWidgetData, this));
    addToEditorAndMakeVisible (stringSeq, cabbageWidgetData);
    addMouseListenerAndSetVisibility (stringSeq, cabbageWidgetData);
}

void CabbagePluginEditor::insertMeter (ValueTree cabbageWidgetData)
{
    CabbageMeter* meter;
    components.add (meter = new CabbageMeter (cabbageWidgetData, this));
    addToEditorAndMakeVisible (meter, cabbageWidgetData);
    addMouseListenerAndSetVisibility (meter, cabbageWidgetData);
}

void CabbagePluginEditor::insertTextEditor (ValueTree cabbageWidgetData)
{
    CabbageTextEditor* editor;
    components.add (editor = new CabbageTextEditor (cabbageWidgetData, this));
    addToEditorAndMakeVisible (editor, cabbageWidgetData);
    addMouseListenerAndSetVisibility (editor, cabbageWidgetData);
}

void CabbagePluginEditor::insertSoundfiler (ValueTree cabbageWidgetData)
{
    CabbageSoundfiler* soundfiler;
    components.add (soundfiler = new CabbageSoundfiler (cabbageWidgetData, this));
    addToEditorAndMakeVisible (soundfiler, cabbageWidgetData);
    addMouseListenerAndSetVisibility (soundfiler, cabbageWidgetData);
}

void CabbagePluginEditor::insertSlider (ValueTree cabbageWidgetData)
{
    CabbageSlider* slider;
    components.add (slider = new CabbageSlider (cabbageWidgetData, this));
    slider->getSlider().addListener (this);
    addToEditorAndMakeVisible (slider, cabbageWidgetData);
    addMouseListenerAndSetVisibility (slider, cabbageWidgetData);
}

void CabbagePluginEditor::insertEncoder (ValueTree cabbageWidgetData)
{
    CabbageEncoder* encoder;
    components.add (encoder = new CabbageEncoder (cabbageWidgetData, this));
    addToEditorAndMakeVisible (encoder, cabbageWidgetData);
    addMouseListenerAndSetVisibility (encoder, cabbageWidgetData);
}

void CabbagePluginEditor::insertXYPad (ValueTree cabbageWidgetData)
{
    CabbageXYPad* xyPad;
    components.add (xyPad = new CabbageXYPad (cabbageWidgetData, this));
    xyPad->getSliderX().addListener (this);
    xyPad->getSliderY().addListener (this);
    addToEditorAndMakeVisible (xyPad, cabbageWidgetData);
    processor.addXYAutomator (xyPad, cabbageWidgetData);
    addMouseListenerAndSetVisibility (xyPad, cabbageWidgetData);
}

void CabbagePluginEditor::insertFileButton (ValueTree cabbageWidgetData)
{
    CabbageFileButton* fileButton;
    components.add (fileButton = new CabbageFileButton (cabbageWidgetData, this));
    addToEditorAndMakeVisible (fileButton, cabbageWidgetData);
    addMouseListenerAndSetVisibility (fileButton, cabbageWidgetData);
}

void CabbagePluginEditor::insertSignalDisplay (ValueTree cabbageWidgetData)
{
    CabbageSignalDisplay* signalDisplay;
    components.add (signalDisplay = new CabbageSignalDisplay (cabbageWidgetData, this));
    addToEditorAndMakeVisible (signalDisplay, cabbageWidgetData);
    addMouseListenerAndSetVisibility (signalDisplay, cabbageWidgetData);
}

void CabbagePluginEditor::insertInfoButton (ValueTree cabbageWidgetData)
{
    CabbageInfoButton* infoButton;
    components.add (infoButton = new CabbageInfoButton (cabbageWidgetData));
    addToEditorAndMakeVisible (infoButton, cabbageWidgetData);
    addMouseListenerAndSetVisibility (infoButton, cabbageWidgetData);
}

void CabbagePluginEditor::insertButton (ValueTree cabbageWidgetData)
{
    CabbageButton* button;
    components.add (button = new CabbageButton (cabbageWidgetData));
    button->addListener (this);
    addToEditorAndMakeVisible (button, cabbageWidgetData);
    addMouseListenerAndSetVisibility (button, cabbageWidgetData);
}

void CabbagePluginEditor::insertNumberSlider (ValueTree cabbageWidgetData)
{
    CabbageNumberSlider* numberBox;
    components.add (numberBox = new CabbageNumberSlider (cabbageWidgetData));
    numberBox->getSlider().addListener (this);
    addToEditorAndMakeVisible (numberBox, cabbageWidgetData);
    addMouseListenerAndSetVisibility (numberBox, cabbageWidgetData);
}

void CabbagePluginEditor::insertGenTable (ValueTree cabbageWidgetData)
{
    CabbageGenTable* genTable;
    components.add (genTable = new CabbageGenTable (cabbageWidgetData, this));
    addToEditorAndMakeVisible (genTable, cabbageWidgetData);
    addMouseListenerAndSetVisibility (genTable, cabbageWidgetData);
}

void CabbagePluginEditor::insertTextBox (ValueTree cabbageWidgetData)
{
    CabbageTextBox* textBox;
    components.add (textBox = new CabbageTextBox (cabbageWidgetData));
    addToEditorAndMakeVisible (textBox, cabbageWidgetData);
    addMouseListenerAndSetVisibility (textBox, cabbageWidgetData);
}

void CabbagePluginEditor::insertCsoundOutputConsole (ValueTree cabbageWidgetData)
{
    if (consoleCount < 1)
    {
        CabbageCsoundConsole* csoundConsole;
        components.add (csoundConsole = new CabbageCsoundConsole (cabbageWidgetData, this));
        addToEditorAndMakeVisible (csoundConsole, cabbageWidgetData);
        addMouseListenerAndSetVisibility (csoundConsole, cabbageWidgetData);
        consoleCount++;
    }
}

void CabbagePluginEditor::insertMIDIKeyboard (ValueTree cabbageWidgetData)
{
    if (keyboardCount < 1)
    {
        CabbageKeyboard* midiKeyboard;
        components.add (midiKeyboard = new CabbageKeyboard (cabbageWidgetData, processor.keyboardState));
        midiKeyboard->setKeyPressBaseOctave (3);
        addToEditorAndMakeVisible (midiKeyboard, cabbageWidgetData);
        addMouseListenerAndSetVisibility (midiKeyboard, cabbageWidgetData);
        keyboardCount++;
    }
}

//======================================================================================================
void CabbagePluginEditor::insertGroupBox (ValueTree cabbageWidgetData)
{
    CabbageGroupBox* groupBox;
    components.add (groupBox = new CabbageGroupBox (cabbageWidgetData));
    addToEditorAndMakeVisible (groupBox, cabbageWidgetData);
    addMouseListenerAndSetVisibility (groupBox, cabbageWidgetData);
    addPlantToPopupPlantsArray (cabbageWidgetData, groupBox); // only groupboxes and images can be plants
}

void CabbagePluginEditor::insertImage (ValueTree cabbageWidgetData)
{
    CabbageImage* image;
    components.add (image = new CabbageImage (cabbageWidgetData, this));
    addToEditorAndMakeVisible (image, cabbageWidgetData);
    addMouseListenerAndSetVisibility (image, cabbageWidgetData);
    addPlantToPopupPlantsArray (cabbageWidgetData, image); // only groupboxes and images can be plants
}

void CabbagePluginEditor::insertLine (ValueTree cabbageWidgetData)
{
    CabbageImage* line;
    components.add (line = new CabbageImage (cabbageWidgetData, this, true));
    addToEditorAndMakeVisible (line, cabbageWidgetData);
    addMouseListenerAndSetVisibility (line, cabbageWidgetData);
}
//======================================================================================================
CabbageAudioParameter* CabbagePluginEditor::getParameterForComponent (const String name)
{
    for (auto param : processor.getParameters())
    {
        if (CabbageAudioParameter* cabbageParam = dynamic_cast<CabbageAudioParameter*> (param))
        {
            if (name == cabbageParam->getWidgetName())
                return dynamic_cast<CabbageAudioParameter*> (cabbageParam);
        }
    }

    return nullptr;
}

//======================================================================================================
void CabbagePluginEditor::comboBoxChanged (ComboBox* combo)
{
    if (CabbageAudioParameter* param = getParameterForComponent (combo->getName()))
    {
        param->beginChangeGesture();

        //preset combos work with 0 index, Cabbage combos start at 1..
        if (CabbageWidgetData::getStringProp (getValueTreeForComponent (combo->getName()), CabbageIdentifierIds::filetype).contains ("snaps"))
            param->setValueNotifyingHost (param->range.convertTo0to1 (combo->getSelectedItemIndex()));
        else
            param->setValueNotifyingHost (param->range.convertTo0to1 (combo->getSelectedItemIndex()+1));

        param->endChangeGesture();
    }
}
//======================================================================================================
void CabbagePluginEditor::buttonClicked (Button* button)
{
    const bool buttonState = button->getToggleState();

    if (CabbageButton* cabbageButton = dynamic_cast<CabbageButton*> (button))
    {
        const StringArray textItems = cabbageButton->getTextArray();
        CabbageUtilities::debug(cabbageButton->getName());
        const ValueTree valueTree = CabbageWidgetData::getValueTreeForComponent (processor.cabbageWidgets, cabbageButton->getName());
        const int latched = CabbageWidgetData::getNumProp (valueTree, CabbageIdentifierIds::latched);

        if (textItems.size() > 0)
            cabbageButton->setButtonText ( textItems[ buttonState == false ? 0 : 1]);

        if (latched == 1)
            toggleButtonState (button, buttonState);

        return;
    }

    toggleButtonState (button, buttonState);
}

void CabbagePluginEditor::buttonStateChanged (Button* button)
{
    if (CabbageButton* cabbageButton = dynamic_cast<CabbageButton*> (button))
    {
        const ValueTree valueTree = CabbageWidgetData::getValueTreeForComponent (processor.cabbageWidgets, cabbageButton->getName());
        const int latched = CabbageWidgetData::getNumProp (valueTree, CabbageIdentifierIds::latched);

        if (latched == 0)
        {
            if (button->isMouseButtonDown())
                toggleButtonState (button, true);
            else
                toggleButtonState (button, false);
        }
    }

}

void CabbagePluginEditor::toggleButtonState (Button* button, bool state)
{
    if (CabbageAudioParameter* param = getParameterForComponent (button->getName()))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (state == true ? 1 : 0);
        param->endChangeGesture();
    }
}
//======================================================================================================
void CabbagePluginEditor::sliderValueChanged (Slider* slider)
{
    if (slider->getSliderStyle() != Slider::TwoValueHorizontal && slider->getSliderStyle() != Slider::TwoValueVertical)
    {
        if (CabbageAudioParameter* param = getParameterForComponent (slider->getName()))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->range.convertTo0to1 (slider->getValue()));
            param->endChangeGesture();
        }
    }
    else
    {
        if (CabbageAudioParameter* param = getParameterForComponent (slider->getName() + "_min"))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->range.convertTo0to1 (slider->getMinValue()));
            param->endChangeGesture();
        }

        if (CabbageAudioParameter* param = getParameterForComponent (slider->getName() + "_max"))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->range.convertTo0to1 (slider->getMaxValue()));
            param->endChangeGesture();
        }
    }
}
//======================================================================================================
void CabbagePluginEditor::enableEditMode (bool enable)
{
#ifdef Cabbage_IDE_Build
    layoutEditor.setEnabled (enable);
    editModeEnabled = enable;
    layoutEditor.toFront (false);
#endif
}
//======================================================================================================
void CabbagePluginEditor::setCurrentlySelectedComponents (StringArray componentNames)
{
    currentlySelectedComponentNames = componentNames;
}

void CabbagePluginEditor::resetCurrentlySelectedComponents()
{
    currentlySelectedComponentNames.clear();
}

Component* CabbagePluginEditor::getComponentFromName (String name)
{
    for (auto comp : components)
    {
        if (name == comp->getName())
            return comp;
    }

    return nullptr;
}

Array<ValueTree> CabbagePluginEditor::getValueTreesForCurrentlySelectedComponents()
{
    Array<ValueTree> valueTreeArray;

    for (String compName : currentlySelectedComponentNames)
        valueTreeArray.add (CabbageWidgetData::getValueTreeForComponent (processor.cabbageWidgets, compName));

    return valueTreeArray;
}

ValueTree CabbagePluginEditor::getValueTreeForComponent (String compName)
{
    if(compName == "form")//special case
    {
        resetCurrentlySelectedComponents();
        return CabbageWidgetData::getValueTreeForComponent (processor.cabbageWidgets, "form");
    }
    else
        return CabbageWidgetData::getValueTreeForComponent (processor.cabbageWidgets, getComponentFromName (compName)->getName());
}

void CabbagePluginEditor::updateLayoutEditorFrames()
{
#ifdef Cabbage_IDE_Build

    if (editModeEnabled)
        layoutEditor.updateFrames();

#endif
}

//======================================================================================================
void CabbagePluginEditor::addToEditorAndMakeVisible (Component* comp, ValueTree widgetData)
{
    const String parent = CabbageWidgetData::getStringProp (widgetData, CabbageIdentifierIds::parentcomponent);

    if (auto parentComp = getComponentFromName (parent))
    {
        parentComp->addAndMakeVisible (comp);
    }
    else
        mainComponent.addAndMakeVisible (comp);
}

void CabbagePluginEditor::addMouseListenerAndSetVisibility (Component* comp, ValueTree wData)
{
    comp->addMouseListener (this, true);
    int visible = CabbageWidgetData::getNumProp (wData, CabbageIdentifierIds::visible);
    comp->setVisible (visible == 1 ? true : 0);
}

void CabbagePluginEditor::addPlantToPopupPlantsArray (ValueTree wData, Component* plant)
{
    const int isPopup = CabbageWidgetData::getNumProp (wData, CabbageIdentifierIds::popup);

    if (isPopup == 1)
    {
        const String caption = CabbageWidgetData::getStringProp (wData, CabbageIdentifierIds::text);
        const String name = CabbageWidgetData::getStringProp (wData, CabbageIdentifierIds::name);
        PopupDocumentWindow* popupPlant;
        popupPlants.add (popupPlant = new PopupDocumentWindow (caption, backgroundColour));
        popupPlant->setLookAndFeel (&getLookAndFeel());
        popupPlant->setWidgetData(wData);
        popupPlant->setContentNonOwned (plant, true);
        popupPlant->setName (name);
    }
}
//======================================================================================================
void CabbagePluginEditor::updatefTableData (GenTable* table)
{
    Array<double> pFields = table->getPfields();

    if ( table->genRoutine == 5 || table->genRoutine == 7 || table->genRoutine == 2)
    {
        FUNC* ftpp;
        EVTBLK  evt;
        memset (&evt, 0, sizeof (EVTBLK));
        evt.pcnt = 5 + pFields.size();
        evt.opcod = 'f';
        evt.p[0] = 0;

        //setting table number to 0.
        evt.p[1] = 0;
        evt.p[2] = 0;
        evt.p[3] = table->tableSize;
        evt.p[4] = table->realGenRoutine;

        if (table->genRoutine == 5)
        {
            for (int i = 0; i < pFields.size() - 1; i++)
                evt.p[5 + i] = jmax (0.00001, pFields[i + 1]);
        }
        else if (table->genRoutine == 7)
        {
            for (int i = 0; i < pFields.size() - 1; i++)
                evt.p[5 + i] = pFields[i + 1];
        }
        else
        {
            for (int i = 0; i < pFields.size(); i++)
                evt.p[5 + i] = pFields[i];
        }

        StringArray fStatement;
        int pCnt = 0;

        for (int i = 0; i < evt.pcnt - 1; i++)
        {
            fStatement.add (String (evt.p[i]));
            pCnt = i;
        }

        if (table->genRoutine != 2)
        {
            fStatement.add (String (1));
            fStatement.add (String (evt.p[pCnt]));
        }

        //now set table number and set score char to f
        fStatement.set (1, String (table->tableNumber));
        fStatement.set (0, "f");

        processor.getCsound()->GetCsound()->hfgens (processor.getCsound()->GetCsound(), &ftpp, &evt, 1);
        Array<float, CriticalSection> points;

        points = Array<float, CriticalSection> (ftpp->ftable, ftpp->flen);
        table->setWaveform (points, false);
        //table->enableEditMode(fStatement);

        processor.getCsound()->InputMessage (fStatement.joinIntoString (" ").toUTF8());
    }

}

//======================================================================================================
void CabbagePluginEditor::sendChannelDataToCsound (String channel, float value)
{
    if (csdCompiledWithoutError())
        processor.getCsound()->SetChannel (channel.getCharPointer(), value);
}

void CabbagePluginEditor::sendChannelStringDataToCsound (String channel, String value)
{
    if (processor.csdCompiledWithoutError())
        processor.getCsound()->SetChannel (channel.getCharPointer(), value.toUTF8().getAddress());
}

void CabbagePluginEditor::sendScoreEventToCsound (String scoreEvent)
{
    if (processor.csdCompiledWithoutError())
        processor.getCsound()->InputMessage(scoreEvent.toUTF8());
}

void CabbagePluginEditor::createEventMatrix(int cols, int rows, String channel)
{
    if (processor.csdCompiledWithoutError())
        processor.createMatrixEventSequencer(cols, rows, channel);
}

void CabbagePluginEditor::setEventMatrixData(int cols, int rows, String channel, String data)
{
    if (processor.csdCompiledWithoutError())
        processor.setMatrixEventSequencerCellData(cols, rows, channel, data);
}


const Array<float, CriticalSection> CabbagePluginEditor::getArrayForSignalDisplay (const String signalVariable, const String displayType)
{
    if (csdCompiledWithoutError())
        return processor.getSignalArray (signalVariable, displayType)->getPoints();

    return Array<float, CriticalSection>();
}

bool CabbagePluginEditor::shouldUpdateSignalDisplay()
{
    return processor.shouldUpdateSignalDisplay();
}

void CabbagePluginEditor::enableXYAutomator (String name, bool enable, Line<float> dragLine)
{
    processor.enableXYAutomator (name, enable, dragLine);
}

bool CabbagePluginEditor::csdCompiledWithoutError()
{
    return processor.csdCompiledWithoutError();
}

StringArray CabbagePluginEditor::getTableStatement (int tableNumber)
{
    if (csdCompiledWithoutError())
        return processor.getTableStatement (tableNumber);

    return StringArray();
}

const Array<float, CriticalSection> CabbagePluginEditor::getTableFloats (int tableNumber)
{
    if (csdCompiledWithoutError())
        return processor.getTableFloats (tableNumber);

    return Array<float, CriticalSection>();
}

CabbagePluginProcessor& CabbagePluginEditor::getProcessor()
{
    return processor;
}

void CabbagePluginEditor::savePluginStateToFile (File snapshotFile)
{
    XmlElement xml = processor.savePluginState (instrumentName.replace (" ", "_"), processor.getCsdFile().withFileExtension (".snaps"));
    xml.writeToFile (processor.getCsdFile().withFileExtension (".snaps"), "");
}

void CabbagePluginEditor::restorePluginStateFrom (String childPreset)
{
    ScopedPointer<XmlElement> xmlElement = XmlDocument::parse (processor.getCsdFile().withFileExtension (".snaps"));

    if (xmlElement->hasTagName ("CABBAGE_PRESETS"))
    {
        forEachXmlChildElement (*xmlElement, e)
        {
            if (e->getStringAttribute ("PresetName") == childPreset)
                processor.restorePluginState (e);
        }
    }

}

void CabbagePluginEditor::refreshComboBoxContents()
{
    for ( int i = 0 ; i < processor.cabbageWidgets.getNumChildren() ; i++)
    {
        const String type = CabbageWidgetData::getStringProp (processor.cabbageWidgets.getChild (i), CabbageIdentifierIds::type);

        if ( type == "combobox")
        {
            const String name = CabbageWidgetData::getStringProp (processor.cabbageWidgets.getChild (i), CabbageIdentifierIds::name);
            const String fileType = CabbageWidgetData::getProperty (processor.cabbageWidgets.getChild (i), CabbageIdentifierIds::filetype);


            if (CabbageComboBox* combo = dynamic_cast<CabbageComboBox*> (getComponentFromName (name)))
            {
                if (fileType.isNotEmpty())
                {
                    combo->addItemsToCombobox (processor.cabbageWidgets.getChild (i), true);
                }

                if(bool(combo->getProperties().getWithDefault("isPresetCombo", false)) == true)
                   combo->setSelectedItemIndex(combo->getNumItems()-1);
            }


        }
    }
}

String CabbagePluginEditor::createNewGenericNameForPresetFile()
{
    Array<File> dirFiles;
    File pluginDir = processor.getCsdFile().getParentDirectory().getFullPathName();
    pluginDir.findChildFiles (dirFiles, 2, false, "*.snaps");
    String newFileName;

    //now check existing files in directory and make sure we use a unique name
    for (int i = 0; i < dirFiles.size(); i++)
    {
        String newName = instrumentName + "_" + String (i + 1);

        newFileName  = pluginDir.getFullPathName() + "/" + newName + ".snaps";

        bool allowSave = true;

        for (auto file : dirFiles)
        {
            if (file.getFileNameWithoutExtension().equalsIgnoreCase (newName))
                allowSave = false;
        }

        if (allowSave)
            return newFileName;
    }

    const String firstPresetFile = instrumentName + "_0";

    if (SystemStats::getOperatingSystemType() == SystemStats::OperatingSystemType::Windows)
        return pluginDir.getFullPathName() + "\\" + firstPresetFile + ".snaps";
    else
        return pluginDir.getFullPathName() + "/" + firstPresetFile + ".snaps";

    return "";

}
//======================================================================================================
const String CabbagePluginEditor::getCsoundOutputFromProcessor()
{
    return processor.getCsoundOutput();
}
