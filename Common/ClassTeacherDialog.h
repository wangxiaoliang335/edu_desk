#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QPushButton>
#include <QFrame>
#include "ScheduleDialog.h"

class ClassTeacherDialog : public QDialog
{
    Q_OBJECT
public:
    ClassTeacherDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("班级 / 教师选择");
        resize(420, 300);
        setStyleSheet("background-color:#dde2f0; font-size:14px;");

        m_scheduleDlg = new ScheduleDialog(this);
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // 顶部黄色圆形数字标签
        QLabel* lblNum = new QLabel("2");
        lblNum->setAlignment(Qt::AlignCenter);
        lblNum->setFixedSize(30, 30);
        lblNum->setStyleSheet("background-color: yellow; color: red; font-weight: bold; font-size: 16px; border-radius: 15px;");
        mainLayout->addWidget(lblNum, 0, Qt::AlignCenter);

        // 班级区域
        QVBoxLayout* classLayout = new QVBoxLayout;
        QLabel* lblClassTitle = new QLabel("班级");
        lblClassTitle->setStyleSheet("background-color:#3b73b8; color:white; font-weight:bold; padding:6px;");
        classLayout->addWidget(lblClassTitle);

        QWidget* classListWidget = new QWidget;
        QVBoxLayout* classListLayout = new QVBoxLayout(classListWidget);
        classListLayout->setSpacing(8);
        addPersonRow(classListLayout, ":/icons/avatar1.png", "软件开发工程师");
        addPersonRow(classListLayout, ":/icons/avatar2.png", "苏州-UI-已入职");
        addPersonRow(classListLayout, ":/icons/avatar3.png", "平平淡淡");
        classLayout->addWidget(classListWidget);

        mainLayout->addLayout(classLayout);

        // 教师区域
        QVBoxLayout* teacherLayout = new QVBoxLayout;
        QLabel* lblTeacherTitle = new QLabel("教师");
        lblTeacherTitle->setStyleSheet("background-color:#3b73b8; color:white; font-weight:bold; padding:6px;");
        teacherLayout->addWidget(lblTeacherTitle);

        QWidget* teacherListWidget = new QWidget;
        QVBoxLayout* teacherListLayout = new QVBoxLayout(teacherListWidget);
        teacherListLayout->setSpacing(8);
        addPersonRow(teacherListLayout, ":/icons/avatar1.png", "软件开发工程师", true); // 这里演示一个选中状态
        addPersonRow(teacherListLayout, ":/icons/avatar2.png", "苏州-UI-已入职");
        addPersonRow(teacherListLayout, ":/icons/avatar3.png", "平平淡淡");
        teacherLayout->addWidget(teacherListWidget);

        mainLayout->addLayout(teacherLayout);

        // 底部按钮
        QHBoxLayout* bottomLayout = new QHBoxLayout;
        QPushButton* btnCancel = new QPushButton("取消");
        QPushButton* btnOk = new QPushButton("确定");
        btnCancel->setStyleSheet("background-color:green; color:white; padding:6px; border-radius:4px;");
        btnOk->setStyleSheet("background-color:green; color:white; padding:6px; border-radius:4px;");
        bottomLayout->addStretch();
        bottomLayout->addWidget(btnCancel);
        bottomLayout->addWidget(btnOk);
        mainLayout->addLayout(bottomLayout);

        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
        //connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
        connect(btnOk, &QPushButton::clicked, this, [=]() {
            if (m_scheduleDlg && m_scheduleDlg->isHidden())
            {
                m_scheduleDlg->show();
            }
            accept();
        });
    }

private:
    void addPersonRow(QVBoxLayout* parentLayout, const QString& iconPath, const QString& name, bool checked = false)
    {
        QHBoxLayout* rowLayout = new QHBoxLayout;
        QRadioButton* radio = new QRadioButton;
        radio->setChecked(checked);

        QLabel* avatar = new QLabel;
        avatar->setFixedSize(36, 36);
        avatar->setStyleSheet("background-color: lightgray; border-radius: 18px;");
        // 如果有头像图片资源，可以这样设置：
        // QPixmap pix(iconPath);
        // avatar->setPixmap(pix.scaled(36,36, Qt::KeepAspectRatio, Qt::SmoothTransformation));

        QLabel* lblName = new QLabel(name);

        rowLayout->addWidget(radio);
        rowLayout->addWidget(avatar);
        rowLayout->addWidget(lblName);
        rowLayout->addStretch();

        QWidget* rowWidget = new QWidget;
        rowWidget->setLayout(rowLayout);
        parentLayout->addWidget(rowWidget);
    }
private:
    ScheduleDialog* m_scheduleDlg = NULL;
};
