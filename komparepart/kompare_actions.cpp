/***************************************************************************
                                kdiff_actions.cpp  -  description
                                -------------------
        begin                   : Sun Mar 4 2001
        copyright               : (C) 2001 by Otto Bruggeman
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

#include "kdiff_actions.h"

#include <qpopupmenu.h>
#include <qwidget.h>

#include <kdebug.h>

#include "difference.h"
#include "diffmodel.h"

KDifferencesAction::KDifferencesAction ( const QString & text, QObject* parent, const char* name ) :
	KAction( text, 0, parent, name )
{
	kdDebug() << "KDifferencesAction::KDifferencesAction" << endl;
	m_firstIndex = 0;
	m_differenceMenu = 0L;
}

int KDifferencesAction::plug( QWidget *widget, int index )
{
	kdDebug() << "KDifferencesAction::plug" << endl;
	// Go menu
	if ( widget->inherits("QPopupMenu") ) {
		m_differenceMenu = (QPopupMenu*)widget;
		// Forward signal (to main view)
		connect( m_differenceMenu, SIGNAL( aboutToShow() ),
		         this, SIGNAL( menuAboutToShow() ) );
		connect( m_differenceMenu, SIGNAL( activated( int ) ),
		         this, SLOT( slotActivated( int ) ) );

		// Store how many items the menu already contains.
		// This means, the KDifferencesAction has to be plugged LAST in a menu !
		m_firstIndex = m_differenceMenu->count();
		return m_differenceMenu->count(); // hmmm, what should this be ?
	}
	return KAction::plug( widget, index );
}

void KDifferencesAction::fillDifferenceMenu( const DiffModel* diffModel, int current )
{
	kdDebug() << "KDifferencesAction::fillDifferenceMenu" << endl;

	if ( m_firstIndex == 0 ) // should never happen since done in plug
		m_firstIndex = m_differenceMenu->count();
	else { // Clean up old differences (from the end, to avoid shifts)
		for ( uint i = m_differenceMenu->count()-1 ; i >= m_firstIndex; i-- )
			m_differenceMenu->removeItemAt( i );
	}

	QPtrListIterator<Difference> it = QPtrListIterator<Difference>(diffModel->getDifferences());
	int i = 0;
	while ( it.current() ) {
		int id = m_differenceMenu->insertItem( it.current()->asString() );
		if ( i == current )
			m_differenceMenu->setItemChecked( id, true );
		i++; ++it;
	}
}

void KDifferencesAction::slotActivated( int id )
{
	int index = m_differenceMenu->indexOf(id) - m_firstIndex;
	emit activated( index );
}

#include "kdiff_actions.moc"
