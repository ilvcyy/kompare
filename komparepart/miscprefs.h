/***************************************************************************
				miscprefs.h  -  description
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

#ifndef MISCPREFS_H
#define MISCPREFS_H

#include <qwidget.h>

#include "prefsbase.h"
#include "settingsbase.h"
#include "miscsettings.h"

class MiscPrefs : public PrefsBase
{
Q_OBJECT
public:
	MiscPrefs( QWidget* );
	~MiscPrefs();

public:
	MiscSettings* m_settings;

public:
	void setSettings( MiscSettings* );
	MiscSettings* getSettings( void );

public:
	virtual void restore();
	virtual void apply();
	virtual void setDefaults();
};

#endif