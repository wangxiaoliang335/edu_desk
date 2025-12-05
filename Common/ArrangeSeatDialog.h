#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QButtonGroup>
#include <QFrame>
#include <QBrush>
#include <QColor>

class ArrangeSeatDialog : public QDialog
{
    Q_OBJECT

public:
    ArrangeSeatDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("排座");
        resize(400, 350);
        setStyleSheet("background-color: #e0f0ff; font-size:14px;");

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(15);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        // 顶部：两个列表框并排
        QHBoxLayout* listLayout = new QHBoxLayout;
        listLayout->setSpacing(20);

        // 左侧下拉框：小组/不分组
        QFrame* leftFrame = new QFrame;
        leftFrame->setFrameShape(QFrame::StyledPanel);
        leftFrame->setStyleSheet("background-color: #e0f0ff; border: 2px solid #87ceeb;");
        QVBoxLayout* leftLayout = new QVBoxLayout(leftFrame);
        leftLayout->setContentsMargins(5, 5, 5, 5);

        leftComboBox = new QComboBox;
        leftComboBox->setEditable(false); // 设置为不可编辑，确保显示选中的项
        leftComboBox->setStyleSheet(
            "QComboBox { background-color: white; border: 1px solid #ccc; padding: 8px; min-width: 120px; color: black; }"
            "QComboBox::drop-down { border: none; }"
            "QComboBox QAbstractItemView { background-color: white; selection-background-color: #ff8c00; color: black; }"
            "QComboBox QAbstractItemView::item { padding: 8px; color: black; }"
            "QComboBox QAbstractItemView::item:selected { background-color: #ff8c00; color: white; }"
        );

        // 添加左侧选项
        leftComboBox->addItem("小组");
        leftComboBox->addItem("不分组");

        // 默认选择"不分组"（索引1）
        leftComboBox->setCurrentIndex(1);
        leftComboBox->update(); // 强制更新显示

        leftLayout->addWidget(leftComboBox);
        listLayout->addWidget(leftFrame);

        // 右侧下拉框：根据左侧选择动态变化
        QFrame* rightFrame = new QFrame;
        rightFrame->setFrameShape(QFrame::StyledPanel);
        rightFrame->setStyleSheet("background-color: #e0f0ff; border: 2px solid #87ceeb;");
        QVBoxLayout* rightLayout = new QVBoxLayout(rightFrame);
        rightLayout->setContentsMargins(5, 5, 5, 5);

        rightComboBox = new QComboBox;
        rightComboBox->setEditable(false); // 设置为不可编辑，确保显示选中的项
        rightComboBox->setStyleSheet(
            "QComboBox { background-color: white; border: 1px solid #ccc; padding: 8px; min-width: 120px; color: black; }"
            "QComboBox::drop-down { border: none; }"
            "QComboBox QAbstractItemView { background-color: white; selection-background-color: #ff8c00; color: black; }"
            "QComboBox QAbstractItemView::item { padding: 8px; color: black; }"
            "QComboBox QAbstractItemView::item:selected { background-color: #ff8c00; color: white; }"
        );

        // 初始化右侧下拉框（默认显示"不分组"的选项）
        updateRightList(false);
        rightComboBox->update(); // 强制更新显示

        rightLayout->addWidget(rightComboBox);
        listLayout->addWidget(rightFrame);

        mainLayout->addLayout(listLayout);

        // 连接左侧下拉框选择变化事件
        connect(leftComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
            QString text = leftComboBox->itemText(index);
            bool isGroup = (text == "小组");

            // 更新右侧下拉框
            updateRightList(isGroup);
        });

        // 底部按钮
        QHBoxLayout* buttonLayout = new QHBoxLayout;
        buttonLayout->setSpacing(10);

        QString grayStyle = "background-color: #808080; color: white; padding: 6px 12px; border-radius: 4px; font-size: 14px;";
        QString greenStyle = "background-color: green; color: white; padding: 6px 12px; border-radius: 4px; font-size: 14px;";

        btnMidtermGrade = new QPushButton("期中成绩单");
        btnSubject = new QPushButton("数学");
        btnConfirm = new QPushButton("确定");

        btnMidtermGrade->setStyleSheet(grayStyle);
        btnSubject->setStyleSheet(grayStyle);
        btnConfirm->setStyleSheet(greenStyle);

        buttonLayout->addWidget(btnMidtermGrade);
        buttonLayout->addWidget(btnSubject);
        buttonLayout->addStretch();
        buttonLayout->addWidget(btnConfirm);

        mainLayout->addLayout(buttonLayout);

        // 连接确定按钮
        connect(btnConfirm, &QPushButton::clicked, this, &QDialog::accept);
    }

    // 获取选中的左侧选项（true=小组, false=不分组）
    bool isGroupMode() const
    {
        return leftComboBox->currentText() == "小组";
    }

    // 获取选中的右侧选项文本
    QString getSelectedMethod() const
    {
        return rightComboBox->currentText();
    }

private:
    void updateRightList(bool isGroup)
    {
        rightComboBox->clear();

        if (isGroup) {
            // 小组模式：2人组、4人组、6人组
            rightComboBox->addItem("2人组排座");
            rightComboBox->addItem("4人组排座");
            rightComboBox->addItem("6人组排座");
            // 默认选择第一个（2人组排座）
            rightComboBox->setCurrentIndex(0);
        } else {
            // 不分组模式：随机排座、倒序、正序
            rightComboBox->addItem("随机排座");
            rightComboBox->addItem("倒序");
            rightComboBox->addItem("正序");
            // 默认选择最后一个（正序）
            rightComboBox->setCurrentIndex(2);
        }
    }

private:
    QComboBox* leftComboBox;
    QComboBox* rightComboBox;
    QPushButton* btnMidtermGrade;
    QPushButton* btnSubject;
    QPushButton* btnConfirm;
};

