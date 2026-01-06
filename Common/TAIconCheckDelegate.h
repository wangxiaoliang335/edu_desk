#pragma once

#include <QStyledItemDelegate>
#include <QPainter>
#include <QPixmap>
#include <QMouseEvent>
#include <QApplication>

// 使用图片自绘复选框（避免系统 checkIndicator + icon 叠加导致错位）
class TAIconCheckDelegate : public QStyledItemDelegate {
public:
    explicit TAIconCheckDelegate(QObject* parent = nullptr,
        const QString& iconPath = QString(":/res/img/com_ic_check_nor@3x.png"))
        : QStyledItemDelegate(parent),
        m_iconPath(iconPath),
        m_baseIcon(iconPath) {}

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
        const QModelIndex& index) const override {
        if (!painter) {
            return;
        }

        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);

        // 不让 style 画系统复选框/文本
        opt.features &= ~QStyleOptionViewItem::HasCheckIndicator;
        opt.text.clear();

        // 先画背景/选中态等
        const QWidget* w = opt.widget;
        QStyle* style = w ? w->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, w);

        // 取 CheckState
        const QVariant v = index.data(Qt::CheckStateRole);
        const bool isChecked = (v.isValid() && v.toInt() == Qt::Checked);

        // 图标大小随行高变化，保持居中
        int s = qMin(22, opt.rect.height() - 10);
        if (s < 14) s = 14;
        const QRect iconRect(
            opt.rect.center().x() - s / 2,
            opt.rect.center().y() - s / 2,
            s, s
        );

        if (m_baseIcon.isNull()) {
            // 资源可能尚未加载，尝试懒加载
            m_baseIcon = QPixmap(m_iconPath);
        }

        if (!m_baseIcon.isNull()) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
            painter->drawPixmap(iconRect, m_baseIcon.scaled(iconRect.size(),
                Qt::KeepAspectRatio, Qt::SmoothTransformation));

            // 选中态：在图标上叠加蓝色对勾（项目里只有 nor 图）
            if (isChecked) {
                QPen pen(QColor("#2563eb"));
                pen.setWidthF(2.2);
                pen.setCapStyle(Qt::RoundCap);
                pen.setJoinStyle(Qt::RoundJoin);
                painter->setPen(pen);

                const QPoint p1(iconRect.left() + iconRect.width() * 0.25, iconRect.top() + iconRect.height() * 0.55);
                const QPoint p2(iconRect.left() + iconRect.width() * 0.45, iconRect.top() + iconRect.height() * 0.72);
                const QPoint p3(iconRect.left() + iconRect.width() * 0.78, iconRect.top() + iconRect.height() * 0.32);
                painter->drawLine(p1, p2);
                painter->drawLine(p2, p3);
            }
            painter->restore();
        }
    }

    bool editorEvent(QEvent* event, QAbstractItemModel* model,
        const QStyleOptionViewItem& option, const QModelIndex& index) override {
        Q_UNUSED(option);
        if (!event || !model) {
            return false;
        }
        if (!(index.flags() & Qt::ItemIsUserCheckable) || !(index.flags() & Qt::ItemIsEnabled)) {
            return false;
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                const int state = model->data(index, Qt::CheckStateRole).toInt();
                const int next = (state == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
                model->setData(index, next, Qt::CheckStateRole);
                return true;
            }
        }
        if (event->type() == QEvent::KeyPress) {
            // Space/Enter 切换
            const int state = model->data(index, Qt::CheckStateRole).toInt();
            const int next = (state == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
            model->setData(index, next, Qt::CheckStateRole);
            return true;
        }
        return false;
    }

private:
    QString m_iconPath;
    mutable QPixmap m_baseIcon;
};


