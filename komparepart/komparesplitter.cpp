/***************************************************************************
                                komparesplitter.cpp  -  description
                                -------------------
        begin                   : Wed Jan 14 2004
        copyright               : (C) 2004 by Jeff Snyder
        email                   : jeff@caffeinated.me.uk
****************************************************************************/

/***************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
***************************************************************************/

#include "komparesplitter.h"

#include <qstyle.h>
#include <qstring.h>
#include <qtimer.h>

#include <kdebug.h>
#include <kapplication.h>
#include <kglobalsettings.h>

#include "diffmodel.h"
#include "difference.h"
#include "viewsettings.h"
#include "kompare_part.h"
#include "komparelistview.h"
#include "kompareconnectwidget.h"

using namespace Diff2;

KompareSplitter::KompareSplitter( ViewSettings *settings, QWidget * parent,
                                  const char *name) :
	QSplitter( Horizontal, parent, name ),
	m_settings( settings )
{
	QFrame *scrollFrame = new QFrame( parent, "scrollFrame" );
	reparent( scrollFrame, *(new QPoint()), false );

	// Set up the scrollFrame
	scrollFrame->setFrameStyle( QFrame::NoFrame | QFrame::Plain );
	scrollFrame->setLineWidth(scrollFrame->style().pixelMetric(QStyle::PM_DefaultFrameWidth));
	QGridLayout *pairlayout = new QGridLayout(scrollFrame, 2, 2, 0);
	m_vScroll = new QScrollBar( QScrollBar::Vertical, scrollFrame );
	pairlayout->addWidget( m_vScroll, 0, 1 );
	m_hScroll = new QScrollBar( QScrollBar::Horizontal, scrollFrame );
	pairlayout->addWidget( m_hScroll, 1, 0 );

	new KompareListViewFrame(true, m_settings, this, "source");
	new KompareListViewFrame(false, m_settings, this, "destination");
	pairlayout->addWidget( this, 0, 0 );

	// set up our looks
	setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
	setLineWidth( style().pixelMetric( QStyle::PM_DefaultFrameWidth ) );

	setHandleWidth(50);
	setChildrenCollapsible( false );
	setFrameStyle( QFrame::NoFrame );
	setSizePolicy( QSizePolicy (QSizePolicy::Ignored, QSizePolicy::Ignored ));
	setOpaqueResize( true );

	connect( this, SIGNAL(configChanged()), SLOT(slotFontChanged()) );
	connect( this, SIGNAL(configChanged()), SLOT(slotDelayedRepaintHandles()) );
	connect( this, SIGNAL(configChanged()), SLOT(slotDelayedUpdateScrollBars()) );
	
	// scrolling
	connect( m_vScroll, SIGNAL(valueChanged(int)), SLOT(scrollToId(int)) );
	connect( m_vScroll, SIGNAL(sliderMoved(int)),  SLOT(scrollToId(int)) );
	connect( m_hScroll, SIGNAL(valueChanged(int)), SIGNAL(setXOffset(int)) );
	connect( m_hScroll, SIGNAL(sliderMoved(int)),  SIGNAL(setXOffset(int)) );

	m_scrollTimer=new QTimer();
	restartTimer = false;
	connect (m_scrollTimer, SIGNAL(timeout()), SLOT(timerTimeout()) );

	// init stuff
	slotUpdateScrollBars();
}

KompareSplitter::~KompareSplitter(){};

/*
    Inserts the widget \a w at the end (or at the beginning if \a
    prepend is TRUE) of the splitter's list of widgets.

    It is the responsibility of the caller to make sure that \a w is
    not already in the splitter and to call recalcId() if needed. (If
    \a prepend is TRUE, then recalcId() is very probably needed.)
*/

QSplitterLayoutStruct *KompareSplitter::addWidget( KompareListViewFrame *w, bool prepend )
{
	/* This function is *not* a good place to make connections to and from
	 * the KompareListView. Make them in the KompareListViewFrame constructor
	 * instad - that way the connections get made upon creation, not upon the
	 * next processing of the event queue. */

	QSplitterLayoutStruct *s;
	KompareConnectWidgetFrame *newHandle = 0;
	if ( d->list.count() > 0 ) {
		s = new QSplitterLayoutStruct;
		s->resizeMode = KeepSize;
		QString tmp = "qt_splithandle_";
		tmp += w->name();
		KompareListView *lw = 
			((KompareListViewFrame*)(prepend?w:d->list.last()->wid))->view();
		KompareListView *rw = 
			((KompareListViewFrame*)(prepend?d->list.first()->wid:w))->view();
		newHandle = new KompareConnectWidgetFrame(lw, rw, m_settings, this, tmp.latin1());
		s->wid = newHandle;
		newHandle->setId( d->list.count() );
		s->isHandle = TRUE;
		s->sizer = pick( newHandle->sizeHint() );
		if ( prepend )
		    d->list.prepend( s );
		else
		    d->list.append( s );
	}
	s = new QSplitterLayoutStruct;
	s->resizeMode = DefaultResizeMode;
	s->wid = w;
	s->isHandle = FALSE;
	if ( prepend ) d->list.prepend( s );
	else d->list.append( s );
	if ( newHandle && isVisible() )
		newHandle->show(); // will trigger sending of post events
	return s;
}


/*!
    Tells the splitter that the child widget described by \a c has
    been inserted or removed.
*/

void KompareSplitter::childEvent( QChildEvent *c )
{
    if ( c->type() == QEvent::ChildInserted ) {
		if ( !c->child()->isWidgetType() )
			return;

		if ( ((QWidget*)c->child())->testWFlags( WType_TopLevel ) )
			return;

		QSplitterLayoutStruct *s = d->list.first();
		while ( s ) {
			if ( s->wid == c->child() )
				return;
			s = d->list.next();
		}
		addWidget( (KompareListViewFrame*)c->child() );
		recalc( isVisible() );
    } else if ( c->type() == QEvent::ChildRemoved ) {
		QSplitterLayoutStruct *prev = 0;
		if ( d->list.count() > 1 )
			prev = d->list.at( 1 );  // yes, this is correct
		QSplitterLayoutStruct *curr = d->list.first();
		while ( curr ) {
			if ( curr->wid == c->child() ) {
			d->list.removeRef( curr );
			if ( prev && prev->isHandle ) {
				QWidget *w = prev->wid;
				d->list.removeRef( prev );
				delete w; // will call childEvent()
			}
			recalcId();
			doResize();
			return;
			}
			prev = curr;
			curr = d->list.next();
		}
	}
}

void KompareSplitter::slotSetSelection( const DiffModel* model, const Difference* diff )
{
	QSplitterLayoutStruct *curr;
	for( curr = d->list.first(); curr; curr = d->list.next() ) {
		if(curr->isHandle) {
			((KompareConnectWidgetFrame*)
				curr->wid)->wid()->slotSetSelection( model, diff );
		} else {
			((KompareListViewFrame*)
				curr->wid)->view()->slotSetSelection( model, diff );
			((KompareListViewFrame*)
				curr->wid)->slotSetModel( model );
		}
	}
	slotDelayedRepaintHandles();
	slotDelayedUpdateScrollBars();
}

void KompareSplitter::slotSetSelection( const Difference* diff )
{
	QSplitterLayoutStruct *curr;
	for( curr = d->list.first(); curr; curr = d->list.next() )
		if(curr->isHandle)
			((KompareConnectWidgetFrame*)
				curr->wid)->wid()->slotSetSelection( diff );
		else
			((KompareListViewFrame*)
				curr->wid)->view()->slotSetSelection( diff );
	slotDelayedRepaintHandles();
	slotDelayedUpdateScrollBars();
}

void KompareSplitter::slotApplyDifference( bool apply )
{
	QSplitterLayoutStruct *curr;
	for( curr = d->list.first(); curr; curr = d->list.next() )
		if(!curr->isHandle)
			((KompareListViewFrame*)
				curr->wid)->view()->slotApplyDifference( apply );
	slotDelayedRepaintHandles();
}

void KompareSplitter::slotApplyAllDifferences( bool apply )
{
	QSplitterLayoutStruct *curr;
	for( curr = d->list.first(); curr; curr = d->list.next() )
		if(!curr->isHandle)
			((KompareListViewFrame*)
				curr->wid)->view()->slotApplyAllDifferences( apply );
	slotDelayedRepaintHandles();
	scrollToId( scrollTo ); // FIXME!
}

void KompareSplitter::slotApplyDifference( const Difference* diff, bool apply )
{
	QSplitterLayoutStruct *curr;
	for( curr = d->list.first(); curr; curr = d->list.next() )
		if(!curr->isHandle)
			((KompareListViewFrame*)
				curr->wid)->view()->slotApplyDifference( diff, apply );
	slotDelayedRepaintHandles();
}

void KompareSplitter::slotDifferenceClicked( const Difference* diff )
{
	QSplitterLayoutStruct *curr;
	for( curr = d->list.first(); curr; curr = d->list.next() )
		if(!curr->isHandle)
			((KompareListViewFrame*)
				curr->wid)->view()->setSelectedDifference( diff, false );
	emit selectionChanged( diff );
}

void KompareSplitter::slotFontChanged()
{
	QSplitterLayoutStruct *curr;
	for( curr = d->list.first(); curr; curr = d->list.next() ) {
		if(!curr->isHandle) {
			((KompareListViewFrame*)
				curr->wid)->view()->setFont( KGlobalSettings::fixedFont() );
			((KompareListViewFrame*)
				curr->wid)->view()->update();
		}
	}
}

void KompareSplitter::slotDelayedRepaintHandles()
{
	QSplitterLayoutStruct *curr;
	for( curr = d->list.first(); curr; curr = d->list.next() )
		if(curr->isHandle)
			((KompareConnectWidgetFrame*)curr->wid)->wid()->slotDelayedRepaint();
}

void KompareSplitter::repaintHandles()
{
	QSplitterLayoutStruct *curr;
	for( curr = d->list.first(); curr; curr = d->list.next() )
		if(curr->isHandle)
			((KompareConnectWidgetFrame*)curr->wid)->wid()->repaint();
}

// Scrolling stuff
/*
 * limit updating to once every 50 msec with a qtimer
 * FIXME: i'm a nasty hack
 */

void KompareSplitter::wheelEvent( QWheelEvent* e )
{
	// scroll lines...
	if ( e->delta() < 0 ) // scroll back into file
	{
		m_vScroll->addLine();
	}
	else // scroll forward into file
	{
		m_vScroll->subtractLine();
	}
	repaintHandles();
}

void KompareSplitter::timerTimeout()
{
	if(restartTimer) restartTimer = false;
	else m_scrollTimer->stop();
	slotDelayedRepaintHandles();	
	emit scrollViewsToId( scrollTo );
}

void KompareSplitter::scrollToId( int id )
{
	scrollTo = id;
	if(restartTimer) {}
	else if( m_scrollTimer->isActive() ) {
		restartTimer = true;
	} else {
		emit scrollViewsToId( id );
		slotDelayedRepaintHandles();
		m_scrollTimer->start(30, false);
	}
}

void KompareSplitter::slotDelayedUpdateScrollBars()
{
	QTimer::singleShot( 0, this, SLOT( slotUpdateScrollBars() ) );
}

void KompareSplitter::slotUpdateScrollBars()
{
	int m_scrollDistance = m_settings->m_scrollNoOfLines * lineSpacing();
	int m_pageSize = pageSize();

	if( needVScrollBar() ) {
		m_vScroll->show();

		m_vScroll->blockSignals( true );
		m_vScroll->setRange( minVScrollId(), 
		                     maxVScrollId() );
		m_vScroll->setValue( scrollId() );
		m_vScroll->setSteps( m_scrollDistance, m_pageSize );
		m_vScroll->blockSignals( false );
	} else {
		m_vScroll->hide();
	}

	if( needHScrollBar() ) {
		m_hScroll->show();
		m_hScroll->blockSignals( true );
		m_hScroll->setRange( 0, maxHScrollId() );
		m_hScroll->setValue( maxContentsX() );
		m_hScroll->setSteps( 10, minVisibleWidth() - 10 );
		m_hScroll->blockSignals( false );
	} else {
		m_hScroll->hide();
	}
}

void KompareSplitter::slotDelayedUpdateVScrollValue()
{
	QTimer::singleShot( 0, this, SLOT( slotUpdateVScrollValue() ) );
}

void KompareSplitter::slotUpdateVScrollValue()
{
	m_vScroll->setValue( scrollId() );
}

/* FIXME: this should return the scrollId() from the listview containing the
 * /base/ of the diff. but there's bigger issues with that atm.
 */
 
int KompareSplitter::scrollId()
{
	QSplitterLayoutStruct *curr = d->list.first();
	for( curr = d->list.first(); curr; curr = d->list.next() )
		if(!curr->isHandle)
			return ((KompareListViewFrame*) curr->wid)->view()->scrollId();
	return minVScrollId();
}

int KompareSplitter::lineSpacing()
{
	QSplitterLayoutStruct *curr = d->list.first();
	for( curr = d->list.first(); curr; curr = d->list.next() )
		if(!curr->isHandle)
			return ((KompareListViewFrame*)
				curr->wid)->view()->fontMetrics().lineSpacing();
	return 1;
}

int KompareSplitter::pageSize()
{
	QSplitterLayoutStruct *curr;
	for( curr = d->list.first(); curr; curr = d->list.next() )  {
		if(!curr->isHandle) {
			KompareListView *view = ((KompareListViewFrame*) curr->wid)->view();
			return view->visibleHeight() - QStyle::PM_ScrollBarExtent;
		}
	}
	return 1;
}

bool KompareSplitter::needVScrollBar()
{
	QSplitterLayoutStruct *curr;
	int pagesize = pageSize();
	for( curr = d->list.first(); curr; curr = d->list.next() ) {
		if(!curr->isHandle) {
			KompareListView *view = ((KompareListViewFrame*) curr->wid)->view();
			if( view ->contentsHeight() > pagesize) {
				return true;
			}
		}
	}
	return false;
}

int KompareSplitter::minVScrollId()
{

	QSplitterLayoutStruct *curr;
	int min = -1;
	int mSId;
	for( curr = d->list.first(); curr; curr = d->list.next() ) {
		if(!curr->isHandle) {
			KompareListView* view = ((KompareListViewFrame*)curr->wid)->view();
			mSId = view->minScrollId();
			if (mSId < min || min == -1) min = mSId;
		}
	}
	return (min==-1)?0:min;
}

int KompareSplitter::maxVScrollId()
{
	QSplitterLayoutStruct *curr;
	int max = 0;
	int mSId;
	for( curr = d->list.first(); curr; curr = d->list.next() ) {
		if(!curr->isHandle) {
			mSId = ((KompareListViewFrame*)curr->wid)->view()->maxScrollId();
			if (mSId > max) max = mSId;
		}
	}
	return max;
}

bool KompareSplitter::needHScrollBar()
{
	QSplitterLayoutStruct *curr;
	for( curr = d->list.first(); curr; curr = d->list.next() ) {
		if(!curr->isHandle) {
			KompareListView *view = ((KompareListViewFrame*) curr->wid)->view();
			if( view->contentsWidth() > view->visibleWidth() ) return true;
		}
	}
	return false;
}

int KompareSplitter::maxHScrollId()
{
	QSplitterLayoutStruct *curr;
	int max = 0;
	int mHSId;
	for( curr = d->list.first(); curr; curr = d->list.next() ) {
		if(!curr->isHandle) {
			KompareListView *view = ((KompareListViewFrame*) curr->wid)->view();
			mHSId = view->contentsWidth() - view->visibleWidth();
			if( mHSId > max ) max = mHSId;
		}
	}
	return max;
}

int KompareSplitter::maxContentsX()
{
	QSplitterLayoutStruct *curr;
	int max = 0;
	int mCX;
	for( curr = d->list.first(); curr; curr = d->list.next() ) {
		if(!curr->isHandle) {
			mCX = ((KompareListViewFrame*) curr->wid)->view()->contentsX();
			if( mCX > max ) max = mCX;
		}
	}
	return max;
}

int KompareSplitter::minVisibleWidth()
{
	// Why the hell do we want to know this?
	// ah yes, its because we use it to set the "page size" for horiz. scrolling.
	// despite the fact that *noone* has a pgright and pgleft key :P
	QSplitterLayoutStruct *curr;
	int min = -1;
	int vW;
	for( curr = d->list.first(); curr; curr = d->list.next() ) {
		if(!curr->isHandle) {
			vW = ((KompareListViewFrame*)curr->wid)->view()->visibleWidth();
			if (vW < min || min == -1) min = vW;
		}
	}
	return (min==-1)?0:min;
}

#include "komparesplitter.moc"
