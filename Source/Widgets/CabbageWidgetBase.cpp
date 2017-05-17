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

#include "CabbageWidgetBase.h"
#include "../CabbageCommonHeaders.h"
#include "../Audio/Plugins/CabbagePluginEditor.h"


void CabbageWidgetBase::initialiseCommonAttributes (Component* child, ValueTree data)
{
    csdFile = CabbageWidgetData::getStringProp (data, CabbageIdentifierIds::csdfile);
    rotate = CabbageWidgetData::getNumProp (data, CabbageIdentifierIds::rotate);
    pivotx = CabbageWidgetData::getNumProp (data, CabbageIdentifierIds::pivotx);
    pivoty = CabbageWidgetData::getNumProp (data, CabbageIdentifierIds::pivoty);
    visible = CabbageWidgetData::getNumProp (data, CabbageIdentifierIds::visible);
    active = CabbageWidgetData::getNumProp (data, CabbageIdentifierIds::active);
    channel = CabbageWidgetData::getStringProp (data, CabbageIdentifierIds::channel);
    file = CabbageWidgetData::getStringProp (data, CabbageIdentifierIds::file);
    tooltipText = CabbageWidgetData::getStringProp (data, CabbageIdentifierIds::popuptext);
    child->setBounds (CabbageWidgetData::getBounds (data));
    child->setName (CabbageWidgetData::getStringProp (data, CabbageIdentifierIds::name));
    text = CabbageWidgetData::getStringProp (data, CabbageIdentifierIds::text);
    currentValue = CabbageWidgetData::getNumProp (data, CabbageIdentifierIds::value);
    value = currentValue;
    valuex = CabbageWidgetData::getNumProp (data, CabbageIdentifierIds::valuex);
    valuey = CabbageWidgetData::getNumProp (data, CabbageIdentifierIds::valuey);
    lineNumber = CabbageWidgetData::getNumProp (data, CabbageIdentifierIds::linenumber);
    child->getProperties().set (CabbageIdentifierIds::linenumber, lineNumber);
    populateTextArrays (data);
    //now initialise everything that can be updated using ident channels
    handleCommonUpdates (child, data, true);
}

void CabbageWidgetBase::handleCommonUpdates (Component* child, ValueTree data, bool calledFromConstructor)
{
    if (calledFromConstructor == false)
    {
        if (getPluginEditor (child) != nullptr && getPluginEditor (child)->isEditModeEnabled() == false)
            child->setBounds (CabbageWidgetData::getBounds (data));

        else if (CabbageWidgetData::getNumProp (data, CabbageIdentifierIds::allowboundsupdate) == 1)
        {
            child->setBounds (CabbageWidgetData::getBounds (data));
            getPluginEditor (child)->updateLayoutEditorFrames();
        }
    }

    if ( rotate != CabbageWidgetData::getNumProp (data, CabbageIdentifierIds::rotate) || calledFromConstructor)
    {
        rotate = CabbageWidgetData::getNumProp (data, CabbageIdentifierIds::rotate);
        child->setTransform (AffineTransform::rotation ( rotate, child->getX() + CabbageWidgetData::getNumProp (data,
                                                         CabbageIdentifierIds::pivotx),
                                                         child->getY() + CabbageWidgetData::getNumProp (data, CabbageIdentifierIds::pivoty)));
    }

    if ( visible != CabbageWidgetData::getNumProp (data, CabbageIdentifierIds::visible) || calledFromConstructor)
    {
        visible = CabbageWidgetData::getNumProp (data, CabbageIdentifierIds::visible);
        child->setVisible ( visible == 1 ? true : false);
        child->setEnabled ( visible == 1 ? true : false);
    }

    if (text != CabbageWidgetData::getStringProp (data, CabbageIdentifierIds::text) || calledFromConstructor)
    {
        text = CabbageWidgetData::getStringProp (data, CabbageIdentifierIds::text);
    }

    if ( active != CabbageWidgetData::getNumProp (data, CabbageIdentifierIds::active) || calledFromConstructor)
    {
        active = CabbageWidgetData::getNumProp (data, CabbageIdentifierIds::active);
        child->setEnabled ( active == 1 ? true : false);
    }

    if ( alpha != CabbageWidgetData::getNumProp (data, CabbageIdentifierIds::alpha) || calledFromConstructor)
    {
        alpha = CabbageWidgetData::getNumProp (data, CabbageIdentifierIds::alpha);
        child->setAlpha ( alpha);
    }

    if ( file != CabbageWidgetData::getStringProp (data, CabbageIdentifierIds::file) || calledFromConstructor)
    {
        file = CabbageWidgetData::getStringProp (data, CabbageIdentifierIds::file);
    }

    populateTextArrays (data);
}

String CabbageWidgetBase::getCurrentText (ValueTree data)
{
    if ( text != CabbageWidgetData::getStringProp (data, CabbageIdentifierIds::text))
    {
        text = CabbageWidgetData::getStringProp (data, CabbageIdentifierIds::text);
        return  text;
    }

    return  text;
}

String CabbageWidgetBase::getCurrentPopupText (ValueTree data)
{
    if ( tooltipText != CabbageWidgetData::getStringProp (data, CabbageIdentifierIds::popuptext))
    {
        tooltipText = CabbageWidgetData::getStringProp (data, CabbageIdentifierIds::popuptext);
        return  tooltipText;
    }

    return  tooltipText;
}

void CabbageWidgetBase::setChannel (ValueTree data)
{
    if ( channel != CabbageWidgetData::getStringProp (data, CabbageIdentifierIds::channel))
    {
        channel = CabbageWidgetData::getStringProp (data, CabbageIdentifierIds::channel);
        CabbageWidgetData::setProperty (data, CabbageIdentifierIds::channel,  channel);
    }
}

float CabbageWidgetBase::getCurrentValue (ValueTree data)
{
    if ( currentValue != CabbageWidgetData::getNumProp (data, CabbageIdentifierIds::value))
    {
        currentValue = CabbageWidgetData::getNumProp (data, CabbageIdentifierIds::value);
        return  currentValue;
    }

    return  currentValue;
}

void CabbageWidgetBase::populateTextArrays (ValueTree data)
{
    channelArray.clear();
    textArray.clear();
    const Array<var>* channelArrayVar = CabbageWidgetData::getProperty (data, CabbageIdentifierIds::channel).getArray();

    if (channelArrayVar && channelArrayVar->size() > 1)
    {
        for (int i = 0 ; i < channelArrayVar->size() ; i++ )
            channelArray.add (channelArrayVar->getReference (i).toString());
    }
    else
        channelArray.add (CabbageWidgetData::getStringProp (data, CabbageIdentifierIds::channel));  //make sure we have at least two items in array

    channelArray.add (CabbageWidgetData::getStringProp (data, CabbageIdentifierIds::channel));

    const Array<var>* textArrayVar = CabbageWidgetData::getProperty (data, CabbageIdentifierIds::text).getArray();

    if (textArrayVar && textArrayVar->size() > 1)
    {
        for (int i = 0 ; i < textArrayVar->size() ; i++ )
        {
            textArray.add (textArrayVar->getReference (i).toString());
        }
    }
    else
        textArray.add (CabbageWidgetData::getStringProp (data, CabbageIdentifierIds::text));  //make sure we have at least two items in array

    textArray.add (CabbageWidgetData::getStringProp (data, CabbageIdentifierIds::text));
}

int CabbageWidgetBase::getSVGWidth (File svgFile)
{
    ScopedPointer<XmlElement> svg (XmlDocument::parse (svgFile.loadFileAsString()));

    for (int i = 0; i < svg->getNumAttributes(); i++)
    {
        if (svg->getAttributeName (i) == "width")
            return svg->getAttributeValue (i).getIntValue();
    }

    return 0;
}

int CabbageWidgetBase::getSVGHeight (File svgFile)
{
    ScopedPointer<XmlElement> svg (XmlDocument::parse (svgFile.loadFileAsString()));

    for (int i = 0; i < svg->getNumAttributes(); i++)
    {
        if (svg->getAttributeName (i) == "height")
            return svg->getAttributeValue (i).getIntValue();
    }

    return 0;
}