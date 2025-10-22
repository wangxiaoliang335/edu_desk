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
#include "TAHttpHandler.h"
#include "ClickableLabel.h"

class ModalDialog : public QDialog
{
    Q_OBJECT
        Q_PROPERTY(QString titleName READ titleName WRITE setTitleName)
        Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
        Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor)
        Q_PROPERTY(int borderWidth READ borderWidth WRITE setBorderWidth)
        Q_PROPERTY(int radius READ radius WRITE setRadius)

public:
    explicit ModalDialog(QWidget* parent = nullptr);
    ~ModalDialog();
    void InitData();

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
    int getRegisterLogin() { return m_registerLogin; }
    int getResetPwdLogin() { return m_resetPwdLogin; }
    int getUserId() { return user_id; }
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

    QLineEdit* phoneEdit;
    QLineEdit* codeEdit;
    QPushButton* getCodeButton;
    QPushButton* loginButton;
    //QPushButton* closeButton;

    QLabel* errLabel = NULL;  //µÇÂ¼´íÎóÏûÏ¢
    QLabel* titleLabel = NULL;
    QPushButton* pwdLoginButton = NULL;
    QLabel* registerLabel;
    QLabel* resetPwdLabel;
    TAHttpHandler* m_httpHandler = NULL;

    QTimer countdownTimer;
    int countdownValue; // Ê£ÓàÃëÊý

    int m_pwdLogin = false;
    int m_registerLogin = false;
    int m_resetPwdLogin = false;
    int user_id = -1;
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
