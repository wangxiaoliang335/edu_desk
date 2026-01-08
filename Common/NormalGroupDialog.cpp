#include "NormalGroupDialog.h"
#include "QGroupInfo.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QStyle>
#include <QDebug>

NormalGroupDialog::NormalGroupDialog(QWidget* parent)
    : QDialog(parent), m_dragging(false),
    m_backgroundColor(QColor(50, 50, 50)),
    m_borderColor(Qt::white),
    m_borderWidth(2), m_radius(6)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowTitle(QStringLiteral("创建普通群"));
    resize(400, 250);

    m_restAPI = new TIMRestAPI(this);

    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 50, 20, 20);

    // 创建标题栏
    QWidget* titleBar = new QWidget(this);
    titleBar->setFixedHeight(40);
    QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(10);

    // 关闭按钮
    m_closeButton = new QPushButton(titleBar);
    m_closeButton->setFixedSize(23, 23);
    m_closeButton->setStyleSheet(
        "QPushButton {"
        " background-color: transparent;"
        " border: none;"
        " color: white;"
        " font-size: 16px;"
        " font-weight: bold;"
        "}"
        "QPushButton:hover {"
        " background-color: rgba(255, 255, 255, 0.1);"
        " border-radius: 3px;"
        "}"
    );
    m_closeButton->setText("×");

    // 标题标签
    m_titleLabel = new QLabel(QStringLiteral("创建普通群"), titleBar);
    m_titleLabel->setStyleSheet("color: white; font-size: 16px; font-weight: bold; background-color: transparent;");

    titleLayout->addWidget(m_closeButton);
    titleLayout->addWidget(m_titleLabel);
    titleLayout->addStretch();

    // 内容区域
    QWidget* contentWidget = new QWidget(this);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(16);
    contentLayout->setContentsMargins(0, 0, 0, 0);

    // 群组名称输入
    QLabel* nameLabel = new QLabel(QStringLiteral("群组名称:"), contentWidget);
    nameLabel->setStyleSheet("color: white; font-size: 14px; background-color: transparent;");
    m_nameEdit = new QLineEdit(contentWidget);
    m_nameEdit->setPlaceholderText(QStringLiteral("请输入群组名称"));
    m_nameEdit->setStyleSheet(
        "QLineEdit {"
        " background-color: rgba(255, 255, 255, 0.1);"
        " border: 1px solid rgba(255, 255, 255, 0.2);"
        " border-radius: 4px;"
        " padding: 8px;"
        " color: white;"
        " font-size: 14px;"
        "}"
        "QLineEdit:focus {"
        " border: 1px solid rgba(255, 255, 255, 0.4);"
        "}"
    );

    // 按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(10);
    QPushButton* btnCreate = new QPushButton(QStringLiteral("创建"), contentWidget);
    QPushButton* btnCancel = new QPushButton(QStringLiteral("取消"), contentWidget);
    
    btnCreate->setStyleSheet(
        "QPushButton {"
        " background-color: #4CAF50;"
        " color: white;"
        " padding: 8px 20px;"
        " border: none;"
        " border-radius: 4px;"
        " font-size: 14px;"
        "}"
        "QPushButton:hover {"
        " background-color: #45a049;"
        "}"
        "QPushButton:pressed {"
        " background-color: #3d8b40;"
        "}"
    );
    
    btnCancel->setStyleSheet(
        "QPushButton {"
        " background-color: rgba(255, 255, 255, 0.1);"
        " color: white;"
        " padding: 8px 20px;"
        " border: 1px solid rgba(255, 255, 255, 0.2);"
        " border-radius: 4px;"
        " font-size: 14px;"
        "}"
        "QPushButton:hover {"
        " background-color: rgba(255, 255, 255, 0.15);"
        "}"
        "QPushButton:pressed {"
        " background-color: rgba(255, 255, 255, 0.2);"
        "}"
    );

    contentLayout->addWidget(nameLabel);
    contentLayout->addWidget(m_nameEdit);
    contentLayout->addLayout(buttonLayout);
    buttonLayout->addStretch();
    buttonLayout->addWidget(btnCreate);
    buttonLayout->addWidget(btnCancel);

    // 添加到主布局
    mainLayout->addWidget(titleBar);
    mainLayout->addWidget(contentWidget);
    mainLayout->addStretch();

    connect(btnCreate, &QPushButton::clicked, this, &NormalGroupDialog::onCreateClicked);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
}

void NormalGroupDialog::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect rect(0, 0, width(), height());
    QPainterPath path;
    path.addRoundedRect(rect, m_radius, m_radius);

    p.fillPath(path, QBrush(m_backgroundColor));
    QPen pen(m_borderColor, m_borderWidth);
    p.strokePath(path, pen);
}

void NormalGroupDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
    }
    QDialog::mousePressEvent(event);
}

void NormalGroupDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
    }
    QDialog::mouseMoveEvent(event);
}

void NormalGroupDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
    QDialog::mouseReleaseEvent(event);
}

void NormalGroupDialog::onCreateClicked()
{
    QString groupName = m_nameEdit->text().trimmed();
    if (groupName.isEmpty()) {
        CustomMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("群组名称不能为空！"));
        return;
    }

    UserInfo userinfo = CommonInfo::GetData();
    createNormalGroup(groupName, userinfo);
}

void NormalGroupDialog::createNormalGroup(const QString& groupName, const UserInfo& userinfo)
{
    if (!m_restAPI) {
        CustomMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("REST API未初始化！"));
        return;
    }

    // 在使用REST API前设置管理员账号信息
    std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
    if (!adminUserId.empty()) {
        std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
        m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
    } else {
        CustomMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("管理员账号未设置！\n请确保用户已登录且CommonInfo已初始化用户信息。"));
        return;
    }

    // 构造初始成员数组（只有创建者）
    QJsonArray memberArray;
    QString creatorId = userinfo.teacher_unique_id;
    QJsonObject ownerMemberInfo;
    ownerMemberInfo["Member_Account"] = creatorId;
    ownerMemberInfo["Role"] = "Admin";
    // 设置创建者群名片（NameCard），否则拉取成员列表时可能只显示账号ID
    // 优先使用本地用户姓名；为空时可不传（由腾讯侧返回 Nick/Identifier）
    if (!userinfo.strName.trimmed().isEmpty()) {
        ownerMemberInfo["NameCard"] = userinfo.strName.trimmed();
    }
    memberArray.append(ownerMemberInfo);

    // 创建回调数据结构
    struct CreateGroupCallbackData {
        NormalGroupDialog* dlg;
        QString groupName;
        UserInfo userInfo;
    };

    CreateGroupCallbackData* callbackData = new CreateGroupCallbackData;
    callbackData->dlg = this;
    callbackData->groupName = groupName;
    callbackData->userInfo = userinfo;

    // 调用腾讯云 IM REST API 创建"普通群"（公开群）
    m_restAPI->createGroup(groupName, "Public", memberArray,
        [=](int errorCode, const QString& errorDesc, const QJsonObject& result) {
            if (errorCode != 0) {
                QString errorMsg = QString(QStringLiteral("创建群组失败: %1 (错误码: %2)")).arg(errorDesc).arg(errorCode);
                qDebug() << errorMsg;
                CustomMessageBox::critical(callbackData->dlg, QStringLiteral("创建群组失败"), errorMsg);
                delete callbackData;
                return;
            }

            // 创建群组成功，解析返回的群组ID
            QString groupId = result["GroupId"].toString();
            if (groupId.isEmpty()) {
                QJsonObject groupInfo = result["GroupInfo"].toObject();
                groupId = groupInfo["GroupId"].toString();
            }

            qDebug() << "创建普通群成功，群组ID:" << groupId;

            if (!groupId.isEmpty()) {
                // 注意：部分腾讯 REST API 场景下，create_group 的 MemberList 可能不会真正写入 NameCard。
                // 为确保后续 get_group_member_info 能拿到成员名字，这里在创建成功后补一次 modify_group_member_info 写入 NameCard。
                const QString creatorId = callbackData->userInfo.teacher_unique_id;
                const QString creatorName = callbackData->userInfo.strName.trimmed();
                if (!creatorId.isEmpty() && !creatorName.isEmpty() && callbackData->dlg && callbackData->dlg->m_restAPI) {
                    callbackData->dlg->m_restAPI->modifyGroupMemberInfo(groupId, creatorId, QString(), -1, creatorName,
                        [creatorId, creatorName](int ec, const QString& ed, const QJsonObject& /*res*/) {
                            if (ec != 0) {
                                qWarning() << "创建普通群后设置创建者 NameCard 失败，member:" << creatorId
                                           << "name:" << creatorName << "error:" << ec << ed;
                            } else {
                                qDebug() << "创建普通群后设置创建者 NameCard 成功，member:" << creatorId << "name:" << creatorName;
                            }
                        });
                }

                CustomMessageBox::information(callbackData->dlg, QStringLiteral("创建群组成功"),
                    QString(QStringLiteral("群组创建成功！\n群组ID: %1")).arg(groupId));

                // 发出群组创建成功信号
                emit callbackData->dlg->groupCreated(groupId);
                callbackData->dlg->accept();
            } else {
                qWarning() << "创建群组成功但未获取到群组ID";
                CustomMessageBox::warning(callbackData->dlg, QStringLiteral("警告"), QStringLiteral("创建群组成功但未获取到群组ID"));
            }

            delete callbackData;
        });
}
