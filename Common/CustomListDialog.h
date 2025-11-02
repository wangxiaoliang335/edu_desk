#pragma once
#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include "MidtermGradeDialog.h"
#include "StudentPhysiqueDialog.h"

class CustomListDialog : public QDialog
{
    Q_OBJECT
public:
    CustomListDialog(QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("列表管理");
        resize(300, 200);
        setStyleSheet("background-color: #f5f5f5;");

        m_midtermGradeDlg = new MidtermGradeDialog(this);
        m_studentPhysiqueDlg = new StudentPhysiqueDialog(this);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(15);
        mainLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

        // 添加两行列表项
        QPushButton* btnMidterm = addRow(mainLayout, "期中成绩单");
        QPushButton* btnPhysique = addRow(mainLayout, "学生体质统计表");

        // 连接期中成绩单按钮点击事件
        connect(btnMidterm, &QPushButton::clicked, this, [=]() {
            if (m_midtermGradeDlg && m_midtermGradeDlg->isHidden()) {
                m_midtermGradeDlg->show();
            } else if (m_midtermGradeDlg && !m_midtermGradeDlg->isHidden()) {
                m_midtermGradeDlg->hide();
            } else {
                m_midtermGradeDlg = new MidtermGradeDialog(this);
                m_midtermGradeDlg->show();
            }
        });

        // 连接学生体质统计表按钮点击事件
        connect(btnPhysique, &QPushButton::clicked, this, [=]() {
            if (m_studentPhysiqueDlg && m_studentPhysiqueDlg->isHidden()) {
                m_studentPhysiqueDlg->show();
            } else if (m_studentPhysiqueDlg && !m_studentPhysiqueDlg->isHidden()) {
                m_studentPhysiqueDlg->hide();
            } else {
                m_studentPhysiqueDlg = new StudentPhysiqueDialog(this);
                m_studentPhysiqueDlg->show();
            }
        });

        // 底部 "+"" 按钮
        QPushButton *btnAdd = new QPushButton("+");
        btnAdd->setFixedSize(40,40);
        btnAdd->setStyleSheet(
            "QPushButton { background-color: orange; color:white; font-weight:bold; font-size: 18px; border: 1px solid #555; }"
            "QPushButton:hover { background-color: #cc6600; }"
        );
        mainLayout->addSpacing(20);
        mainLayout->addWidget(btnAdd, 0, Qt::AlignCenter);
    }

private:
    QPushButton* addRow(QVBoxLayout *parentLayout, const QString &text)
    {
        QHBoxLayout *rowLayout = new QHBoxLayout;
        rowLayout->setSpacing(0);

        QPushButton *btnTitle = new QPushButton(text);
        btnTitle->setStyleSheet(
            "QPushButton { background-color: green; color: white; font-size: 14px; padding: 4px; border: 1px solid #555; }"
            "QPushButton:hover { background-color: darkgreen; }"
        );
        btnTitle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        QPushButton *btnClose = new QPushButton("X");
        btnClose->setFixedWidth(30);
        btnClose->setStyleSheet(
            "QPushButton { background-color: orange; color: white; font-weight:bold; border: 1px solid #555; }"
            "QPushButton:hover { background-color: #cc6600; }"
        );

        rowLayout->addWidget(btnTitle);
        rowLayout->addWidget(btnClose);
        parentLayout->addLayout(rowLayout);
        
        return btnTitle;
    }

private:
    MidtermGradeDialog* m_midtermGradeDlg = nullptr;
    StudentPhysiqueDialog* m_studentPhysiqueDlg = nullptr;
};
