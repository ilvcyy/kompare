/***************************************************************************
                                generalsettings.h  -  description
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

#ifndef VIEWSETTINGS_H
#define VIEWSETTINGS_H

#include <qcolor.h>
#include <qwidget.h>

#include "difference.h"
#include "settingsbase.h"

class ViewSettings : public SettingsBase
{
Q_OBJECT
public:
	static const QColor default_removeColor;
	static const QColor default_changeColor;
	static const QColor default_addColor;
	static const QColor default_appliedColor;

	ViewSettings( QWidget* parent );
	~ViewSettings();
public:
	// some virtual functions that will be overloaded from the base class
	virtual void loadSettings( KConfig* config );
	virtual void saveSettings( KConfig* config );
	QColor colorForDifferenceType( Diff2::Difference::Type type, bool selected = false, bool applied = false );

public:
	QColor m_removeColor;
	QColor m_changeColor;
	QColor m_addColor;
	QColor m_appliedColor;
	QColor m_selectedRemoveColor;
	QColor m_selectedChangeColor;
	QColor m_selectedAddColor;
	QColor m_selectedAppliedColor;
	int    m_scrollNoOfLines;
	int    m_tabToNumberOfSpaces;

	QFont  m_font;
	int    m_fontSize;
};

#endif // VIEWSETTINGS_H