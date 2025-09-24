#include "TACTypeButton.h"
#include <QPainter>
#include <QStyleOptionButton>
#include <QStyle>
#include <QPixmap>
#include <QPainterPath>
TACTypeButton::TACTypeButton(QWidget* parent)
    : QPushButton(parent), m_normalBgColor(Qt::lightGray),
    m_checkedBgColor(Qt::darkBlue),
    m_cornerIconSize(23),
    m_cornerMargin(0)
{
    this->setCheckable(true);
}

TACTypeButton::~TACTypeButton()
{}


void TACTypeButton::setCornerIcon(const QPixmap& icon)
{
    m_cornerIcon = icon.scaled(m_cornerIconSize, m_cornerIconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    update();
}

void TACTypeButton::setCornerIcon(const QIcon& icon)
{
    QPixmap pixmap = icon.pixmap(QSize(m_cornerIconSize, m_cornerIconSize), QIcon::Normal, QIcon::On);
    setCornerIcon(pixmap);
}

void TACTypeButton::paintEvent(QPaintEvent* event)
{
    QPushButton::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QRect buttonRect = this->rect();
    if (!m_cornerIcon.isNull() && this->isChecked()) {
        int x = buttonRect.right() - m_cornerIconSize - m_cornerMargin;
        int y = buttonRect.bottom() - m_cornerIconSize - m_cornerMargin;
        QRect iconRect(x, y, m_cornerIconSize, m_cornerIconSize);
        painter.drawPixmap(iconRect, m_cornerIcon);
    }
    
}