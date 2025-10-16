#pragma execution_character_set("utf-8")
#include <QApplication>
#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <qpainterpath.h>
#include <qpainter.h>
#include <QMouseEvent>
#include "QSchoolInfoWidget.h"
#include "QClassMgr.h"
#include "MemberManagerWidget.h"

class SchoolInfoDialog : public QDialog
{
    Q_OBJECT
public:
    SchoolInfoDialog(QWidget *parent = nullptr) : QDialog(parent),
        m_backgroundColor(QColor(50, 50, 50)),
        m_dragging(false),
        m_borderColor(Qt::white),
        m_borderWidth(2), m_radius(6)
    {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setWindowTitle("学校信息");
        resize(1000, 400);

        // 设置背景图片
        //setStyleSheet("QDialog {"
        //              "background-image: url(:/images/bg.png);"
        //              "background-repeat: no-repeat;"
        //              "background-position: center;"
        //              "}");

        // 左侧菜单按钮
        menuLayout = new QVBoxLayout;
        btn1 = new QPushButton("学校信息");
        btn1->setStyleSheet("background-color:red; color:white; font-size:16px;");
        btn2 = new QPushButton("班级管理");
        btn3 = new QPushButton("通讯录");
        btn4 = new QPushButton("健康管理");
        btn5 = new QPushButton("成长管理");

        QString greenBtnStyle = "background-color:green; color:white; font-size:16px;";
        btn2->setStyleSheet(greenBtnStyle);
        btn3->setStyleSheet(greenBtnStyle);
        btn4->setStyleSheet(greenBtnStyle);
        btn5->setStyleSheet(greenBtnStyle);

        menuLayout->addWidget(btn1);
        menuLayout->addWidget(btn2);
        menuLayout->addWidget(btn3);
        menuLayout->addWidget(btn4);
        menuLayout->addWidget(btn5);
        menuLayout->addStretch();

        closeButton = new QPushButton;
        closeButton->setIcon(QIcon(":/res/img/widget-close.png"));
        closeButton->setIconSize(QSize(22, 22));
        closeButton->setFixedSize(QSize(22, 22));
        closeButton->setStyleSheet("background: transparent;");
        closeButton->move(width() - 24, 4);
        closeButton->hide();
        connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);

        connect(btn1, &QPushButton::clicked, this, [=]() {
            int count = mainLayout->count();
            /*if(2 <= mainLayout->count())
            { 
                if (curWidget)
                {
                    mainLayout->removeWidget(curWidget);
                    mainLayout->addWidget(schoolInfoWidget);
                    curWidget = schoolInfoWidget;
                }
            }
            else*/
            {
                m_vBoxLayout->addWidget(schoolInfoWidget);
                schoolInfoWidget->show();
                classMgr->hide();
                memberMgrWidget->hide();
                curWidget = schoolInfoWidget;

                btn1->setStyleSheet("background-color:red; color:white; font-size:16px;");
                QString greenBtnStyle = "background-color:green; color:white; font-size:16px;";
                btn2->setStyleSheet(greenBtnStyle);
                btn3->setStyleSheet(greenBtnStyle);
            }
        });

        connect(btn2, &QPushButton::clicked, this, [=]() {
            int count = mainLayout->count();
            /*if (2 <= mainLayout->count())
            {
                if (curWidget)
                {
                    mainLayout->removeWidget(curWidget);
                    mainLayout->addWidget(classMgr);
                    curWidget = classMgr;
                }
            }
            else*/
            {

                QString schoolId = schoolInfoWidget->getSchoolId();
                classMgr->setSchoolId(schoolId);
                classMgr->initData();
                m_vBoxLayout->addWidget(classMgr);
                classMgr->show();
                schoolInfoWidget->hide();
                memberMgrWidget->hide();
                curWidget = classMgr;

                btn2->setStyleSheet("background-color:red; color:white; font-size:16px;");
                QString greenBtnStyle = "background-color:green; color:white; font-size:16px;";
                btn1->setStyleSheet(greenBtnStyle);
                btn3->setStyleSheet(greenBtnStyle);
            }
        });

        connect(btn3, &QPushButton::clicked, this, [=]() {
            int count = mainLayout->count();
            /*if (2 <= mainLayout->count())
            {
                if (curWidget)
                {
                    mainLayout->removeWidget(curWidget);
                    mainLayout->addWidget(classMgr);
                    curWidget = classMgr;
                }
            }
            else*/
            {
                m_vBoxLayout->addWidget(memberMgrWidget);
                QString schoolId = schoolInfoWidget->getSchoolId();
                memberMgrWidget->setSchoolId(schoolId);
                memberMgrWidget->show();
                schoolInfoWidget->hide();
                classMgr->hide();
                curWidget = classMgr;

                btn3->setStyleSheet("background-color:red; color:white; font-size:16px;");
                QString greenBtnStyle = "background-color:green; color:white; font-size:16px;";
                btn1->setStyleSheet(greenBtnStyle);
                btn2->setStyleSheet(greenBtnStyle);
            }
            });

        QHBoxLayout* closeLayout = new QHBoxLayout;
        m_vBoxLayout = new QVBoxLayout;

        schoolInfoWidget = new QSchoolInfoWidget(NULL);
        classMgr = new QClassMgr(NULL);
        memberMgrWidget = new MemberManagerWidget;

        pLabel = new QLabel(NULL);
        pLabel->setFixedSize(QSize(22, 22));
        closeLayout->addWidget(pLabel);
        closeLayout->addStretch();
        closeLayout->addWidget(closeButton);

        m_vBoxLayout->addLayout(closeLayout);
        m_vBoxLayout->addWidget(schoolInfoWidget);

        // 总布局：左菜单 + 右表单
        mainLayout = new QHBoxLayout(NULL);
        mainLayout->addLayout(menuLayout);
        int count = mainLayout->count();
        mainLayout->addLayout(m_vBoxLayout);
        curWidget = schoolInfoWidget;
        schoolInfoWidget->show();
        count = mainLayout->count();
        setLayout(mainLayout);
    }

    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QRect rect(0, 0, width(), height());
        QPainterPath path;
        path.addRoundedRect(rect, m_radius, m_radius);

        p.fillPath(path, QBrush(m_backgroundColor));
        QPen pen(m_borderColor, m_borderWidth);
        p.strokePath(path, pen);

        // 标题
        p.setPen(Qt::white);
        p.drawText(10, 25, m_titleName);
    }

    void mousePressEvent(QMouseEvent* event) {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        }
    }

    void mouseMoveEvent(QMouseEvent* event) {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - m_dragStartPos);
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
        }
    }

    void leaveEvent(QEvent* event)
    {
        QDialog::leaveEvent(event);
        closeButton->hide();
    }

    void enterEvent(QEvent* event)
    {
        QDialog::enterEvent(event);
        if (m_visibleCloseButton)
            closeButton->show();
    }

    void resizeEvent(QResizeEvent* event)
    {
        QDialog::resizeEvent(event);
        //initShow();
        closeButton->move(this->width() - 22, 0);
    }

    void setTitleName(const QString& name) {
        m_titleName = name;
        update();
    }

    void visibleCloseButton(bool val)
    {
        m_visibleCloseButton = val;
    }

    void setBackgroundColor(const QColor& color) {
        m_backgroundColor = color;
        update();
    }

    void setBorderColor(const QColor& color) {
        m_borderColor = color;
        update();
    }

    void setBorderWidth(int val) {
        m_borderWidth = val;
        update();
    }

    void setRadius(int val) {
        m_radius = val;
        update();
    }

    void InitData(UserInfo userInfo)
    {
        m_userInfo = userInfo;
        if (schoolInfoWidget)
        {
            schoolInfoWidget->InitData(m_userInfo);
        }
        if (memberMgrWidget)
        {
            memberMgrWidget->InitData(m_userInfo);
        }
    }

    QSchoolInfoWidget* schoolInfoWidget = NULL;
    QClassMgr* classMgr = NULL;
    QHBoxLayout* mainLayout = NULL;
    MemberManagerWidget* memberMgrWidget = NULL;
    QWidget* curWidget = NULL;
    QColor m_backgroundColor;
    QColor m_borderColor;
    int m_borderWidth;
    int m_radius;
    QString m_titleName;
    bool m_dragging;
    bool m_visibleCloseButton;
    QPoint m_dragStartPos;
    QPushButton* closeButton = NULL;
    QVBoxLayout* m_vBoxLayout = NULL;
    QVBoxLayout* menuLayout = NULL;
    QLabel* pLabel = NULL;
    QPushButton* btn1 = NULL;
    QPushButton* btn2 = NULL;
    QPushButton* btn3 = NULL;
    QPushButton* btn4 = NULL;
    QPushButton* btn5 = NULL;
    UserInfo m_userInfo;
};