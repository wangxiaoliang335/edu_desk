#include <QHBoxLayout>
#include <QLabel>
#include "common.h"
#include "TACCountDownWidget.h"

TACCountDownWidget::TACCountDownWidget(QWidget *parent)
	: TAFloatingWidget(parent)
{
	this->setObjectName("TACCountDownWidget");
	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setSpacing(0);

	contentLabel = new QLabel(this);
	contentLabel->setAlignment(Qt::AlignVCenter | Qt::AlignCenter);
	layout->addWidget(contentLabel);

	
	setLayout(layout);
	resize(656, 62);
	this->setBackgroundColor(QColor(243, 96, 93, 50));
	this->setBorderColor(QColor(243, 94, 91, 100));
	this->setBorderWidth(WIDGET_BORDER_WIDTH);
	this->setRadius(20);
}

TACCountDownWidget::~TACCountDownWidget()
{}


void TACCountDownWidget::setContent(const QString& text)
{
	contentLabel->setText(text);
}

void TACCountDownWidget::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	initShow();
}

void TACCountDownWidget::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	initShow();
}
void TACCountDownWidget::initShow()
{
	QRect rect = this->getScreenGeometryWithTaskbar();
	if (rect.isEmpty())
		return;
	QSize windowSize = this->size();
	int x = rect.x() + (rect.width() - windowSize.width()) / 2;
	int y = rect.y() + 40;
	this->move(x, y);
}
void TACCountDownWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		emit doubleClicked();
	}
}