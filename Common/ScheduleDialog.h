#pragma once
#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QFrame>
#include "CustomListDialog.h"

class ScheduleDialog : public QDialog
{
    Q_OBJECT
public:
    ScheduleDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("课程表");
        resize(700, 500);
        setStyleSheet("QPushButton { font-size:14px; } QLabel { font-size:14px; }");

        customListDlg = new CustomListDialog(this);
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // 顶部：头像 + 班级信息 + 功能按钮 + 更多
        QHBoxLayout* topLayout = new QHBoxLayout;
        QLabel* lblAvatar = new QLabel("头像");
        lblAvatar->setFixedSize(50, 50);
        lblAvatar->setStyleSheet("background-color: lightgray; border:1px solid gray; text-align:center;");
        QLabel* lblClass = new QLabel("7年级3班");
        QPushButton* btnEdit = new QPushButton("✎");
        btnEdit->setFixedSize(24, 24);

        QPushButton* btnSeat = new QPushButton("座次表");
        QPushButton* btnCam = new QPushButton("摄像头");
        QPushButton* btnTalk = new QPushButton("对讲");
        QPushButton* btnMsg = new QPushButton("通知");
        QPushButton* btnTask = new QPushButton("作业");
        QString greenStyle = "background-color: green; color: white; padding: 4px 8px;";
        btnSeat->setStyleSheet(greenStyle);
        btnCam->setStyleSheet(greenStyle);
        btnTalk->setStyleSheet(greenStyle);
        btnMsg->setStyleSheet(greenStyle);
        btnTask->setStyleSheet(greenStyle);

        QPushButton* btnMore = new QPushButton("...");
        btnMore->setFixedSize(48, 24);
        btnMore->setText("...");
        btnMore->setStyleSheet(
            "QPushButton {"
            "background-color: transparent;"
            "color: black;"
            "font-weight: bold;"
            "font-size: 16px;"
            "border: none;"
            "}"
            "QPushButton:hover {"
            "color: black;"
            "background-color: transparent;"
            "}"
        );

        connect(btnMore, &QPushButton::clicked, this, [=]() {
            if (customListDlg && customListDlg->isHidden())
            {
                customListDlg->show();
            }
            else if (customListDlg && !customListDlg->isHidden())
            {
                customListDlg->hide();
            }
        });

        topLayout->addWidget(lblAvatar);
        topLayout->addWidget(lblClass);
        topLayout->addWidget(btnEdit);
        topLayout->addSpacing(10);
        topLayout->addWidget(btnSeat);
        topLayout->addWidget(btnCam);
        topLayout->addWidget(btnTalk);
        topLayout->addWidget(btnMsg);
        topLayout->addWidget(btnTask);
        topLayout->addStretch();
        topLayout->addWidget(btnMore);
        mainLayout->addLayout(topLayout);

        // 时间 + 科目行
        QHBoxLayout* timeLayout = new QHBoxLayout;
        QString timeStyle = "background-color: royalblue; color: white; font-size:12px; min-width:40px;";
        QString subjectStyle = "background-color: royalblue; color: white; font-size:12px; min-width:50px;";

        QStringList times = { "7:20","8:00","2:00","2:00","2:00","2:00","2:00","2:00","2:00","2:00","2:00","2:00","2:00" };
        QStringList subs = { "晨读","语文","数学","英语","物理","午饭","午休","数学","美术","道法","课服","晚自习","" };

        QVBoxLayout* vTimes = new QVBoxLayout;
        QHBoxLayout* hTimeRow = new QHBoxLayout;
        QHBoxLayout* hSubRow = new QHBoxLayout;

        for (int i = 0; i < times.size(); ++i) {
            QPushButton* btnT = new QPushButton(times[i]);
            btnT->setStyleSheet(timeStyle);
            hTimeRow->addWidget(btnT);

            QPushButton* btnS = new QPushButton(subs[i]);
            btnS->setStyleSheet(subjectStyle);
            hSubRow->addWidget(btnS);
        }

        vTimes->addLayout(hTimeRow);
        vTimes->addLayout(hSubRow);
        // 包一层方便加边框
        QFrame* frameTimes = new QFrame;
        frameTimes->setLayout(vTimes);
        frameTimes->setFrameShape(QFrame::StyledPanel);
        mainLayout->addWidget(frameTimes);

        // 红色分隔线与时间箭头
        QFrame* line = new QFrame;
        line->setFrameShape(QFrame::HLine);
        line->setStyleSheet("color: red; border: 1px solid red;");
        mainLayout->addWidget(line);

        QHBoxLayout* timeIndicatorLayout = new QHBoxLayout;
        QLabel* lblArrow = new QLabel("↓");
        QLabel* lblTime = new QLabel("12:10");
        lblTime->setAlignment(Qt::AlignCenter);
        lblTime->setFixedSize(50, 25);
        lblTime->setStyleSheet("background-color: pink; color:red; font-weight:bold;");
        timeIndicatorLayout->addWidget(lblArrow);
        timeIndicatorLayout->addWidget(lblTime);
        timeIndicatorLayout->addStretch();
        mainLayout->addLayout(timeIndicatorLayout);

        // 表格区域
        QTableWidget* table = new QTableWidget(5, 8);
        table->horizontalHeader()->setVisible(false);
        table->verticalHeader()->setVisible(false);
        table->setStyleSheet("QTableWidget { gridline-color:blue; } QHeaderView::section { background-color:blue; }");
        mainLayout->addWidget(table);

        // 底部右下角黄色圆圈数字
        QLabel* lblNum = new QLabel("3");
        lblNum->setAlignment(Qt::AlignCenter);
        lblNum->setFixedSize(30, 30);
        lblNum->setStyleSheet("background-color: yellow; color: red; font-weight: bold; font-size: 16px; border-radius: 15px;");
        mainLayout->addWidget(lblNum, 0, Qt::AlignRight);
    }
    CustomListDialog* customListDlg = NULL;
};
