/***************************************************************************
				generalsettings.cpp  -  description
				-------------------
	begin			: Sun Mar 4 2001
	copyright			: (C) 2001 by Otto Bruggeman
	email			: otto.bruggeman@home.nl
****************************************************************************/

/***************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
****************************************************************************/

#include "generalsettings.h"

#include "generalsettings.moc"

const QColor GeneralSettings::default_removeColor(190, 237, 190);
const QColor GeneralSettings::default_changeColor(237, 190, 190);
const QColor GeneralSettings::default_addColor(190, 190, 237);

GeneralSettings::GeneralSettings( QWidget* parent ) : SettingsBase( parent )
{

};

GeneralSettings::~GeneralSettings()
{

};

void GeneralSettings::loadSettings( KConfig* config )
{
	m_removeColor = config->readColorEntry( "RemoveColor", &default_removeColor );
	m_changeColor = config->readColorEntry( "ChangeColor", &default_changeColor );
	m_addColor = config->readColorEntry( "AddColor", &default_addColor );
	SettingsBase::loadSettings( config );
};

void GeneralSettings::saveSettings( KConfig* config )
{
	config->writeEntry( "RemoveColor", m_removeColor );
	config->writeEntry( "ChangeColor", m_changeColor );
	config->writeEntry( "AddColor", m_addColor );
	SettingsBase::saveSettings( config );
};