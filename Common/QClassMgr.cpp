#include "QClassMgr.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QCheckBox>

QClassMgr::QClassMgr(QWidget *parent)
	: QWidget(parent)
{ // 右侧内容布局
    QVBoxLayout* contentLayout = new QVBoxLayout(this);

    // 上方按钮组合 (学段、年级、班级数)
    QGridLayout* topGrid = new QGridLayout;
    QLabel* lblStage = new QLabel("学段");
    QLabel* lblGrade = new QLabel("年级");
    QLabel* lblClassCount = new QLabel("班级数");

    QPushButton* btnStage1 = new QPushButton("小学");
    QPushButton* btnStage2 = new QPushButton("初中");
    btnStage1->setStyleSheet("background-color:blue; color:white;");
    btnStage2->setStyleSheet("background-color:blue; color:white;");

    QPushButton* btnGrade1 = new QPushButton("1");
    QPushButton* btnGrade2 = new QPushButton("2");
    QPushButton* btnAddGrade = new QPushButton("+");
    btnGrade1->setStyleSheet("background-color:blue; color:white;");
    btnGrade2->setStyleSheet("background-color:blue; color:white;");
    btnAddGrade->setStyleSheet("background-color:green; color:white;");

    QPushButton* btnClassNum = new QPushButton("5");
    btnClassNum->setStyleSheet("background-color:blue; color:white;");

    // 布局放置
    topGrid->addWidget(lblStage, 0, 0);
    topGrid->addWidget(btnStage1, 0, 1);
    topGrid->addWidget(btnStage2, 0, 2);

    topGrid->addWidget(lblGrade, 1, 0);
    topGrid->addWidget(btnGrade1, 1, 1);
    topGrid->addWidget(btnGrade2, 1, 2);
    topGrid->addWidget(btnAddGrade, 1, 3);

    topGrid->addWidget(lblClassCount, 2, 0);
    topGrid->addWidget(btnClassNum, 2, 1);

    QPushButton* btnGenerate = new QPushButton("生成");
    btnGenerate->setStyleSheet("background-color:blue; color:white;");
    topGrid->addWidget(btnGenerate, 0, 4, 3, 1);

    contentLayout->addLayout(topGrid);

    // 添加、删除按钮
    QHBoxLayout* btnLayout = new QHBoxLayout;
    QPushButton* btnAdd = new QPushButton("添加");
    QPushButton* btnDelete = new QPushButton("删除");
    btnAdd->setStyleSheet("background-color:green; color:white; font-size:16px;");
    btnDelete->setStyleSheet("background-color:green; color:white; font-size:16px;");
    btnLayout->addWidget(btnAdd);
    btnLayout->addWidget(btnDelete);
    contentLayout->addLayout(btnLayout);

    // 表格
    QTableWidget* table = new QTableWidget(7, 5);
    table->setHorizontalHeaderLabels({ "学段","年级","班级","班级编号","备注" });

    // 第一列加复选框
    for (int row = 0; row < table->rowCount(); ++row) {
        QCheckBox* chk = new QCheckBox;
        table->setCellWidget(row, 0, chk);
    }
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    contentLayout->addWidget(table);

    // 将左菜单和右内容加到主布局
    //mainLayout->addLayout(menuLayout);
    //mainLayout->addLayout(contentLayout);
}

QClassMgr::~QClassMgr()
{}

