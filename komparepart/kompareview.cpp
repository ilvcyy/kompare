
#include "kdiffview.h"

#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qstrlist.h>
#include <qtextstream.h>
#include <qfile.h>
#include <klocale.h>

#include "diffview.h"
#include "diffmodel.h"
#include "diffhunk.h"
#include "difference.h"
#include "generalsettings.h"

KDiffView::KDiffView( GeneralSettings* settings, QWidget *parent, const char *name )
    : QWidget(parent, name)
{
	m_settings = settings;

	QGridLayout *pairlayout = new QGridLayout(this, 2, 3, 10);
	pairlayout->setSpacing( 0 );
	pairlayout->setMargin( 0 );
	pairlayout->setRowStretch(0, 0);
	pairlayout->setRowStretch(1, 1);
	pairlayout->setColStretch(1, 0);
	pairlayout->addColSpacing(1, 16);
	pairlayout->setColStretch(0, 10);
	pairlayout->setColStretch(2, 10);

	QFrame* Frame1 = new QFrame( this, "Frame1" );
	Frame1->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)5, (QSizePolicy::SizeType)1, Frame1->sizePolicy().hasHeightForWidth() ) );
	Frame1->setFrameShape( QFrame::StyledPanel );
	Frame1->setFrameShadow( QFrame::Raised );
	QVBoxLayout* Frame1Layout = new QVBoxLayout( Frame1 );
	Frame1Layout->setSpacing( 0 );
	Frame1Layout->setMargin( 3 );
	revlabel1 = new QLabel("Rev A", Frame1);
	Frame1Layout->addWidget( revlabel1 );
	pairlayout->addWidget(Frame1, 0, 0);

	QFrame* Frame3 = new QFrame( this, "Frame3" );
	Frame3->setFrameShape( QFrame::StyledPanel );
	Frame3->setFrameShadow( QFrame::Raised );
	pairlayout->addWidget( Frame3, 0, 1 );

	QFrame* Frame2 = new QFrame( this, "Frame2" );
	Frame2->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)5, (QSizePolicy::SizeType)3, Frame2->sizePolicy().hasHeightForWidth() ) );
	Frame2->setFrameShape( QFrame::StyledPanel );
	Frame2->setFrameShadow( QFrame::Raised );
	QVBoxLayout* Frame2Layout = new QVBoxLayout( Frame2 );
	Frame2Layout->setSpacing( 0 );
	Frame2Layout->setMargin( 3 );
	revlabel2 = new QLabel("Rev A", Frame2 );
	Frame2Layout->addWidget( revlabel2 );
	pairlayout->addWidget( Frame2, 0, 2 );

	diff1 = new DiffView( m_settings, true, false, this);
	diff2 = new DiffView( m_settings, true, true, this);
	pairlayout->addWidget(diff1, 1, 0);
	pairlayout->addWidget(diff2, 1, 2);
	diff1->setPartner(diff2);
	diff2->setPartner(diff1);

	QFrame* Frame6 = new QFrame( this, "Frame6" );
	Frame6->setFrameShape( QFrame::StyledPanel );
	Frame6->setFrameShadow( QFrame::Sunken );
	QVBoxLayout* Frame6Layout = new QVBoxLayout( Frame6 );
	Frame6Layout->setSpacing( 0 );
	Frame6Layout->setMargin( 1 );
	zoom = new DiffZoomWidget( m_settings, Frame6 );
	zoom->setDiffView(diff2);
	Frame6Layout->addWidget( zoom );
	pairlayout->addWidget(Frame6,  1, 1);

	markeditem = -1;
	model = new DiffModel();
	ownsModel = true;
}

KDiffView::~KDiffView()
{
	if( ownsModel ) delete model;
}

DiffModel* KDiffView::getModel()
{
	return model;
}

void KDiffView::setModel( DiffModel* model, bool ownsModel )
{
	this->model = model;
	this->ownsModel = ownsModel;
	QListIterator<DiffHunk> it = QListIterator<DiffHunk>( model->getHunks() );
	for( ; it.current(); ++it ) {
		const DiffHunk* h = it.current();
		int linenoA = h->lineStartA;
		int linenoB = h->lineStartB;
		diff1->addLine( i18n( "%1: Line %1" ).arg( model->getSourceFilename() ).arg( linenoA ), Difference::Separator);
		diff2->addLine( i18n( "%1: Line %1" ).arg( model->getDestinationFilename() ).arg( linenoB ), Difference::Separator);
		QListIterator<Difference> differences = QListIterator<Difference>( h->getDifferences() );
		for( ; differences.current(); ++differences ) {
			const Difference* d = differences.current();
			const QStringList linesA = d->getSourceLines();
			const QStringList linesB = d->getDestinationLines();
			QStringList::ConstIterator itA = linesA.begin();
			QStringList::ConstIterator itB = linesB.begin();
			if( d->type == Difference::Unchanged ) {
				for ( ; itA != linesA.end(); ++itA ) {
					diff1->addLine((*itA), Difference::Unchanged,linenoA++);
					diff2->addLine((*itA), Difference::Unchanged,linenoB++);
				}
			} else {
				while( itA != linesA.end() || itB != linesB.end() ) {
					if (itA != linesA.end()) {
						diff1->addLine((*itA), Difference::Neutral,linenoA++);
						++itA;
						if (itB != linesB.end()) {
							diff2->addLine((*itB), Difference::Change,linenoB++);
							++itB;
						} else
							diff2->addLine("", Difference::Delete);
					} else {
						diff1->addLine("", Difference::Neutral);
						diff2->addLine((*itB), Difference::Insert,linenoB++);
						++itB;
					}
				}
			}
		}
	}
	revlabel1->setText( model->getSourceFilename() );
	revlabel2->setText( model->getDestinationFilename() );
	setSelectedItem( 0 );
	zoom->repaint();
	emit itemsChanged();
}

void KDiffView::setSynchronizeScrollBars( bool b )
{
	diff1->setPartner(b? diff2 : 0);
	diff2->setPartner(b? diff1 : 0);
}

void KDiffView::setFont( const QFont& font )
{
	diff1->setFont(font);
	diff2->setFont(font);
}

void KDiffView::setTabWidth( uint tabWidth )
{
	diff1->setTabWidth(tabWidth);
	diff2->setTabWidth(tabWidth);
}

int KDiffView::getSelectedItem()
{
	return markeditem;
}

void KDiffView::setSelectedItem( int newitem )
{
	if (markeditem >= 0) {
		const Difference* d = const_cast<QList<Difference>&>(model->getDifferences()).at((uint)markeditem);
		for (int i = d->linenoA; i < d->linenoA+d->sourceLineCount(); ++i)
			diff1->setInverted(i, false);
		for (int i = d->linenoB; i < d->linenoB+d->destinationLineCount(); ++i)
			diff2->setInverted(i, false);
	}

	markeditem = newitem;

	if (markeditem >= 0) {
		const Difference* d = const_cast<QList<Difference>&>(model->getDifferences()).at(markeditem);
		for (int i = d->linenoA; i < d->linenoA+d->sourceLineCount(); ++i)
			diff1->setInverted(i, true);
		for (int i = d->linenoB; i < d->linenoB+d->destinationLineCount(); ++i)
			diff2->setInverted(i, true);
		diff1->setCenterLine(d->linenoA);
		diff2->setCenterLine(d->linenoB);
	}
	diff1->repaint();
	diff2->repaint();

	emit selectionChanged();
}

#include "kdiffview.moc"