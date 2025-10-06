#pragma once
#include <QLabel>
#include <QMouseEvent>

class ClickableLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ClickableLabel(QWidget *parent = nullptr)
        : QLabel(parent) {}

signals:
    void clicked();  // 自定义点击信号

protected:
    void mousePressEvent(QMouseEvent *event) override {
        // 判断是否左键点击
        if (event->button() == Qt::LeftButton) {
            emit clicked();  // 发出信号
        }
        // 保留原有的 QLabel 行为
        QLabel::mousePressEvent(event);
    }
};
