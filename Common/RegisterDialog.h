#pragma once
#include <QDialog>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include "ClickableLabel.h"
#include "TAHttpHandler.h"

class RegisterDialog : public QDialog
{
    Q_OBJECT
        Q_PROPERTY(QString titleName READ titleName WRITE setTitleName)
        Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
        Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor)
        Q_PROPERTY(int borderWidth READ borderWidth WRITE setBorderWidth)
        Q_PROPERTY(int radius READ radius WRITE setRadius)

public:
    explicit RegisterDialog(QWidget* parent = nullptr);
    ~RegisterDialog();

    QString titleName() const { return m_titleName; }
    QColor backgroundColor() const { return m_backgroundColor; }
    QColor borderColor() const { return m_borderColor; }
    int borderWidth() const { return m_borderWidth; }
    int radius() const { return m_radius; }

    void setTitleName(const QString& name);
    void setBackgroundColor(const QColor& color);
    void setBorderColor(const QColor& color);
    void setBorderWidth(int val);
    void setRadius(int val);
    void visibleCloseButton(bool val);

    QString phoneNumber() const { return phoneEdit->text(); }
    QString verifyCode()  const { return codeEdit->text(); }
    int getIsPwdLogin() { return m_pwdLogin; }

signals:
    void pwdLogin();
private:
    bool m_dragging;
    QPoint m_dragStartPos;

    QString m_titleName;
    QColor m_backgroundColor;
    QColor m_borderColor;
    int m_borderWidth;
    int m_radius;

    QPushButton* closeButton = NULL;
    //QPushButton* okButton;
    //QPushButton* cancelButton;
    //QLabel* label = NULL;
    QVBoxLayout* mainLayout;
    bool m_visibleCloseButton;
    TAHttpHandler* m_httpHandler = NULL;

    QLineEdit* phoneEdit = NULL;
    QLineEdit* codeEdit = NULL;
    QLineEdit* pwdEdit = NULL;
    QLineEdit* secPwdEdit = NULL;
    QPushButton* getCodeButton;
    QLabel* errLabel = NULL;  //µÇÂ¼´íÎóÏûÏ¢
    QPushButton* loginButton;
    //QPushButton* closeButton;
    QLabel* titleLabel;
    QPushButton* pwdLoginButton = NULL;
    QLabel* registerLabel;
    QLabel* resetPwdLabel;

    QTimer countdownTimer;
    int countdownValue; // Ê£ÓàÃëÊý
    int m_pwdLogin = false;

private slots:
    void onGetCodeClicked();
    void onTimerTick();
    void onLoginClicked();
    void onPwdLoginClicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void enterEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
};
