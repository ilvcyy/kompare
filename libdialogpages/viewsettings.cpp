/***************************************************************************
                                generalsettings.cpp  -  description
                                -------------------
        begin                   : Sun Mar 4 2001
        copyright               : (C) 2001-2003 by Otto Bruggeman
                                  and John Firebaugh
        email                   : otto.bruggeman@home.nl
                                  jfirebaugh@kde.org
****************************************************************************/

/***************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
***************************************************************************/

#include <qfont.h>

#include <kconfig.h>
#include <kglobalsettings.h>

#include "viewsettings.h"

using namespace Diff2;

const QColor ViewSettings::default_removeColor (190, 237, 190);
const QColor ViewSettings::default_changeColor (237, 190, 190);
const QColor ViewSettings::default_addColor    (190, 190, 237);
const QColor ViewSettings::default_appliedColor(237, 237, 190);

ViewSettings::ViewSettings( QWidget* parent )
	: SettingsBase( parent ),
	m_removeColor( 0, 0, 0 ),
	m_changeColor( 0, 0, 0),
	m_addColor( 0, 0, 0),
	m_appliedColor( 0, 0, 0),
	m_scrollNoOfLines( 0 ),
	m_tabToNumberOfSpaces( 0 ),
	m_fontSize( 0 )
{
}

ViewSettings::~ViewSettings()
{
}

void ViewSettings::loadSettings( KConfig* config )
{
	config->setGroup( "View Options" );
	m_removeColor         = config->readColorEntry( "RemoveColor",         &default_removeColor );
	m_changeColor         = config->readColorEntry( "ChangeColor",         &default_changeColor );
	m_addColor            = config->readColorEntry( "AddColor",            &default_addColor );
	m_appliedColor        = config->readColorEntry( "AppliedColor",        &default_appliedColor );
	m_scrollNoOfLines     = config->readNumEntry  ( "ScrollNoOfLines",     3 );
	m_tabToNumberOfSpaces = config->readNumEntry  ( "TabToNumberOfSpaces", 4 );

	m_font                = config->readFontEntry ( "TextFont" );
	m_fontSize            = config->readNumEntry  ( "TextFontSize", 10 );

	m_font.setPointSize( m_fontSize );

	SettingsBase::loadSettings( config );
}

void ViewSettings::saveSettings( KConfig* config )
{
	config->setGroup( "View Options" );
	config->writeEntry( "RemoveColor",         m_removeColor );
	config->writeEntry( "ChangeColor",         m_changeColor );
	config->writeEntry( "AddColor",            m_addColor );
	config->writeEntry( "AppliedColor",        m_appliedColor );
	config->writeEntry( "ScrollNoOfLines",     m_scrollNoOfLines );
	config->writeEntry( "TabToNumberOfSpaces", m_tabToNumberOfSpaces );

	config->writeEntry( "TextFont",            m_font );
	config->writeEntry( "TextFontSize",        m_fontSize );

	SettingsBase::saveSettings( config );
}

QColor ViewSettings::colorForDifferenceType( Difference::Type type, bool selected, bool applied )
{
	// FIXME: does not belong here
	QColor color;
	if( applied ) {
		color = m_appliedColor;
	} else {
		switch( type ) {
			case Difference::Unchanged: color = white; break;
			case Difference::Change:    color = m_changeColor; break;
			case Difference::Insert:    color = m_addColor; break;
			case Difference::Delete:    color = m_removeColor; break;
			default: break;
		}
	}

	if( selected ) {
		color = color.light( 105 );
	}

	return color;
}

#include "viewsettings.moc"
