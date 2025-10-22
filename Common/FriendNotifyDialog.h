#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>

class FriendNotifyDialog : public QDialog
{
    Q_OBJECT
public:
    FriendNotifyDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("好友通知");
        resize(750, 700);
        setStyleSheet("background-color: #f5f5f5; font-size: 14px;");

        mainLayout = new QVBoxLayout(this);

        // ===== 顶部标题栏 =====
        QHBoxLayout* titleLayout = new QHBoxLayout;
        QLabel* lblTitle = new QLabel("好友通知");
        lblTitle->setStyleSheet("font-weight: bold; font-size: 20px;");
        QPushButton* btnFilter = new QPushButton("🔍");   // 这里可以换成图标
        QPushButton* btnDelete = new QPushButton("🗑");
        btnFilter->setFixedSize(30, 30);
        btnDelete->setFixedSize(30, 30);
        btnFilter->setFlat(true);
        btnDelete->setFlat(true);
        titleLayout->addWidget(lblTitle);
        titleLayout->addStretch();
        titleLayout->addWidget(btnFilter);
        titleLayout->addWidget(btnDelete);
        mainLayout->addLayout(titleLayout);
    }

    void InitData(QVector<QString> vecMsg = QVector<QString>())
    {
        if (m_bInit == true)
        {
            return;
        }

        // ===== 滚动区域 =====
        scroll = new QScrollArea;
        scroll->setWidgetResizable(true);
        container = new QWidget;
        listLayout = new QVBoxLayout(container);
        listLayout->setSpacing(10);
        for (int i = 0; i < vecMsg.size(); i++)
        {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(vecMsg[i].toUtf8(), &parseError);

            if (parseError.error != QJsonParseError::NoError) {
                qDebug() << "JSON parse error:" << parseError.errorString();
            }
            else {
                if (doc.isObject()) {
                    QJsonObject obj = doc.object();
                    if (obj["data"].isArray())
                    {
                        // 4. 取出数组
                        QJsonArray arr = obj["data"].toArray();
                        // 5. 遍历数组
                        for (const QJsonValue& value : arr) {
                            if (value.isObject()) { // 每个元素是一个对象
                                QJsonObject obj = value.toObject();
                                int qSender_id = obj["sender_id"].toInt();
                                QString strSender_id = QString("%1").arg(qSender_id, 6, 10, QChar('0'));
                                QString SenderName = obj["sender_name"].toString();
                                int iContent_text = obj["content_text"].toInt();
                                QString updated_at = obj["updated_at"].toString();
                                int is_agreed = obj["is_agreed"].toInt();
                                QString msgContent;
                                if (1 == iContent_text)
                                {
                                    msgContent = "请求加为好友";
                                }
                                if (1 == iContent_text || 2 == iContent_text)
                                {
                                    addItem(listLayout, "头像", SenderName, msgContent, updated_at,
                                        "", is_agreed);
                                }
                            }
                        }
                    }
                }
            }
            
            // 添加示例条目
            /*addItem(listLayout, "头像", "英语-周老师", "请求加为好友", "2025/09/09",
                "我是来自群聊“集鑫中学2025级3班”的英语-周老师", "已同意");
            addItem(listLayout, "头像", "零声教育【Mark老师】", "请求加为好友", "2025/07/13",
                "我是来自群聊“C++音视频服务器开发交流群”的零声教育", "已同意");
            addItem(listLayout, "头像", "廖深意", "请求加为好友", "2024/11/13",
                "我是来自群聊“驱动交流群”的", "已同意");
            addItem(listLayout, "头像", "a", "请求加为好友", "2024/11/13",
                "我是来自群聊“驱动交流群”的", "已同意");
            addItem(listLayout, "头像", "世纪无成", "请求加为好友", "2024/11/13",
                "我是来自群聊“WINDOWS驱动开发”的世纪无成", "已同意");
            addItem(listLayout, "头像", "Miky", "请求加为好友", "2024/11/02",
                "我是Miky Sunny，APP，小程序", "已同意");*/
        }
        listLayout->addStretch();
        scroll->setWidget(container);
        mainLayout->addWidget(scroll);
        m_bInit = true;
    }

private:
    void addItem(QVBoxLayout* parent,
        const QString& avatarText,
        const QString& name,
        const QString& action,
        const QString& date,
        const QString& remark,
        const int& is_agreed)
    {
        QFrame* itemFrame = new QFrame;
        itemFrame->setStyleSheet("background-color: white; border-radius: 8px;");
        QVBoxLayout* outerLayout = new QVBoxLayout(itemFrame);

        // 第一行：头像 + 名称 + 请求 + 日期
        QHBoxLayout* topLayout = new QHBoxLayout;
        QLabel* avatar = new QLabel(avatarText);
        avatar->setFixedSize(50, 50);
        avatar->setStyleSheet("background-color: lightgray; border-radius: 25px; text-align:center;");
        QLabel* lblName = new QLabel(QString("<b style='color:#0055cc'>%1</b> %2").arg(name, action));
        QLabel* lblDate = new QLabel(date);
        lblDate->setStyleSheet("color: gray;");
        topLayout->addWidget(avatar);
        topLayout->addWidget(lblName);
        topLayout->addStretch();
        topLayout->addWidget(lblDate);
        outerLayout->addLayout(topLayout);

        // 备注信息
        QLabel* lblRemark = new QLabel("留言: " + remark);
        lblRemark->setWordWrap(true);
        lblRemark->setStyleSheet("color: gray;");
        outerLayout->addWidget(lblRemark);

        if (1 == is_agreed)
        {
            // 状态
            QLabel* lblStatus = new QLabel("已同意");
            lblStatus->setStyleSheet("color: gray;");
            outerLayout->addWidget(lblStatus, 0, Qt::AlignRight);
        }
        else
        {
            // 状态
            QPushButton* lblStatusBtn = new QPushButton("同意");
            //veclblStatusBtn.push_back(lblStatusBtn);
            lblStatusBtn->setStyleSheet(
                "QPushButton {"
                "    color: gray;"                  /* 默认文字颜色 */
                "    border: 1px solid gray;"        /* 默认边框 */
                "    border-radius: 6px;"            /* 圆角半径 */
                "    padding: 6px 12px;"             /* 内边距: 上下6px, 左右12px */
                "    background-color: white;"       /* 默认背景色 */
                "}"
                "QPushButton:hover {"
                "    color: black;"                  /* 悬停时文字变黑 */
                "    border: 1px solid #409EFF;"      /* 悬停时边框变蓝 */
                "}"
                "QPushButton:pressed {"
                "    background-color: #f2f2f2;"     /* 点击时浅灰背景 */
                "    border: 1px solid #66b1ff;"      /* 点击时浅蓝边框 */
                "}"
            );
            outerLayout->addWidget(lblStatusBtn, 0, Qt::AlignRight);

            //QLabel* lblStatus = new QLabel("已同意");
            //lblStatus->setStyleSheet("color: gray;");
            //outerLayout->addWidget(lblStatus, 0, Qt::AlignRight);
            //lblStatus->hide();
            QObject::connect(lblStatusBtn, &QPushButton::clicked, [lblStatusBtn, outerLayout]() {
                //lblStatus->show();
                //lblStatusBtn->hide();
                // 1. 从布局中移除按钮
                //outerLayout->removeWidget(lblStatusBtn);
                // 2. 删除 QPushButton（可选，如果不想复用）
                //lblStatusBtn->deleteLater();

                // 3. 创建新的 QLabel
                QLabel* label = new QLabel("已同意");
                label->setStyleSheet("color: green; font-weight: bold;"); // 样式可选

                // 4. 添加到布局
                outerLayout->addWidget(label);
                outerLayout->replaceWidget(lblStatusBtn, label);
                lblStatusBtn->deleteLater();  // 这里更安全，因为布局不会再持有按钮
                });
        }
        parent->addWidget(itemFrame);
    }
    QVBoxLayout* mainLayout = NULL;
    QScrollArea* scroll = NULL;
    QWidget* container = NULL;
    QVBoxLayout* listLayout = NULL;
    bool m_bInit = false;
};
