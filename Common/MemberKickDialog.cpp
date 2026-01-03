#pragma execution_character_set("utf-8")
#include "MemberKickDialog.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QScrollArea>
#include <QWidget>
#include <QPointer>
#include <QMetaObject>
#include <QMouseEvent>
#include <QEvent>

#include "ImSDK/includes/TIMCloud.h"
#include "ImSDK/includes/TIMCloudDef.h"

MemberKickDialog::MemberKickDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("踢出群成员");
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(400, 500);
    
    m_httpHandler = new TAHttpHandler(this);

    // 创建主容器
    QWidget* container = new QWidget(this);
    container->setStyleSheet(
        "QWidget { background-color: #282A2B; border: 1px solid #282A2B; border-radius: 8px; }"
    );

    QVBoxLayout* containerLayout = new QVBoxLayout(this);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);
    containerLayout->addWidget(container);

    // 标题栏
    QWidget* titleBar = new QWidget(container);
    titleBar->setFixedHeight(40);
    titleBar->setStyleSheet("background-color: #282A2B; border-top-left-radius: 8px; border-top-right-radius: 8px;");

    QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(15, 0, 15, 0);
    titleLayout->setSpacing(10);

    // 关闭按钮（初始隐藏）
    QPushButton* btnClose = new QPushButton("×", titleBar);
    btnClose->setFixedSize(24, 24);
    btnClose->setStyleSheet(
        "QPushButton {"
        "border: none;"
        "color: #ffffff;"
        "background: rgba(255,255,255,0.12);"
        "border-radius: 12px;"
        "font-weight: bold;"
        "font-size: 16px;"
        "}"
        "QPushButton:hover {"
        "background: rgba(255,0,0,0.35);"
        "}"
    );
    btnClose->setVisible(false);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::reject);

    // 标题文本
    QLabel* lblTitle = new QLabel("群成员列表", titleBar);
    lblTitle->setStyleSheet("color: #ffffff; font-size: 14px; font-weight: bold; background: transparent; border: none;");
    lblTitle->setAlignment(Qt::AlignCenter);

    titleLayout->addWidget(lblTitle, 1);
    titleLayout->addWidget(btnClose);

    // 内容区域
    QWidget* contentWidget = new QWidget(container);
    contentWidget->setStyleSheet("background-color: #282A2B; border-bottom-left-radius: 8px; border-bottom-right-radius: 8px;");

    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setContentsMargins(20, 15, 20, 15);
    mainLayout->setSpacing(10);

    // 滚动区域
    m_scrollArea = new QScrollArea(contentWidget);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setStyleSheet("background-color:transparent; border:none;");

    m_scrollWidget = new QWidget;
    m_membersLayout = new QVBoxLayout(m_scrollWidget);
    m_membersLayout->setSpacing(8);
    m_membersLayout->setContentsMargins(5, 5, 5, 5);
    m_membersLayout->addStretch();

    m_scrollArea->setWidget(m_scrollWidget);
    mainLayout->addWidget(m_scrollArea);

    // 底部按钮
    QHBoxLayout* bottomLayout = new QHBoxLayout;
    bottomLayout->setSpacing(10);
    QPushButton* btnCancel = new QPushButton("取消", contentWidget);
    QPushButton* btnOk = new QPushButton("确定", contentWidget);
    btnCancel->setFixedSize(80, 35);
    btnCancel->setStyleSheet(
        "QPushButton {"
        "background-color: #555555;"
        "color: #ffffff;"
        "border: none;"
        "border-radius: 6px;"
        "font-size: 13px;"
        "font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "background-color: #666666;"
        "}"
        "QPushButton:pressed {"
        "background-color: #444444;"
        "}"
    );
    btnOk->setFixedSize(80, 35);
    btnOk->setStyleSheet(
        "QPushButton {"
        "background-color: #4169E1;"
        "color: #ffffff;"
        "border: none;"
        "border-radius: 6px;"
        "font-size: 13px;"
        "font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "background-color: #5B7FD8;"
        "}"
        "QPushButton:pressed {"
        "background-color: #3357C7;"
        "}"
    );
    bottomLayout->addStretch();
    bottomLayout->addWidget(btnCancel);
    bottomLayout->addWidget(btnOk);
    mainLayout->addLayout(bottomLayout);

    // 主布局
    QVBoxLayout* containerMainLayout = new QVBoxLayout(container);
    containerMainLayout->setContentsMargins(0, 0, 0, 0);
    containerMainLayout->setSpacing(0);
    containerMainLayout->addWidget(titleBar);
    containerMainLayout->addWidget(contentWidget, 1);

    // 鼠标移入显示关闭按钮，移出隐藏
    class CloseButtonEventFilter : public QObject {
    public:
        CloseButtonEventFilter(QDialog* dlg, QPushButton* closeBtn)
            : QObject(dlg), m_dlg(dlg), m_closeBtn(closeBtn) {}
        
        bool eventFilter(QObject* obj, QEvent* event) override {
            if (obj == m_dlg) {
                if (event->type() == QEvent::Enter) {
                    m_closeBtn->setVisible(true);
                    return true;
                } else if (event->type() == QEvent::Leave) {
                    m_closeBtn->setVisible(false);
                    return true;
                }
            }
            return QObject::eventFilter(obj, event);
        }
    private:
        QDialog* m_dlg;
        QPushButton* m_closeBtn;
    };
    
    CloseButtonEventFilter* filter = new CloseButtonEventFilter(this, btnClose);
    installEventFilter(filter);

    // 拖拽功能
    class DragEventFilter : public QObject {
    public:
        DragEventFilter(QDialog* dlg, QWidget* titleBar)
            : QObject(dlg), m_dlg(dlg), m_titleBar(titleBar), m_dragging(false) {}
        
        bool eventFilter(QObject* obj, QEvent* event) override {
            if (obj == m_dlg) {
                if (event->type() == QEvent::MouseButtonPress) {
                    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                    if (mouseEvent->button() == Qt::LeftButton) {
                        QPoint pos = m_dlg->mapFromGlobal(mouseEvent->globalPos());
                        if (m_titleBar->geometry().contains(pos)) {
                            m_dragging = true;
                            m_dragStartPos = mouseEvent->globalPos() - m_dlg->frameGeometry().topLeft();
                            return true;
                        }
                    }
                } else if (event->type() == QEvent::MouseMove) {
                    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                    if (m_dragging && (mouseEvent->buttons() & Qt::LeftButton)) {
                        m_dlg->move(mouseEvent->globalPos() - m_dragStartPos);
                        return true;
                    }
                } else if (event->type() == QEvent::MouseButtonRelease) {
                    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                    if (mouseEvent->button() == Qt::LeftButton) {
                        m_dragging = false;
                    }
                }
            }
            return QObject::eventFilter(obj, event);
        }
    private:
        QDialog* m_dlg;
        QWidget* m_titleBar;
        QPoint m_dragStartPos;
        bool m_dragging;
    };
    
    DragEventFilter* dragFilter = new DragEventFilter(this, titleBar);
    installEventFilter(dragFilter);

    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnOk, &QPushButton::clicked, this, &MemberKickDialog::onOkClicked);
}

MemberKickDialog::~MemberKickDialog()
{}

void MemberKickDialog::setUseTencentSDK(bool useTencentSDK)
{
    m_useTencentSDK = useTencentSDK;
}

void MemberKickDialog::InitData(const QVector<GroupMemberInfo>& memberList)
{
    clearLayout(m_membersLayout);
    
    UserInfo userInfo = CommonInfo::GetData();
    QString currentUserId = userInfo.teacher_unique_id;
    
    // 遍历成员列表，排除群主（角色为"群主"的成员）
    for (const auto& member : memberList)
    {
        // 排除群主
        if (member.member_role == "群主")
        {
            continue;
        }
        
        // 创建复选框
        QCheckBox* checkbox = new QCheckBox(member.member_name, m_scrollWidget);
        checkbox->setProperty("member_id", member.member_id);
        checkbox->setStyleSheet("color:white; padding:4px;");
        
        // 存储成员信息
        m_memberInfoMap[member.member_id] = member.member_name;
        
        m_checkBoxes.append(checkbox);
        m_membersLayout->insertWidget(m_membersLayout->count() - 1, checkbox); // 插入到stretch之前
    }
}

void MemberKickDialog::setGroupId(const QString& groupId)
{
    m_groupId = groupId;
}

void MemberKickDialog::setGroupName(const QString& groupName)
{
    m_groupName = groupName;
}

QVector<QString> MemberKickDialog::getSelectedMemberIds()
{
    QVector<QString> selectedIds;
    for (QCheckBox* checkbox : m_checkBoxes)
    {
        if (checkbox && checkbox->isChecked())
        {
            QString memberId = checkbox->property("member_id").toString();
            if (!memberId.isEmpty())
            {
                selectedIds.append(memberId);
            }
        }
    }
    return selectedIds;
}

void MemberKickDialog::onOkClicked()
{
    // 获取选中的成员
    QVector<QString> selectedMemberIds;
    QVector<QString> selectedMemberNames;
    
    for (QCheckBox* checkbox : m_checkBoxes)
    {
        if (checkbox && checkbox->isChecked())
        {
            QString memberId = checkbox->property("member_id").toString();
            
            if (!memberId.isEmpty())
            {
                selectedMemberIds.append(memberId);
                
                // 获取名称
                if (m_memberInfoMap.contains(memberId))
                {
                    selectedMemberNames.append(m_memberInfoMap[memberId]);
                }
                else
                {
                    selectedMemberNames.append(memberId);
                }
            }
        }
    }
    
    if (selectedMemberIds.isEmpty())
    {
        CustomMessageBox::information(this, "提示", "请至少选择一个成员");
        return;
    }
    
    if (m_groupId.isEmpty())
    {
        CustomMessageBox::warning(this, "错误", "群组ID未设置");
        return;
    }
    
    // 普通群：踢人走腾讯 SDK；班级群：沿用原有服务器接口
    if (m_useTencentSDK) {
        QJsonObject payload;
        payload[QString::fromUtf8(kTIMGroupDeleteMemberParamGroupId)] = m_groupId;

        QJsonArray idArray;
        for (const auto& id : selectedMemberIds) {
            idArray.append(id);
        }
        payload[QString::fromUtf8(kTIMGroupDeleteMemberParamIdentifierArray)] = idArray;
        payload[QString::fromUtf8(kTIMGroupDeleteMemberParamUserData)] = QStringLiteral("ta_kick");

        const QByteArray jsonData = QJsonDocument(payload).toJson(QJsonDocument::Compact);

        struct KickCbData {
            QPointer<MemberKickDialog> dlg;
            QString groupId;
        };
        KickCbData* cbData = new KickCbData;
        cbData->dlg = this;
        cbData->groupId = m_groupId;

        int callRet = TIMGroupDeleteMember(jsonData.constData(),
            [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
                KickCbData* d = (KickCbData*)user_data;
                if (!d) return;
                if (!d->dlg) { delete d; return; }

                const QPointer<MemberKickDialog> dlg = d->dlg;
                const QString groupId = d->groupId;
                const QString errDesc = QString::fromUtf8(desc ? desc : "");
                const QByteArray payload = QByteArray(json_params ? json_params : "");

                if (dlg) {
                    QMetaObject::invokeMethod(dlg, [dlg, groupId, code, errDesc, payload]() {
                        if (!dlg) return;
                        if (code != 0) {
                            CustomMessageBox::warning(dlg, QString::fromUtf8(u8"踢出失败"),
                                QString("腾讯SDK踢出失败\n错误码: %1\n错误描述: %2").arg(code).arg(errDesc));
                            return;
                        }

                        int suc = 0, included = 0, invited = 0, failed = 0;
                        QStringList failedIds;

                        QJsonParseError pe;
                        QJsonDocument doc = QJsonDocument::fromJson(payload, &pe);
                        if (pe.error == QJsonParseError::NoError && doc.isArray()) {
                            const QJsonArray arr = doc.array();
                            for (const auto& v : arr) {
                                if (!v.isObject()) continue;
                                const QJsonObject o = v.toObject();
                                const QString id = o.value(QString::fromUtf8(kTIMGroupDeleteMemberResultIdentifier)).toString();
                                const int r = o.value(QString::fromUtf8(kTIMGroupDeleteMemberResultResult)).toInt(0);
                                if (r == (int)kTIMGroupMember_HandledSuc) suc++;
                                else if (r == (int)kTIMGroupMember_Included) included++; // 对 delete 来说通常不会出现
                                else if (r == (int)kTIMGroupMember_Invited) invited++;   // 对 delete 来说通常不会出现
                                else { failed++; if (!id.isEmpty()) failedIds << id; }
                            }
                        }

                        QString msg = QString::fromUtf8(u8"踢出已提交。\n");
                        msg += QString("成功: %1，失败: %2").arg(suc).arg(failed);
                        if (!failedIds.isEmpty()) {
                            msg += QString::fromUtf8(u8"\n失败成员: ") + failedIds.mid(0, 8).join(", ");
                            if (failedIds.size() > 8) msg += "...";
                        }

                        CustomMessageBox::information(dlg, QString::fromUtf8(u8"踢出结果"), msg);

                        if (suc > 0) {
                            emit dlg->membersKickedSuccess(groupId);
                            dlg->accept();
                        }
                    }, Qt::QueuedConnection);
                }

                delete d;
            },
            cbData);

        if (callRet != TIM_SUCC) {
            delete cbData;
            CustomMessageBox::warning(this, QString::fromUtf8(u8"踢出失败"),
                QString("TIMGroupDeleteMember 调用失败，错误码: %1").arg(callRet));
        }
        return;
    }

    if (!m_httpHandler) {
        CustomMessageBox::critical(this, "错误", "HTTP处理器未初始化！");
        return;
    }
    
    // 直接调用服务器接口踢出成员
    QString url = "http://47.100.126.194:5000/groups/remove-member";
    
    // 构造成员ID数组
    QJsonArray membersArray;
    for (const QString& memberId : selectedMemberIds)
    {
        membersArray.append(memberId);
    }
    
    // 构造请求JSON
    QJsonObject payload;
    payload["group_id"] = m_groupId;
    payload["members"] = membersArray;
    
    // 转换为JSON字符串
    QJsonDocument doc(payload);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    qDebug() << "========== 踢出成员 - 服务器接口 ==========";
    qDebug() << "群组ID:" << m_groupId;
    qDebug() << "成员数量:" << membersArray.size();
    qDebug() << "请求JSON:" << QString::fromUtf8(jsonData);
    
    // 创建HTTP处理器用于接收响应
    TAHttpHandler* kickHandler = new TAHttpHandler(this);
    connect(kickHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
        qDebug() << "踢出成员服务器响应:" << responseString;
        
        // 解析响应
        QJsonDocument respDoc = QJsonDocument::fromJson(responseString.toUtf8());
        bool success = false;
        QString message = QString::fromUtf8(u8"成员已成功踢出群组");
        
        if (respDoc.isObject()) {
            QJsonObject obj = respDoc.object();
            if (obj.contains("code")) {
                int code = obj.value("code").toInt(-1);
                success = (code == 0 || code == 200);
            }
            if (obj.contains("message") && obj.value("message").isString()) {
                message = obj.value("message").toString();
            }
        } else {
            // 非JSON响应但HTTP成功，视为成功
            success = true;
        }
        
        if (success) {
            CustomMessageBox::information(this, QString::fromUtf8(u8"踢出成功"), message);
            emit membersKickedSuccess(m_groupId); // 发出信号通知刷新成员列表
            accept(); // 关闭对话框
        } else {
            CustomMessageBox::critical(this, QString::fromUtf8(u8"踢出失败"), message);
        }
        
        kickHandler->deleteLater();
    });
    
    connect(kickHandler, &TAHttpHandler::failed, this, [=](const QString& errResponseString) {
        qDebug() << "踢出成员服务器错误:" << errResponseString;
        
        QString errorMsg = QString::fromUtf8(u8"踢出成员失败");
        QJsonDocument errDoc = QJsonDocument::fromJson(errResponseString.toUtf8());
        if (errDoc.isObject()) {
            QJsonObject errObj = errDoc.object();
            if (errObj.contains("message") && errObj.value("message").isString()) {
                errorMsg = errObj.value("message").toString();
            }
        }
        
        CustomMessageBox::critical(this, QString::fromUtf8(u8"踢出失败"), errorMsg);
        kickHandler->deleteLater();
    });
    
    // 发送POST请求
    kickHandler->post(url, jsonData);
}

void MemberKickDialog::clearLayout(QVBoxLayout* layout)
{
    if (!layout) return;
    
    // 清空复选框列表
    for (QCheckBox* checkbox : m_checkBoxes)
    {
        if (checkbox)
        {
            checkbox->deleteLater();
        }
    }
    m_checkBoxes.clear();
    m_memberInfoMap.clear(); // 清空成员信息映射

    QLayoutItem* child;
    while ((child = layout->takeAt(0)) != nullptr)
    {
        if (child->widget() && child->widget() != m_scrollWidget)
        {
            child->widget()->deleteLater();
        }
        delete child;
    }
    
    // 重新添加stretch
    layout->addStretch();
}

