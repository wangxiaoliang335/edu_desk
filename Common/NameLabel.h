#pragma once

#include <QLabel>
#include <QPixmap>
#include <qpainter.h>
#include <QMouseEvent>
class NameLabel : public QLabel
{
    Q_OBJECT
public:
    explicit NameLabel(QWidget* parent = nullptr)
        : QLabel(parent) {}

    void setEditIcon(const QPixmap& pix) {
        editIcon = pix;
        update();
    }

signals:
    void editIconClicked();
    void labelClicked();

protected:
    void paintEvent(QPaintEvent* event) override {
        QLabel::paintEvent(event); // 先绘制原文字

        if (!editIcon.isNull()) {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            int iconSize = height() * 0.2; // 图标高度为文字高度的 60%
            int x = fontMetrics().horizontalAdvance(text()) + 4; // 文字后面留 4px
            int y = (height() - iconSize) * 2/3;
            iconRect = QRect(x, y, iconSize, iconSize);

            painter.drawPixmap(iconRect, editIcon);
        }
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (iconRect.contains(event->pos())) {
            emit editIconClicked();
        }
        else {
            emit labelClicked();
        }
    }

private:
    QPixmap editIcon;
    QRect iconRect;
};

