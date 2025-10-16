#pragma once

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QLabel>

typedef struct tagUserInfo {
    QString strPhone;
    QString strName;
    QString strSex;
    QString strAddress;
    QString strSchoolName;
    QString strGradeLevel;
    QString strGrade;
    QString strSubject;
    QString strClassTaught;
    QString strIsAdministrator;
    QString avatar;
    QString strIdNumber;
}UserInfo;

class TABaseDialog  : public QWidget
{
	Q_OBJECT

public:
	TABaseDialog(QWidget *parent);
	~TABaseDialog();
    void setTitle(const QString& text);
    Q_PROPERTY(QString titleName READ titleName WRITE setTitleName)
        Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
        Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor)
        Q_PROPERTY(int borderWidth READ borderWidth WRITE setBorderWidth)
        Q_PROPERTY(int radius READ radius WRITE setRadius)
        QString titleName() const;
    QColor backgroundColor() const;
    QColor borderColor() const;
    QRect getScreenGeometryWithTaskbar();
    int radius();
    int borderWidth();
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void clearLayout(QLayout* layout);

public slots:
    void setTitleName(const QString& name);
    void setBackgroundColor(const QColor& color);
    void setBorderColor(const QColor& color);
    void setBorderWidth(int val);
    void setRadius(int val);

protected:
    QWidget* titleBar;
    QLabel* titleLabel;
    QPushButton* closeButton;
    QVBoxLayout* mainLayout;
    QHBoxLayout* titleLayout;
    QVBoxLayout* contentLayout;

private slots:
    void onClose();
private:
    bool m_dragging;
    QPoint m_dragStartPosition;
    QString m_titleName;
    QColor m_backgroundColor;
    QColor m_borderColor;
    int m_borderWidth;
    int m_radius;
};
