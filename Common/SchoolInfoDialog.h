#pragma execution_character_set("utf-8")
#include <QApplication>
#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "QSchoolInfoWidget.h"
#include "QClassMgr.h"
#include "MemberManagerWidget.h"

class SchoolInfoDialog : public QDialog
{
    Q_OBJECT
public:
    SchoolInfoDialog(QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("学校信息");
        resize(600, 400);

        // 设置背景图片
        setStyleSheet("QDialog {"
                      "background-image: url(:/images/bg.png);"
                      "background-repeat: no-repeat;"
                      "background-position: center;"
                      "}");

        // 左侧菜单按钮
        QVBoxLayout *menuLayout = new QVBoxLayout;
        QPushButton *btn1 = new QPushButton("学校信息");
        btn1->setStyleSheet("background-color:red; color:white; font-size:16px;");
        QPushButton *btn2 = new QPushButton("班级管理");
        QPushButton* btn3 = new QPushButton("通讯录");
        QPushButton *btn4 = new QPushButton("健康管理");
        QPushButton *btn5 = new QPushButton("成长管理");

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
                schoolInfoWidget->show();
                mainLayout->addWidget(schoolInfoWidget);
                classMgr->hide();
                memberMgrWidget->hide();
                curWidget = schoolInfoWidget;
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
                classMgr->show();
                mainLayout->addWidget(classMgr);
                schoolInfoWidget->hide();
                memberMgrWidget->hide();
                curWidget = classMgr;
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
                memberMgrWidget->show();
                mainLayout->addWidget(memberMgrWidget);
                schoolInfoWidget->hide();
                classMgr->hide();
                curWidget = classMgr;
            }
            });

        schoolInfoWidget = new QSchoolInfoWidget(this);
        classMgr = new QClassMgr(this);
        memberMgrWidget = new MemberManagerWidget(this);

        // 总布局：左菜单 + 右表单
        mainLayout = new QHBoxLayout(this);
        mainLayout->addLayout(menuLayout);
        int count = mainLayout->count();
        mainLayout->addWidget(schoolInfoWidget);
        curWidget = schoolInfoWidget;
        count = mainLayout->count();
        //mainLayout->addStretch();
    }
    QSchoolInfoWidget* schoolInfoWidget = NULL;
    QClassMgr* classMgr = NULL;
    QHBoxLayout* mainLayout = NULL;
    MemberManagerWidget* memberMgrWidget = NULL;
    QWidget* curWidget = NULL;
};