/***************************************************************************
                                KompareNavTreePart.cpp
                                ----------------------
        begin                   : Sun Mar 4 2001
        Copyright 2001-2005,2009 Otto Bruggeman <bruggie@gmail.com>
        Copyright 2001-2003 John Firebaugh <jfirebaugh@kde.org>
        Copyright 2007      Kevin Kofler   <kevin.kofler@chello.at>
****************************************************************************/

/***************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
***************************************************************************/

#include "komparenavtreepart.h"

#include <kdebug.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kmimetype.h>
#include <k3listview.h>
#include <kaboutdata.h>
#include <kcomponentdata.h>

#include "difference.h"
#include "diffmodel.h"
#include "diffmodellist.h"
#include "komparemodellist.h"

#define COL_SOURCE        0
#define COL_DESTINATION   1
#define COL_DIFFERENCE    2

using namespace Diff2;

KompareNavTreePart::KompareNavTreePart( QWidget* parent, const char* name )
	: KParts::ReadOnlyPart( parent ),
	m_splitter( 0 ),
	m_modelList( 0 ),
	m_srcDirTree( 0 ),
	m_destDirTree( 0 ),
	m_fileList( 0 ),
	m_changesList( 0 ),
	m_srcRootItem( 0 ),
	m_destRootItem( 0 ),
	m_selectedModel( 0 ),
	m_selectedDifference( 0 ),
	m_source( "" ),
	m_destination( "" ),
	m_info( 0 )
{
	setObjectName( name );
	m_splitter = new QSplitter( Qt::Horizontal );

	setWidget( m_splitter );

	m_srcDirTree = new K3ListView( m_splitter );
	m_srcDirTree->addColumn( i18n("Source Folder") );
	m_srcDirTree->setRootIsDecorated( false );
	m_srcDirTree->setSorting( 0, true );

	m_destDirTree = new K3ListView( m_splitter );
	m_destDirTree->addColumn( i18n("Destination Folder") );
	m_destDirTree->setRootIsDecorated( false );
	m_destDirTree->setSorting( 0, true );

	m_fileList = new K3ListView( m_splitter );
	m_fileList->addColumn( i18n("Source File") );
	m_fileList->addColumn( i18n("Destination File") );
	m_fileList->setAllColumnsShowFocus( true );
	m_fileList->setRootIsDecorated( false );
	m_fileList->setSorting( 0, true );

	m_changesList = new K3ListView( m_splitter );
	m_changesList->addColumn( i18n("Source Line") );
	m_changesList->addColumn( i18n("Destination Line") );
	m_changesList->addColumn( i18n("Difference") );
	m_changesList->setAllColumnsShowFocus( true );
	m_changesList->setRootIsDecorated( false );
	m_changesList->setSorting( 0, true );

	connect( m_srcDirTree, SIGNAL(selectionChanged( Q3ListViewItem* )),
	         this, SLOT(slotSrcDirTreeSelectionChanged( Q3ListViewItem* )) );
	connect( m_destDirTree, SIGNAL(selectionChanged( Q3ListViewItem* )),
	         this, SLOT(slotDestDirTreeSelectionChanged( Q3ListViewItem* )) );
	connect( m_fileList, SIGNAL(selectionChanged( Q3ListViewItem* )),
	         this, SLOT(slotFileListSelectionChanged( Q3ListViewItem* )) );
	connect( m_changesList, SIGNAL(selectionChanged( Q3ListViewItem* )),
	         this, SLOT(slotChangesListSelectionChanged( Q3ListViewItem* )) );
}

KompareNavTreePart::~KompareNavTreePart()
{
	m_modelList = 0;
	m_selectedModel = 0;
	m_selectedDifference = 0;
}

void KompareNavTreePart::slotKompareInfo( struct Kompare::Info* info )
{
	m_info = info;
}

void KompareNavTreePart::slotModelsChanged( const DiffModelList* modelList )
{
	kDebug(8105) << "Models (" << modelList << ") have changed... scanning the models... " << endl;

	if ( modelList )
	{
		m_modelList = modelList;
		m_srcDirTree->clear();
		m_destDirTree->clear();
		m_fileList->clear();
		m_changesList->clear();
		buildTreeInMemory();
	}
	else
	{
		m_modelList = modelList;
		m_srcDirTree->clear();
		m_destDirTree->clear();
		m_fileList->clear();
		m_changesList->clear();
	}
}

void KompareNavTreePart::buildTreeInMemory()
{
	kDebug(8105) << "BuildTreeInMemory called" << endl;

	if ( m_modelList->count() == 0 )
	{
		kDebug(8105) << "No models... weird shit..." << endl;
		return; // avoids a crash on clear()
	}

	if ( m_info == 0 )
	{
		kDebug(8105) << "No Info... weird shit..." << endl;
		return;
	}

	QString srcBase;
	QString destBase;

	DiffModel* model;
	model = m_modelList->first();
	m_selectedModel = 0L;

	switch ( m_info->mode )
	{
	case Kompare::ShowingDiff:
		// BUG: 107489 No common root because it is a multi directory relative path diff
		// We need to detect this and create a different rootitem / or so or should we always add this?
		// Trouble we run into is that the directories do not start with a /
		// so we have an unknown top root dir
		// Thinking some more about it i guess it is best to use "" as base and simply show some string
		// like Unknown filesystem path as root text but only in the case of dirs starting without a /
		srcBase = model->sourcePath();
		destBase = model->destinationPath();
		// FIXME: these tests will not work on windows, we need something else
		if ( srcBase[0] != '/' )
			srcBase = "";
		if ( destBase[0] != '/' )
			destBase = "";
		break;
	case Kompare::ComparingFiles:
		srcBase  = model->sourcePath();
		destBase = model->destinationPath();
		break;
	case Kompare::ComparingDirs:
		srcBase = m_info->localSource;
		if ( !srcBase.endsWith( '/' ) )
			srcBase += '/';
		destBase = m_info->localDestination;
		if ( !destBase.endsWith( '/' ) )
			destBase += '/';
		break;
	case Kompare::BlendingFile:
	case Kompare::BlendingDir:
	default:
		kDebug(8105) << "Oops needs to implement this..." << endl;
	}

//	kDebug(8105) << "srcBase  = " << srcBase << endl;
//	kDebug(8105) << "destBase = " << destBase << endl;

	m_srcRootItem  = new KDirLVI( m_srcDirTree, srcBase );
	m_destRootItem = new KDirLVI( m_destDirTree, destBase );

	QString srcPath;
	QString destPath;

	// Create the tree from the models
	DiffModelListConstIterator modelIt = m_modelList->begin();
	DiffModelListConstIterator mEnd    = m_modelList->end();

	for ( ; modelIt != mEnd; ++modelIt )
	{
		model = *modelIt;
		srcPath  = model->sourcePath();
		destPath = model->destinationPath();

		kDebug(8105) << "srcPath  = " << srcPath  << endl;
		kDebug(8105) << "destPath = " << destPath << endl;
		m_srcRootItem->addModel( srcPath, model, &m_modelToSrcDirItemDict );
		m_destRootItem->addModel( destPath, model, &m_modelToDestDirItemDict );
	}
//	m_srcDirTree->setSelected( m_srcDirTree->firstChild(), true );
}

void KompareNavTreePart::buildDirectoryTree()
{
// FIXME: afaict this can be deleted
//	kDebug(8105) << "BuildDirTree called" << endl;
}

QString KompareNavTreePart::compareFromEndAndReturnSame(
    const QString& string1,
    const QString& string2 )
{
	QString result;

	int srcLen = string1.length();
	int destLen = string2.length();

	while ( srcLen != 0 && destLen != 0 )
	{
		if ( string1[--srcLen] == string2[--destLen] )
			result.prepend( string1[srcLen] );
		else
			break;
	}

	if ( srcLen != 0 && destLen != 0 && result.startsWith( '/' ) )
		result = result.remove( 0, 1 ); // strip leading /, we need it later

	return result;
}

void KompareNavTreePart::slotSetSelection( const DiffModel* model, const Difference* diff )
{
	kDebug(8105) << "KompareNavTreePart::slotSetSelection model = " << model << ", diff = " << diff << endl;
	if ( model == m_selectedModel )
	{
		// model is the same, so no need to update that...
		if ( diff != m_selectedDifference )
		{
			m_selectedDifference = diff;
			setSelectedDifference( diff );
		}
		return;
	}

	// model is different so we need to find the right dirs, file and changeitems to select
	// if m_selectedModel == NULL then everything needs to be done as well
	if ( !m_selectedModel || model->sourcePath() != m_selectedModel->sourcePath() )
	{   // dirs are different, so we need to update the dirviews as well
		m_selectedModel = model;
		m_selectedDifference = diff;

		setSelectedDir( model );
		setSelectedFile( model );
		setSelectedDifference( diff );
		return;
	}

	if ( !m_selectedModel || model->sourceFile() != m_selectedModel->sourceFile() )
	{
		m_selectedModel = model;
		setSelectedFile( model );

		m_selectedDifference = diff;
		setSelectedDifference( diff );
	}
}

void KompareNavTreePart::setSelectedDir( const DiffModel* model )
{
	KDirLVI* currentDir;
	currentDir = m_modelToSrcDirItemDict[ model ];
	kDebug(8105) << "Manually setting selection in srcdirtree with currentDir = " << currentDir << endl;
	m_srcDirTree->blockSignals( true );
	m_srcDirTree->setSelected( currentDir, true );
	m_srcDirTree->ensureItemVisible( currentDir );
	m_srcDirTree->blockSignals( false );

	currentDir = m_modelToDestDirItemDict[ model ];
	kDebug(8105) << "Manually setting selection in destdirtree with currentDir = " << currentDir << endl;
	m_destDirTree->blockSignals( true );
	m_destDirTree->setSelected( currentDir, true );
	m_destDirTree->ensureItemVisible( currentDir );
	m_destDirTree->blockSignals( false );

	m_fileList->blockSignals( true );
	currentDir->fillFileList( m_fileList, &m_modelToFileItemDict );
	m_fileList->blockSignals( false );
}

void KompareNavTreePart::setSelectedFile( const DiffModel* model )
{
	KFileLVI* currentFile;
	currentFile = m_modelToFileItemDict[ model ];
	kDebug(8105) << "Manually setting selection in filelist" << endl;
	m_fileList->blockSignals( true );
	m_fileList->setSelected( currentFile, true );
	m_fileList->ensureItemVisible( currentFile );
	m_fileList->blockSignals( false );

	m_changesList->blockSignals( true );
	currentFile->fillChangesList( m_changesList, &m_diffToChangeItemDict );
	m_changesList->blockSignals( false );
}

void KompareNavTreePart::setSelectedDifference( const Difference* diff )
{
	KChangeLVI* currentDiff;
	currentDiff = m_diffToChangeItemDict[ diff ];
	kDebug(8105) << "Manually setting selection in changeslist to " << currentDiff << endl;
	m_changesList->blockSignals( true );
	m_changesList->setSelected( currentDiff, true );
	m_changesList->ensureItemVisible( currentDiff );
	m_changesList->blockSignals( false );
}

void KompareNavTreePart::slotSetSelection( const Difference* diff )
{
//	kDebug(8105) << "Scotty i need more power !!" << endl;
	if ( m_selectedDifference != diff )
	{
//		kDebug(8105) << "But sir, i am giving you all she's got" << endl;
		m_selectedDifference = diff;
		setSelectedDifference( diff );
	}
}

void KompareNavTreePart::slotSrcDirTreeSelectionChanged( Q3ListViewItem* item )
{
	kDebug(8105) << "Sent by the sourceDirectoryTree with item = " << item << endl;
	m_srcDirTree->ensureItemVisible( item );
	KDirLVI* dir = static_cast<KDirLVI*>(item);
	// order the dest tree view to set its selected item to the same as here
	QString path;
	// We start with an empty path and after the call path contains the full path
	path = dir->fullPath( path );
	KDirLVI* selItem = m_destRootItem->setSelected( path );
	m_destDirTree->blockSignals( true );
	m_destDirTree->setSelected( selItem, true );
	m_destDirTree->ensureItemVisible( selItem );
	m_destDirTree->blockSignals( false );
	// fill the changes list
	dir->fillFileList( m_fileList, &m_modelToFileItemDict );
}

void KompareNavTreePart::slotDestDirTreeSelectionChanged( Q3ListViewItem* item )
{
	kDebug(8105) << "Sent by the destinationDirectoryTree with item = " << item << endl;
	m_destDirTree->ensureItemVisible( item );
	KDirLVI* dir = static_cast<KDirLVI*>(item);
	// order the src tree view to set its selected item to the same as here
	QString path;
	// We start with an empty path and after the call path contains the full path
	path = dir->fullPath( path );
	KDirLVI* selItem = m_srcRootItem->setSelected( path );
	m_srcDirTree->blockSignals( true );
	m_srcDirTree->setSelected( selItem, true );
	m_srcDirTree->ensureItemVisible( selItem );
	m_srcDirTree->blockSignals( false );
	// fill the changes list
	dir->fillFileList( m_fileList, &m_modelToFileItemDict );
}

void KompareNavTreePart::slotFileListSelectionChanged( Q3ListViewItem* item )
{
	kDebug(8105) << "Sent by the fileList with item = " << item << endl;

	KFileLVI* file = static_cast<KFileLVI*>(item);
	m_selectedModel = file->model();
	m_changesList->blockSignals( true );
	file->fillChangesList( m_changesList, &m_diffToChangeItemDict );
	m_changesList->blockSignals( false );

	if ( m_changesList->selectedItem() )
	{
		// FIXME: This is ugly...
		m_selectedDifference = (static_cast<KChangeLVI*>(m_changesList->selectedItem()))->difference();
	}

	emit selectionChanged( m_selectedModel, m_selectedDifference );
}

void KompareNavTreePart::slotChangesListSelectionChanged( Q3ListViewItem* item )
{
	kDebug(8105) << "Sent by the changesList" << endl;

	KChangeLVI* change = static_cast<KChangeLVI*>(item);
	m_selectedDifference = change->difference();

	emit selectionChanged( m_selectedDifference );
}

void KompareNavTreePart::slotApplyDifference( bool /*apply*/ )
{
	KChangeLVI* clvi = m_diffToChangeItemDict[m_selectedDifference];
	if ( clvi )
		clvi->setDifferenceText();
}

void KompareNavTreePart::slotApplyAllDifferences( bool /*apply*/ )
{
	QHash<const Diff2::Difference*, KChangeLVI*>::ConstIterator it = m_diffToChangeItemDict.constBegin();
	QHash<const Diff2::Difference*, KChangeLVI*>::ConstIterator end = m_diffToChangeItemDict.constEnd();

	kDebug(8105) << "m_diffToChangeItemDict.count() = " << m_diffToChangeItemDict.count() << endl;

	for ( ; it != end ; ++it )
	{
		it.value()->setDifferenceText();
	}
}

void KompareNavTreePart::slotApplyDifference( const Difference* diff, bool /*apply*/ )
{
	// this applies to the currently selected difference
	KChangeLVI* clvi = m_diffToChangeItemDict[diff];
	if ( clvi )
		clvi->setDifferenceText();
}

void KChangeLVI::setDifferenceText()
{
	QString text;
	switch( m_difference->type() ) {
	case Difference::Change:
		// Shouldn't this simply be diff->sourceLineCount() ?
		// because you change the _number of lines_ lines in source, not in dest
		if( m_difference->applied() )
			text = i18np( "Applied: Changes made to %1 line undone", "Applied: Changes made to  %1 lines undone",
			             m_difference->sourceLineCount() );
		else
			text = i18np( "Changed %1 line", "Changed %1 lines",
			             m_difference->sourceLineCount() );
		break;
	case Difference::Insert:
		if( m_difference->applied() )
			text = i18np( "Applied: Insertion of %1 line undone", "Applied: Insertion of %1 lines undone",
			             m_difference->destinationLineCount() );
		else
			text = i18np( "Inserted %1 line", "Inserted %1 lines",
			             m_difference->destinationLineCount() );
		break;
	case Difference::Delete:
		if( m_difference->applied() )
			text = i18np( "Applied: Deletion of %1 line undone", "Applied: Deletion of %1 lines undone",
			             m_difference->sourceLineCount() );
		else
			text = i18np( "Deleted %1 line", "Deleted %1 lines",
			             m_difference->sourceLineCount() );
		break;
	default:
		kDebug(8105) << "Unknown or Unchanged enum value when checking for diff->type() in KChangeLVI's constructor" << endl;
		text = "";
	}

	setText( 2, text );
}

KChangeLVI::KChangeLVI( K3ListView* parent, Difference* diff ) : K3ListViewItem( parent )
{
	m_difference = diff;

	setText( 0, QString::number( diff->sourceLineNumber() ) );
	setText( 1, QString::number( diff->destinationLineNumber() ) );

	setDifferenceText();
}

int KChangeLVI::compare( Q3ListViewItem* item, int column, bool ascending ) const
{
	if ( ascending )
	{
		if ( this->text(column).length() < item->text(column).length() )
			return -1;
		if ( this->text(column).length() > item->text(column).length() )
			return 1;
	}
	else
	{
		if ( this->text(column).length() > item->text(column).length() )
			return -1;
		if ( this->text(column).length() < item->text(column).length() )
			return 1;
	}

	return key( column, ascending ).compare( item->key( column, ascending ) );
}

KChangeLVI::~KChangeLVI()
{
}

KFileLVI::KFileLVI( K3ListView* parent, DiffModel* model ) : K3ListViewItem( parent )
{
	m_model = model;

	QString src = model->sourceFile();
	QString dst = model->destinationFile();

	setText( 0, src );
	setText( 1, dst );
	setPixmap( 0, SmallIcon( getIcon( src ) ) );
	setPixmap( 1, SmallIcon( getIcon( dst ) ) );
	setSelectable( true );
}

bool KFileLVI::hasExtension(const QString& extensions, const QString& fileName)
{
    QStringList extList = extensions.split(' ');
	foreach (const QString &ext, extList) {
		if ( fileName.endsWith(ext, Qt::CaseInsensitive) ) {
			return true;
		}
	}
	return false;
}

const QString KFileLVI::getIcon(const QString& fileName)
{
	// C++, C
	if ( hasExtension( ".h .hpp", fileName ) ) {
		return "text-x-c++hdr";
	}
	if ( hasExtension( ".cpp", fileName ) ) {
		return "text-x-c++src";
	}
	if ( hasExtension( ".c", fileName ) ) {
		return "text-x-csrc";
	}
	// Python
	if ( hasExtension( ".py .pyw", fileName ) ) {
		return "text-x-python";
	}
	// C#
	if ( hasExtension( ".cs", fileName ) ) {
		return "text-x-csharp";
	}
	// Objective-C
	if ( hasExtension( ".m", fileName ) ) {
		return "text-x-objcsrc";
	}
	// Java
	if ( hasExtension( ".java", fileName ) ) {
		return "text-x-java";
	}
	// Script
	if ( hasExtension( ".sh", fileName ) ) {
		return "text-x-script";
	}
	// Makefile
	if ( hasExtension( ".cmake Makefile", fileName ) ) {
		return "text-x-makefile";
	}
	// Ada
	if ( hasExtension( ".ada .ads .adb", fileName ) ) {
		return "text-x-adasrc";
	}
	// Pascal
	if ( hasExtension( ".pas", fileName ) ) {
		return "text-x-pascal";
	}
	// Patch
	if ( hasExtension( ".diff", fileName ) ) {
		return "text-x-patch";
	}
	// Tcl
	if ( hasExtension( ".tcl", fileName ) ) {
		return "text-x-tcl";
	}
	// Text
	if ( hasExtension( ".txt", fileName ) ) {
		return "text-plain";
	}
	// Xml
	if ( hasExtension( ".xml", fileName ) ) {
		return "text-xml";
	}
	// unknown or no file extension
	return "text-plain";
}

void KFileLVI::fillChangesList( K3ListView* changesList, QHash<const Diff2::Difference*, KChangeLVI*>* diffToChangeItemDict )
{
	changesList->clear();
	diffToChangeItemDict->clear();

	DifferenceListConstIterator diffIt = m_model->differences()->constBegin();
	DifferenceListConstIterator dEnd   = m_model->differences()->constEnd();

	for ( ; diffIt != dEnd; ++diffIt )
	{
		KChangeLVI* change = new KChangeLVI( changesList, *diffIt );
		diffToChangeItemDict->insert( *diffIt, change );
	}

	changesList->setSelected( changesList->firstChild(), true );
}

KFileLVI::~KFileLVI()
{
}

KDirLVI::KDirLVI( K3ListView* parent, QString& dir ) : K3ListViewItem( parent )
{
//	kDebug(8105) << "KDirLVI (K3ListView) constructor called with dir = " << dir << endl;
	m_rootItem = true;
	m_dirName = dir;
	setPixmap( 0, SmallIcon( "folder" ) );
	setOpen( true );
	setSelectable( true );
	if ( m_dirName.isEmpty() )
		setText( 0, i18n( "Unknown" ) );
	else
		setText( 0, m_dirName );
}

KDirLVI::KDirLVI( KDirLVI* parent, QString& dir ) : K3ListViewItem( parent )
{
//	kDebug(8105) << "KDirLVI (KDirLVI) constructor called with dir = " << dir << endl;
	m_rootItem = false;
	m_dirName = dir;
	setPixmap( 0, SmallIcon( "folder" ) );
	setOpen( true );
	setSelectable( true );
	setText( 0, m_dirName );
}

// addModel always removes it own path from the beginning
void KDirLVI::addModel( QString& path, DiffModel* model, QHash<const Diff2::DiffModel*, KDirLVI*>* modelToDirItemDict )
{
//	kDebug(8105) << "KDirLVI::addModel called with path = " << path << " from KDirLVI with m_dirName = " << m_dirName << endl;

	if ( !m_dirName.isEmpty() )
	{
		if ( path.find( m_dirName ) > -1 )
			path = path.remove( path.find( m_dirName ), m_dirName.length() );
	}

//	kDebug(8105) << "Path after removal of own dir (\"" << m_dirName << "\") = " << path << endl;

	if ( path.isEmpty() ) {
		m_modelList.append( model );
		modelToDirItemDict->insert( model, this );
		return;
	}

	KDirLVI* child;

	QString dir = path.mid( 0, path.find( "/", 0 ) + 1 );
	child = findChild( dir );
	if ( !child )
	{
		// does not exist yet so make it
//		kDebug(8105) << "KDirLVI::addModel creating new KDirLVI because not found" << endl;
		child = new KDirLVI( this, dir );
	}

	child->addModel( path, model, modelToDirItemDict );
}

KDirLVI* KDirLVI::findChild( QString dir )
{
//	kDebug(8105) << "KDirLVI::findChild called with dir = " << dir << endl;
	KDirLVI* child;
	if ( ( child = static_cast<KDirLVI*>(this->firstChild()) ) != 0L )
	{ // has children, check if dir already exists, if so addModel
		do {
			if ( dir == child->dirName() )
				return child;
		} while ( ( child = static_cast<KDirLVI*>(child->nextSibling()) ) != 0L );
	}

	return 0L;
}

void KDirLVI::fillFileList( K3ListView* fileList, QHash<const Diff2::DiffModel*, KFileLVI*>* modelToFileItemDict )
{
	fileList->clear();

	DiffModelListIterator modelIt = m_modelList.begin();
	DiffModelListIterator mEnd    = m_modelList.end();
	for ( ;modelIt != mEnd; ++modelIt )
	{
		KFileLVI* file = new KFileLVI( fileList, *modelIt );
		modelToFileItemDict->insert( *modelIt, file );
	}

	fileList->setSelected( fileList->firstChild(), true );
}

QString KDirLVI::fullPath( QString& path )
{
//	if ( !path.isEmpty() )
//		kDebug(8105) << "KDirLVI::fullPath called with path = " << path << endl;
//	else
//		kDebug(8105) << "KDirLVI::fullPath called with empty path..." << endl;

	if ( m_rootItem ) // don't bother adding rootItem's dir...
		return path;

	path = path.prepend( m_dirName );

	KDirLVI* lviParent = dynamic_cast<KDirLVI*>( parent() );
	if ( lviParent )
	{
		path = lviParent->fullPath( path );
	}

	return path;
}

KDirLVI* KDirLVI::setSelected( QString dir )
{
//	kDebug(8105) << "KDirLVI::setSelected called with dir = " << dir << endl;

	// root item's dirName is never taken into account... remember that
	if ( !m_rootItem )
	{
		dir = dir.remove( 0, m_dirName.length() );
	}

	if ( dir.isEmpty() )
	{
		return this;
	}
	KDirLVI* child = static_cast<KDirLVI*>(firstChild());
	if ( !child )
		return 0L;

	do {
		if ( dir.startsWith( child->dirName() ) )
			return child->setSelected( dir );
	} while ( ( child = static_cast<KDirLVI*>(child->nextSibling()) ) != 0L );

	return 0L;
}

KDirLVI::~KDirLVI()
{
	m_modelList.clear();
}

// part stuff
KComponentData *KompareNavTreePartFactory::s_instance = 0L;
KAboutData* KompareNavTreePartFactory::s_about = 0L;

KompareNavTreePartFactory::KompareNavTreePartFactory()
    : KParts::Factory()
{
}

KompareNavTreePartFactory::~KompareNavTreePartFactory()
{
	delete s_instance;
	delete s_about;

	s_instance = 0L;
}

KParts::Part* KompareNavTreePartFactory::createPartObject( QWidget* parentWidget,
                                                  QObject* /*parent*/,
                                                  const char* /*classname*/, const QStringList & /*args*/ )
{
	// Create an instance of our Part
	KompareNavTreePart* obj = new KompareNavTreePart( parentWidget, 0 );

	KGlobal::locale()->insertCatalog("kompare");

	return obj;
}

const KComponentData &KompareNavTreePartFactory::componentData()
{
	if( !s_instance )
	{
		s_about = new KAboutData("komparenavtreepart", 0, ki18n("KompareNavTreePart"), "1.2");
		s_about->addAuthor(ki18n("John Firebaugh"), ki18n("Author"), "jfirebaugh@kde.org");
		s_about->addAuthor(ki18n("Otto Bruggeman"), ki18n("Author"), "bruggie@gmail.com" );
		s_instance = new KComponentData(s_about);
	}
	return *s_instance;
}

K_EXPORT_COMPONENT_FACTORY( libkomparenavtreepart, KompareNavTreePartFactory )
K_EXPORT_PLUGIN( KompareNavTreePartFactory )

#include "komparenavtreepart.moc"
