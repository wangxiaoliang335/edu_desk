#pragma execution_character_set("utf-8")
#include "QSchoolInfoWidget.h"
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

QSchoolInfoWidget::QSchoolInfoWidget(QWidget *parent)
	: QWidget(parent)
{// 右侧表单
    QGridLayout* formLayout = new QGridLayout;
    //        QLabel *lblSchool = new QLabel("学校名");
    QLabel* lblSchool = new QLabel("学校名");
    QLineEdit* editSchool = new QLineEdit;
    QLabel* lblAddr = new QLabel("地址");
    QLineEdit* editAddr = new QLineEdit;

    formLayout->addWidget(lblSchool, 0, 0);
    formLayout->addWidget(editSchool, 0, 1);
    formLayout->addWidget(lblAddr, 1, 0);
    formLayout->addWidget(editAddr, 1, 1);

    // 右侧按钮区域
    QPushButton* btnGetCode = new QPushButton("获取组织代码");
    btnGetCode->setStyleSheet("background-color:green; color:white; font-size:16px;");
    QLabel* lblCode = new QLabel("234868");
    lblCode->setAlignment(Qt::AlignCenter);
    lblCode->setStyleSheet("background-color:blue; color:white; font-size:20px;");
    lblCode->setFixedSize(100, 40);

    QHBoxLayout* codeLayout = new QHBoxLayout;
    codeLayout->addWidget(btnGetCode);
    codeLayout->addWidget(lblCode);

    QVBoxLayout* rightLayout = new QVBoxLayout(this);
    rightLayout->addLayout(formLayout);
    rightLayout->addLayout(codeLayout);
    rightLayout->addStretch();
    setStyleSheet("background-color: rgba(255,255,255,200);");
}

QSchoolInfoWidget::~QSchoolInfoWidget()
{}

