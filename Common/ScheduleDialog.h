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
#include <qfiledialog.h>
#include <qdebug.h>
#include <qlineedit.h>
#include <QWidget>
#include <QObject>
#include <QMouseEvent>
#include "CustomListDialog.h"
#include "ClickableLabel.h"
#include "TAHttpHandler.h"
#include "ChatDialog.h"

class ClickableWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ClickableWidget(QWidget* parent = nullptr) : QWidget(parent) {}

signals:
    void clicked();  // 点击信号

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            emit clicked();
        }
        QWidget::mousePressEvent(event); // 可选：让父类继续处理事件
    }
};


class ScheduleDialog : public QDialog
{
    Q_OBJECT
public:
    ScheduleDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("课程表");
        resize(700, 500);
        setStyleSheet("QPushButton { font-size:14px; } QLabel { font-size:14px; }");
        m_taHttpHandler = new TAHttpHandler();

        customListDlg = new CustomListDialog(this);
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        m_chatDlg = new ChatDialog(this);

        // 顶部：头像 + 班级信息 + 功能按钮 + 更多
        QHBoxLayout* topLayout = new QHBoxLayout;
        ClickableLabel* lblAvatar = new ClickableLabel();
        lblAvatar->setFixedSize(50, 50);

        QPixmap avatarPixmap(".\\res\\img\\home.png");
        // 如果需要缩放到控件大小：
        avatarPixmap = avatarPixmap.scaled(lblAvatar->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        lblAvatar->setPixmap(avatarPixmap);
        lblAvatar->setScaledContents(true); // 自动适应 QLabel 尺寸

        lblAvatar->setStyleSheet("background-color: lightgray; border:1px solid gray; text-align:center;");
        connect(lblAvatar, &ClickableLabel::clicked, this, [&, lblAvatar]() {
            QString file = QFileDialog::getOpenFileName(
                this, "选择新头像", "", "Images (*.png *.jpg *.jpeg *.bmp)");
            if (!file.isEmpty()) {
                lblAvatar->setPixmap(QPixmap(file));
                uploadAvatar(file);
            }
        });

        m_lblClass = new QLabel("");
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
        topLayout->addWidget(m_lblClass);
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

        // 红框消息输入栏
        QHBoxLayout* inputLayout = new QHBoxLayout;

        QPushButton* btnVoice = new QPushButton("🔊");
        btnVoice->setFixedSize(30, 30);

        QLineEdit* editMessage = new QLineEdit();
        editMessage->setPlaceholderText("请输入消息...");
        editMessage->setMinimumHeight(30);
        editMessage->setEnabled(false);

        QPushButton* btnEmoji = new QPushButton("😊");
        btnEmoji->setFixedSize(30, 30);

        QPushButton* btnPlus = new QPushButton("➕");
        btnPlus->setFixedSize(30, 30);

        inputLayout->addStretch(1);
        inputLayout->addWidget(btnVoice);
        inputLayout->addWidget(editMessage, 1);
        inputLayout->addWidget(btnEmoji);
        inputLayout->addWidget(btnPlus);
        inputLayout->addStretch(1);

        ClickableWidget* inputWidget = new ClickableWidget();
        inputWidget->setLayout(inputLayout);
        inputWidget->setStyleSheet("background-color: white; border: 1px solid red;");

        // 绑定点击事件
        connect(inputWidget, &ClickableWidget::clicked, this, [=]() {
            qDebug() << "红框区域被点击！";
            // 这里可以弹出输入框、打开聊天功能等
            if (m_chatDlg)
            {
                m_chatDlg->exec();
            }
        });

        mainLayout->addWidget(inputWidget);

        // 黄色圆圈数字
        QLabel* lblNum = new QLabel("3");
        lblNum->setAlignment(Qt::AlignCenter);
        lblNum->setFixedSize(30, 30);
        lblNum->setStyleSheet("background-color: yellow; color: red; font-weight: bold; font-size: 16px; border-radius: 15px;");
        mainLayout->addWidget(lblNum, 0, Qt::AlignRight);

        //// 底部右下角黄色圆圈数字
        //QLabel* lblNum = new QLabel("3");
        //lblNum->setAlignment(Qt::AlignCenter);
        //lblNum->setFixedSize(30, 30);
        //lblNum->setStyleSheet("background-color: yellow; color: red; font-weight: bold; font-size: 16px; border-radius: 15px;");
        //mainLayout->addWidget(lblNum, 0, Qt::AlignRight);
    }

    void uploadAvatar(QString filePath)
    {
        // ===== 1. 读取头像图片 =====
        QFile file(filePath);  // 本地头像路径
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to open image file.";
            return;
        }
        QByteArray imageData = file.readAll(); // 二进制数据
        file.close();

        // ===== 2. 图片转 Base64 =====
        QString imageBase64 = QString::fromLatin1(imageData.toBase64());

        // ===== 3. 构造 JSON 数据 =====
        QMap<QString, QString> params;
        params["avatar"] = imageBase64;
        params["unique_group_id"] = m_unique_group_id;
        if (m_taHttpHandler)
        {
            m_taHttpHandler->post(QString("http://47.100.126.194:5000/updateGroupInfo"), params);
        }
    }

    void InitData(QString groupName, QString unique_group_id)
    {
        m_groupName = groupName;
        m_unique_group_id = unique_group_id;
        if (m_lblClass)
        {
            m_lblClass->setText(groupName);
        }
    }

private:
    CustomListDialog* customListDlg = NULL;
    QLabel* m_lblClass = NULL;
    QString m_groupName;
    QString m_unique_group_id;
    TAHttpHandler* m_taHttpHandler = NULL;
    ChatDialog* m_chatDlg = NULL;
};
