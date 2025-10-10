#pragma once

#include <QObject>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWidget>
#include <QVBoxLayout>
#include <QFileDialog>
class AvatarLabel  : public QLabel
{
	Q_OBJECT
public:
    explicit AvatarLabel(QWidget* parent = nullptr)
        : QLabel(parent)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    void setAvatar(const QPixmap& pix) {
        avatar = pix;
        update();
    }

    void setEditIcon(const QPixmap& pix) {
        editIcon = pix;
        update();
    }

signals:
    void avatarClicked();
    void editIconClicked();

protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        // 画圆形头像
        int side = qMin(width(), height());
        QPainterPath path;
        path.addEllipse(0, 0, side, side);
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, side, side, avatar);

        painter.setClipping(false);

        // 画右下角编辑图标
        if (!editIcon.isNull()) {
            iconSize = side / 3; // 记录 图标大小
            iconX = side - iconSize;
            iconY = side - iconSize;
            painter.drawPixmap(iconX, iconY, iconSize, iconSize, editIcon);
        }
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (!editIcon.isNull()) {
            QRect iconRect(iconX, iconY, iconSize, iconSize);
            if (iconRect.contains(event->pos())) {
                emit editIconClicked();
                return;
            }
        }
        emit avatarClicked();
    }

private:
    QPixmap avatar;
    QPixmap editIcon;
    int iconX{ 0 };
    int iconY{ 0 };
    int iconSize{ 0 };
};

