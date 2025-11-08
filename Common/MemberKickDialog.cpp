#pragma execution_character_set("utf-8")
#include "MemberKickDialog.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QScrollArea>
#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDateTime>

MemberKickDialog::MemberKickDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("踢出群成员");
    resize(400, 500);
    setStyleSheet("background-color:#555555; font-size:14px;");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // 标题
    QLabel* lblTitle = new QLabel("群成员列表");
    lblTitle->setStyleSheet("background-color:#3b73b8; qproperty-alignment: AlignCenter; color:white; font-weight:bold; padding:6px;");
    mainLayout->addWidget(lblTitle);

    // 滚动区域
    m_scrollArea = new QScrollArea(this);
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
    QPushButton* btnCancel = new QPushButton("取消");
    QPushButton* btnOk = new QPushButton("确定");
    btnCancel->setStyleSheet("background-color:green; color:white; padding:6px; border-radius:4px;");
    btnOk->setStyleSheet("background-color:green; color:white; padding:6px; border-radius:4px;");
    bottomLayout->addStretch();
    bottomLayout->addWidget(btnCancel);
    bottomLayout->addWidget(btnOk);
    mainLayout->addLayout(bottomLayout);

    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnOk, &QPushButton::clicked, this, &MemberKickDialog::onOkClicked);
}

MemberKickDialog::~MemberKickDialog()
{}

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

// 将TIMResult错误码转换为错误描述字符串
static QString getTIMResultErrorString(int errorCode) {
    switch (errorCode) {
        case TIM_SUCC:
            return "成功";
        case TIM_ERR_SDKUNINIT:
            return "ImSDK未初始化";
        case TIM_ERR_NOTLOGIN:
            return "用户未登录";
        case TIM_ERR_JSON:
            return "错误的Json格式或Json Key - 请检查JSON参数格式是否正确，特别是字段名称和数据类型";
        case TIM_ERR_PARAM:
            return "参数错误 - 请检查传入的参数是否有效";
        case TIM_ERR_CONV:
            return "无效的会话";
        case TIM_ERR_GROUP:
            return "无效的群组";
        default:
            return QString("未知错误码: %1").arg(errorCode);
    }
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
        QMessageBox::information(this, "提示", "请至少选择一个成员");
        return;
    }
    
    if (m_groupId.isEmpty())
    {
        QMessageBox::warning(this, "错误", "群组ID未设置");
        return;
    }
    
    // 调用腾讯SDK踢出成员接口
    QJsonObject deleteMemberParam;
    deleteMemberParam[kTIMGroupDeleteMemberParamGroupId] = m_groupId;
    QJsonArray identifierArray;
    for (const QString& memberId : selectedMemberIds)
    {
        identifierArray.append(memberId);
    }
    deleteMemberParam[kTIMGroupDeleteMemberParamIdentifierArray] = identifierArray;
    deleteMemberParam[kTIMGroupDeleteMemberParamUserData] = ""; // 添加user_data字段，即使为空字符串
    
    QJsonDocument doc(deleteMemberParam);
    
    // 验证JSON是否有效
    if (doc.isNull() || doc.isEmpty()) {
        QMessageBox::critical(this, "错误", "生成的JSON参数为空或无效！");
        return;
    }
    
    // 生成JSON字符串
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 验证JSON字符串
    QJsonParseError parseError;
    QJsonDocument verifyDoc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        QString errorMsg = QString("JSON格式验证失败: %1\n位置: %2").arg(parseError.errorString()).arg(parseError.offset);
        qDebug() << errorMsg;
        QMessageBox::critical(this, "JSON格式错误", errorMsg);
        return;
    }
    
    // 输出格式化的JSON用于调试
    QByteArray formattedJson = doc.toJson(QJsonDocument::Indented);
    qDebug() << "========== 踢出成员参数（格式化）==========";
    qDebug() << QString::fromUtf8(formattedJson);
    qDebug() << "========== 踢出成员参数（紧凑格式）==========";
    qDebug() << QString::fromUtf8(jsonData);
    
    // 创建回调数据结构
    struct KickCallbackData {
        MemberKickDialog* dlg;
        QVector<QString> memberIds;
        QVector<QString> memberNames;
        QString groupId;
    };
    
    KickCallbackData* callbackData = new KickCallbackData;
    callbackData->dlg = this;
    callbackData->memberIds = selectedMemberIds;
    callbackData->memberNames = selectedMemberNames;
    callbackData->groupId = m_groupId;
    
    // 确保JSON字符串是null-terminated
    QByteArray jsonDataWithNull = jsonData;
    if (jsonDataWithNull.isEmpty() || jsonDataWithNull.at(jsonDataWithNull.length() - 1) != '\0') {
        jsonDataWithNull.append('\0');
    }
    
    const char* jsonCStr = jsonDataWithNull.constData();
    
    qDebug() << "========== 传递给TIMGroupDeleteMember的JSON字符串 ==========";
    qDebug() << jsonCStr;
    
    int ret = TIMGroupDeleteMember(jsonCStr,
        [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
            KickCallbackData* data = (KickCallbackData*)user_data;
            if (!data || !data->dlg) {
                delete data;
                return;
            }
            
            if (ERR_SUCC != code) {
                QString errorDesc = QString::fromUtf8(desc ? desc : "未知错误");
                QString errorMsg;
                
                // 特殊处理常见的错误码
                if (code == 10007) {
                    errorMsg = QString("踢出成员失败\n错误码: %1\n错误描述: %2\n\n操作权限不足，只有群主或管理员可以踢出成员。").arg(code).arg(errorDesc);
                } else {
                    errorMsg = QString("踢出成员失败\n错误码: %1\n错误描述: %2").arg(code).arg(errorDesc);
                }
                
                qDebug() << errorMsg;
                QMessageBox::critical(data->dlg, "踢出失败", errorMsg);
                delete data;
                return;
            }
            
            // 踢出成功，解析返回结果
            if (json_params) {
                QJsonParseError parseError;
                QJsonDocument respDoc = QJsonDocument::fromJson(QString::fromUtf8(json_params).toUtf8(), &parseError);
                if (parseError.error == QJsonParseError::NoError && respDoc.isArray()) {
                    QJsonArray resultArray = respDoc.array();
                    qDebug() << "踢出成员成功，返回结果数量:" << resultArray.size();
                    
                    // 调用服务器API移除成员
                    data->dlg->kickMembersFromServer(data->memberIds, data->memberNames);
                    
                    QMessageBox::information(data->dlg, "踢出成功", "成员已成功踢出群组");
                    data->dlg->accept(); // 关闭对话框
                }
            }
            
            delete data;
        }, callbackData);
    
    if (TIM_SUCC != ret) {
        QString errorDescStr = getTIMResultErrorString(ret);
        QString errorDesc = QString("调用TIMGroupDeleteMember接口失败\n错误码: %1\n错误描述: %2").arg(ret).arg(errorDescStr);
        qDebug() << errorDesc;
        QMessageBox::critical(this, "踢出失败", errorDesc);
        delete callbackData;
    }
}

void MemberKickDialog::kickMembersFromServer(const QVector<QString>& memberIds, const QVector<QString>& memberNames)
{
    if (memberIds.size() != memberNames.size()) {
        qWarning() << "成员ID和名称数量不匹配";
        return;
    }
    
    if (memberIds.isEmpty()) {
        // 如果没有成员需要移除，直接发出信号
        emit membersKickedSuccess(m_groupId);
        return;
    }
    
    QString url = "http://47.100.126.194:5000/groups/remove-member";
    
    // 使用共享指针来跟踪所有请求的状态
    struct RemoveState {
        int totalCount;
        int successCount;
        int failCount;
        MemberKickDialog* dlg;
        QString groupId;
    };
    
    RemoveState* state = new RemoveState;
    state->totalCount = memberIds.size();
    state->successCount = 0;
    state->failCount = 0;
    state->dlg = this;
    state->groupId = m_groupId;
    
    // 为每个被踢出的成员发送移除请求
    for (int i = 0; i < memberIds.size(); i++) {
        QString memberId = memberIds[i];
        QString memberName = memberNames[i];
        
        // 构造请求JSON
        QJsonObject requestData;
        requestData["group_id"] = m_groupId;
        requestData["user_id"] = memberId;
        
        // 转换为JSON字符串
        QJsonDocument doc(requestData);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        // 发送POST请求到服务器
        QNetworkAccessManager* manager = new QNetworkAccessManager(this);
        QNetworkRequest request;
        request.setUrl(QUrl(url));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QNetworkReply* reply = manager->post(request, jsonData);
        
        connect(reply, &QNetworkReply::finished, [=]() {
            if (reply->error() == QNetworkReply::NoError) {
                qDebug() << "从服务器移除成员成功，成员ID:" << memberId;
                state->successCount++;
            } else {
                qWarning() << "从服务器移除成员失败，成员ID:" << memberId << "错误:" << reply->errorString();
                state->failCount++;
            }
            
            // 检查是否所有请求都已完成
            if (state->successCount + state->failCount >= state->totalCount) {
                // 所有请求都已完成，发出信号通知刷新成员列表
                if (state->successCount > 0) {
                    qDebug() << "所有成员移除完成，成功:" << state->successCount << "失败:" << state->failCount;
                    emit state->dlg->membersKickedSuccess(state->groupId);
                }
                delete state;
            }
            
            reply->deleteLater();
            manager->deleteLater();
        });
    }
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

