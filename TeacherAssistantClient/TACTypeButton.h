#pragma once

#include <QPushButton>
#include <QColor>
#include <QPixmap>
#include <QIcon>
class TACTypeButton  : public QPushButton
{
	Q_OBJECT

public:
	TACTypeButton(QWidget *parent);
	~TACTypeButton();
    void setCornerIcon(const QPixmap& icon);
    void setCornerIcon(const QIcon& icon);
protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QColor m_normalBgColor;
    QColor m_checkedBgColor;
    QPixmap m_cornerIcon;
    int m_cornerIconSize;
    int m_cornerMargin;
};