#include "ClassTeacherDelDialog.h"
#include "QGroupInfo.h"
#include "ClassTeacherDialog.h"
#include "FriendSelectDialog.h"
#include "MemberKickDialog.h"
#include <QFrame>
#include <QToolButton>
#include <QRegularExpression>
#include <QInputDialog>
#include <QJsonArray>
#include <QSet>
#include <QPointer>
#include <QMetaObject>
#include <QScreen>
#include <QApplication>

namespace {
struct GroupMemberFetchSDKData {
    QPointer<QGroupInfo> dlg;
    QString groupId;
    QVector<GroupMemberInfo> members;
};

struct MemberRenderSig {
    QString name;
    QString role;
    bool voiceEnabled = false;
};

static bool sameMemberListUnorderedForRender(const QVector<GroupMemberInfo>& a, const QVector<GroupMemberInfo>& b) {
    if (a.size() != b.size()) return false;

    // 按 member_id 对齐比较（忽略顺序）。发现重复 member_id 时直接认为不同，避免误判。
    QHash<QString, MemberRenderSig> mapB;
    mapB.reserve(b.size() * 2);
    for (const auto& m : b) {
        if (m.member_id.isEmpty()) return false;
        if (mapB.contains(m.member_id)) return false; // duplicate in b
        mapB.insert(m.member_id, MemberRenderSig{ m.member_name, m.member_role, m.is_voice_enabled });
    }

    QSet<QString> seenA;
    seenA.reserve(a.size());
    for (const auto& m : a) {
        if (m.member_id.isEmpty()) return false;
        if (seenA.contains(m.member_id)) return false; // duplicate in a
        seenA.insert(m.member_id);

        const auto it = mapB.constFind(m.member_id);
        if (it == mapB.constEnd()) return false;
        const MemberRenderSig& sig = it.value();
        if (sig.name != m.member_name || sig.role != m.member_role || sig.voiceEnabled != m.is_voice_enabled) {
            return false;
        }
    }

    return true;
}

static QString mapSdkRoleToText(int role) {
    if (role == kTIMMemberRole_Owner) return QStringLiteral("群主");
    if (role == kTIMMemberRole_Admin) return QStringLiteral("管理员");
    return QStringLiteral("成员");
}

static void startFetchMembersFromSDK(GroupMemberFetchSDKData* data, quint64 nextSeq) {
    if (!data || !data->dlg) {
        delete data;
        return;
    }
    if (data->groupId.trimmed().isEmpty()) {
        delete data;
        return;
    }

    QJsonObject option;
    const quint64 infoFlag = static_cast<quint64>(kTIMGroupMemberInfoFlag_NameCard) |
                             static_cast<quint64>(kTIMGroupMemberInfoFlag_MemberRole);
    option[QString::fromUtf8(kTIMGroupMemberGetInfoOptionInfoFlag)] = static_cast<double>(infoFlag);
    option[QString::fromUtf8(kTIMGroupMemberGetInfoOptionRoleFlag)] = static_cast<double>(kTIMGroupMemberRoleFlag_All);

    QJsonObject req;
    req[QString::fromUtf8(kTIMGroupGetMemberInfoListParamGroupId)] = data->groupId;
    req[QString::fromUtf8(kTIMGroupGetMemberInfoListParamOption)] = option;
    req[QString::fromUtf8(kTIMGroupGetMemberInfoListParamNextSeq)] = static_cast<double>(nextSeq);

    const QByteArray json = QJsonDocument(req).toJson(QJsonDocument::Compact);

    int ret = TIMGroupGetMemberInfoList(json.constData(),
        [](int32_t code, const char* desc, const char* json_param, const void* user_data) {
            GroupMemberFetchSDKData* cb = (GroupMemberFetchSDKData*)user_data;
            if (!cb) return;
            if (!cb->dlg) { delete cb; return; }

            if (code != 0) {
                qWarning() << "TIMGroupGetMemberInfoList failed, code:" << code << "desc:" << (desc ? desc : "");
                // SDK 回调线程不确定，UI/网络都切回主线程再处理
                const QPointer<QGroupInfo> dlg = cb->dlg;
                const QString groupId = cb->groupId;
                if (dlg) {
                    QMetaObject::invokeMethod(dlg, [dlg, groupId]() {
                        if (!dlg) return;
                        // SDK 失败时，尝试用 REST 兜底（若 admin 已配置）
                        dlg->fetchGroupMemberListFromREST(groupId);
                    }, Qt::QueuedConnection);
                }
                delete cb;
                return;
            }

            const QByteArray payload = QByteArray(json_param ? json_param : "");
            QJsonParseError pe;
            QJsonDocument doc = QJsonDocument::fromJson(payload, &pe);
            if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
                qWarning() << "TIMGroupGetMemberInfoList parse json failed:" << pe.errorString();
                // 解析失败也走一次兜底，避免成员区长期空白
                const QPointer<QGroupInfo> dlg = cb->dlg;
                const QString groupId = cb->groupId;
                if (dlg) {
                    QMetaObject::invokeMethod(dlg, [dlg, groupId]() {
                        if (!dlg) return;
                        dlg->fetchGroupMemberListFromREST(groupId);
                    }, Qt::QueuedConnection);
                }
                delete cb;
                return;
            }

            const QJsonObject obj = doc.object();
            const QJsonArray infoArr = obj.value(QString::fromUtf8(kTIMGroupGetMemberInfoListResultInfoArray)).toArray();

            QSet<QString> existingIds;
            for (const auto& m : cb->members) existingIds.insert(m.member_id);

            for (const auto& v : infoArr) {
                if (!v.isObject()) continue;
                const QJsonObject mo = v.toObject();

                const QString memberId = mo.value(QString::fromUtf8(kTIMGroupMemberInfoIdentifier)).toString();
                if (memberId.isEmpty()) continue;
                if (existingIds.contains(memberId)) continue;

                const QString nameCard = mo.value(QString::fromUtf8(kTIMGroupMemberInfoNameCard)).toString();
                const int role = mo.value(QString::fromUtf8(kTIMGroupMemberInfoMemberRole)).toInt();

                GroupMemberInfo mi;
                mi.member_id = memberId;
                mi.member_name = nameCard.trimmed().isEmpty() ? memberId : nameCard;
                mi.member_role = mapSdkRoleToText(role);
                mi.is_voice_enabled = true; // 普通群默认不控制对讲
                cb->members.append(mi);
                existingIds.insert(memberId);
            }

            const quint64 nextSeq = static_cast<quint64>(obj.value(QString::fromUtf8(kTIMGroupGetMemberInfoListResultNexSeq)).toVariant().toULongLong());
            if (nextSeq != 0) {
                startFetchMembersFromSDK(cb, nextSeq);
                return;
            }

            // UI 刷新必须在主线程
            const QPointer<QGroupInfo> dlg = cb->dlg;
            const QString groupId = cb->groupId;
            const QVector<GroupMemberInfo> members = cb->members;
            if (dlg) {
                QMetaObject::invokeMethod(dlg, [dlg, groupId, members]() {
                    if (!dlg) return;
                    dlg->InitGroupMember(groupId, members);
                }, Qt::QueuedConnection);
            }
            delete cb;
        },
        data);

    if (ret != 0) {
        qWarning() << "TIMGroupGetMemberInfoList call returned:" << ret;
        if (data->dlg) data->dlg->fetchGroupMemberListFromREST(data->groupId);
        delete data;
        return;
    }
}
} // namespace

// ==================== IntercomControlWidget 实现 ====================

IntercomControlWidget::IntercomControlWidget(QWidget* parent)
    : QWidget(parent)
    , m_enabled(false)
    , m_buttonPressed(false)
{
    setFixedHeight(50); // 设置固定高度
    setMinimumWidth(200);
}

IntercomControlWidget::~IntercomControlWidget()
{
}

void IntercomControlWidget::setIntercomEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        update(); // 触发重绘
        emit intercomToggled(enabled);
    }
}

void IntercomControlWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // 抗锯齿
    
    // 绘制背景
    drawBackground(painter);
    
    // 绘制"开启对讲"按钮
    drawButton(painter);
    
    // 绘制开关
    drawToggleSwitch(painter);
}

void IntercomControlWidget::drawBackground(QPainter& painter)
{
    // 绘制背景色 #555555
    painter.fillRect(rect(), QColor(0x55, 0x55, 0x55));
}

void IntercomControlWidget::drawButton(QPainter& painter)
{
    QRect btnRect = getButtonRect();
    
    // 绘制按钮背景（蓝色）
    QColor btnColor = m_buttonPressed ? QColor(0, 100, 200) : QColor(0, 120, 255); // 按下时颜色稍深
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(btnColor));
    painter.drawRoundedRect(btnRect, 5, 5); // 圆角矩形
    
    // 绘制按钮文字
    painter.setPen(QColor(255, 255, 255)); // 白色文字
    QFont btnFont = painter.font();
    btnFont.setPointSize(12);
    btnFont.setBold(true);
    painter.setFont(btnFont);
    painter.drawText(btnRect, Qt::AlignCenter, "开启对讲");
}

void IntercomControlWidget::drawToggleSwitch(QPainter& painter)
{
    QRect switchRect = getSwitchRect();
    
    // 开关背景（灰色或绿色）
    QColor bgColor = m_enabled ? QColor(76, 175, 80) : QColor(200, 200, 200); // 开启时绿色，关闭时灰色
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(bgColor));
    painter.drawRoundedRect(switchRect, switchRect.height() / 2, switchRect.height() / 2);
    
    // 开关滑块（圆形）
    int sliderSize = switchRect.height() - 4;
    int sliderX = m_enabled ? (switchRect.right() - sliderSize - 2) : (switchRect.left() + 2);
    int sliderY = switchRect.top() + 2;
    
    painter.setBrush(QBrush(QColor(255, 255, 255))); // 白色滑块
    painter.drawEllipse(sliderX, sliderY, sliderSize, sliderSize);
}

QRect IntercomControlWidget::getButtonRect() const
{
    int btnWidth = 100;
    int btnHeight = 35;
    int btnX = 10;
    int btnY = (height() - btnHeight) / 2;
    return QRect(btnX, btnY, btnWidth, btnHeight);
}

QRect IntercomControlWidget::getSwitchRect() const
{
    int switchWidth = 50;
    int switchHeight = 25;
    int switchX = getButtonRect().right() + 20;
    int switchY = (height() - switchHeight) / 2;
    return QRect(switchX, switchY, switchWidth, switchHeight);
}

void IntercomControlWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        QRect btnRect = getButtonRect();
        QRect switchRect = getSwitchRect();
        
        if (btnRect.contains(event->pos())) {
            // 点击了按钮
            m_buttonPressed = true;
            update();
            emit buttonClicked();
        } else if (switchRect.contains(event->pos())) {
            // 点击了开关
            setIntercomEnabled(!m_enabled);
        }
    }
    QWidget::mousePressEvent(event);
}

void IntercomControlWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_buttonPressed = false;
        update();
    }
    QWidget::mouseReleaseEvent(event);
}

// ==================== QGroupInfo 实现 ====================

QGroupInfo::QGroupInfo(QWidget* parent)
	: QDialog(parent)
{
    // 初始化时不显示，由外部调用 show() 时才显示
    //setVisible(false);
    
    // 初始化成员变量
    m_circlePlus = nullptr;
    m_circleMinus = nullptr;
    m_btnDismiss = nullptr;
    m_btnExit = nullptr;
}

bool QGroupInfo::validateSubjectFormat(bool showMessage) const
{
    if (!m_subjectTagLayout) {
        return true;
    }

    // 至少有一个 tag
    int tagCount = 0;
    for (int i = 0; i < m_subjectTagLayout->count(); ++i) {
        QWidget* w = m_subjectTagLayout->itemAt(i) ? m_subjectTagLayout->itemAt(i)->widget() : nullptr;
        if (!w) continue;
        if (w == m_addSubjectBtn) continue;
        if (w->property("isSubjectTag").toBool()) {
            const QString text = w->property("subjectText").toString().trimmed();
            if (!text.isEmpty()) {
                tagCount++;
            }
        }
    }

    if (tagCount <= 0) {
        if (showMessage) {
            // 创建自定义警告对话框（无标题栏，有关闭按钮）
            QDialog* warnDlg = new QDialog(const_cast<QGroupInfo*>(this));
            warnDlg->setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog | Qt::WindowStaysOnTopHint);
            warnDlg->setAttribute(Qt::WA_TranslucentBackground);
            warnDlg->setFixedSize(350, 120);
            
            // 创建主容器
            QWidget* container = new QWidget(warnDlg);
            container->setStyleSheet(
                "QWidget { background-color: #282A2B; border: 1px solid #282A2B; border-radius: 8px; }"
            );
            
            QVBoxLayout* containerLayout = new QVBoxLayout(warnDlg);
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
            btnClose->setVisible(false); // 初始隐藏
            connect(btnClose, &QPushButton::clicked, warnDlg, &QDialog::reject);
            
            // 标题文本
            QLabel* titleLabel = new QLabel(QString::fromUtf8(u8"提示"), titleBar);
            titleLabel->setStyleSheet("color: #ffffff; font-size: 14px; font-weight: bold; background: transparent; border: none;");
            titleLabel->setAlignment(Qt::AlignCenter);
            
            titleLayout->addWidget(titleLabel, 1);
            titleLayout->addWidget(btnClose);
            
            // 内容区域
            QWidget* contentWidget = new QWidget(container);
            contentWidget->setStyleSheet("background-color: #282A2B; border-bottom-left-radius: 8px; border-bottom-right-radius: 8px;");
            
            QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
            mainLayout->setContentsMargins(20, 15, 20, 15);
            mainLayout->setSpacing(10);
            
            // 消息文本
            QLabel* messageLabel = new QLabel(QString::fromUtf8(u8"请至少输入一个任教科目。"), contentWidget);
            messageLabel->setStyleSheet("color: white; font-size: 14px; background: transparent;");
            messageLabel->setAlignment(Qt::AlignCenter);
            messageLabel->setWordWrap(true);
            
            mainLayout->addWidget(messageLabel);
            
            // 确定按钮
            QPushButton* btnOk = new QPushButton(QString::fromUtf8(u8"确定"), contentWidget);
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
            connect(btnOk, &QPushButton::clicked, warnDlg, &QDialog::accept);
            
            QHBoxLayout* btnLayout = new QHBoxLayout();
            btnLayout->addStretch();
            btnLayout->addWidget(btnOk);
            btnLayout->addStretch();
            mainLayout->addLayout(btnLayout);
            
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
            
            CloseButtonEventFilter* filter = new CloseButtonEventFilter(warnDlg, btnClose);
            warnDlg->installEventFilter(filter);
            
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
            
            DragEventFilter* dragFilter = new DragEventFilter(warnDlg, titleBar);
            warnDlg->installEventFilter(dragFilter);
            
            // 居中显示
            QScreen* screen = QApplication::primaryScreen();
            if (screen) {
                QRect screenGeometry = screen->geometry();
                int x = (screenGeometry.width() - warnDlg->width()) / 2;
                int y = (screenGeometry.height() - warnDlg->height()) / 2;
                warnDlg->move(x, y);
            }
            
            warnDlg->exec();
            warnDlg->deleteLater();
            
            if (m_addSubjectBtn) m_addSubjectBtn->setFocus();
        }
        return false;
    }

    return true;
}

QWidget* QGroupInfo::makeSubjectTagWidget(const QString& subjectText)
{
    QWidget* parent = m_subjectTagContainer ? static_cast<QWidget*>(m_subjectTagContainer) : static_cast<QWidget*>(this);

    QFrame* tag = new QFrame(parent);
    tag->setProperty("isSubjectTag", true);
    tag->setProperty("subjectText", subjectText);
    tag->setFixedHeight(28); // 与“+ 添加”按钮高度一致
    tag->setStyleSheet(
        "QFrame {"
        "  background-color: rgba(0,0,0,0.18);"
        "  border: 1px solid rgba(255,255,255,0.16);"
        "  border-radius: 14px;"
        "}"
        "QToolButton { border:none; color:#ffffff; font-weight:bold; padding:0 6px; }"
        "QToolButton:hover { color:#ffdddd; }"
        "QLabel { color:#ffffff; padding-right:10px; }"
    );

    QHBoxLayout* l = new QHBoxLayout(tag);
    l->setContentsMargins(6, 0, 6, 0);
    l->setSpacing(2);

    QToolButton* btnX = new QToolButton(tag);
    btnX->setText(QStringLiteral("×"));
    btnX->setCursor(Qt::PointingHandCursor);

    QLabel* lbl = new QLabel(subjectText, tag);
    lbl->setStyleSheet("font-size: 13px;");

    l->addWidget(btnX, 0, Qt::AlignVCenter);
    l->addWidget(lbl, 0, Qt::AlignVCenter);

    connect(btnX, &QToolButton::clicked, this, [this, tag]() {
        m_subjectsDirty = true;
        if (!m_subjectTagLayout) { tag->deleteLater(); return; }
        m_subjectTagLayout->removeWidget(tag);
        tag->deleteLater();
    });

    return tag;
}

void QGroupInfo::setTeachSubjectsInUI(const QStringList& subjects)
{
    if (!m_subjectTagLayout) return;

    // 清空布局中的所有项，但保留 m_addSubjectBtn（复用）
    while (m_subjectTagLayout->count() > 0) {
        QLayoutItem* item = m_subjectTagLayout->takeAt(0);
        if (!item) break;
        QWidget* w = item->widget();
        if (w) {
            if (w == m_addSubjectBtn) {
                // 不删除，仅从布局中移除，稍后重新插入
            } else {
                w->deleteLater();
            }
        }
        delete item; // spacer / layout item
    }

    // 重新构建：tags + stretch + "+ 添加"
    for (const auto& s : subjects) {
        const QString t = s.trimmed();
        if (t.isEmpty()) continue;
        m_subjectTagLayout->addWidget(makeSubjectTagWidget(t), 0, Qt::AlignVCenter);
    }
    m_subjectTagLayout->addStretch();
    if (m_addSubjectBtn) {
        m_subjectTagLayout->addWidget(m_addSubjectBtn, 0, Qt::AlignVCenter);
    }

    m_subjectsDirty = false;
}

QStringList QGroupInfo::collectTeachSubjects() const
{
    QStringList subjects;
    if (!m_subjectTagLayout) return subjects;

    QSet<QString> seen;
    for (int i = 0; i < m_subjectTagLayout->count(); ++i) {
        QWidget* w = m_subjectTagLayout->itemAt(i) ? m_subjectTagLayout->itemAt(i)->widget() : nullptr;
        if (!w) continue;
        if (w == m_addSubjectBtn) continue;

        if (w->property("isSubjectTag").toBool()) {
            const QString text = w->property("subjectText").toString().trimmed();
            if (text.isEmpty()) continue;
            if (seen.contains(text)) continue;
            seen.insert(text);
            subjects.append(text);
        }
    }

    return subjects;
}

void QGroupInfo::postTeachSubjectsAndThenClose(int doneCode)
{
    if (m_savingTeachSubjects) return;

    // 未修改科目则不提交，直接关闭（避免重复请求/误触发服务端错误）
    if (!m_subjectsDirty) {
        QDialog::done(doneCode);
        return;
    }

    if (!validateSubjectFormat(true)) {
        return;
    }

    if (m_groupNumberId.isEmpty()) {
        qWarning() << "postTeachSubjectsAndThenClose: group_id is empty, skip posting teach_subjects.";
        QDialog::done(doneCode);
        return;
    }

    UserInfo userInfo = CommonInfo::GetData();
    const QString teacherUniqueId = userInfo.teacher_unique_id;
    if (teacherUniqueId.isEmpty()) {
        qWarning() << "postTeachSubjectsAndThenClose: teacher_unique_id is empty, skip posting teach_subjects.";
        QDialog::done(doneCode);
        return;
    }

    const QStringList subjects = collectTeachSubjects();
    QJsonArray subjectArray;
    for (const auto& s : subjects) subjectArray.append(s);

    QJsonObject requestData;
    requestData["group_id"] = m_groupNumberId;
    // 注意：后端该字段实际使用 teacher_unique_id
    requestData["user_id"] = teacherUniqueId;
    requestData["teach_subjects"] = subjectArray;

    QJsonDocument doc(requestData);
    const QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    const QString url = "http://47.100.126.194:5000/groups/member/teach-subjects";

    m_savingTeachSubjects = true;

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest networkRequest;
    networkRequest.setUrl(QUrl(url));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = manager->post(networkRequest, jsonData);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        m_savingTeachSubjects = false;

        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray response = reply->readAll();

        if (reply->error() != QNetworkReply::NoError) {
            const QString err = reply->errorString();
            qWarning() << "teach-subjects post failed:" << err << "httpStatus:" << httpStatus
                       << "response:" << QString::fromUtf8(response);
            // 404 可能来自服务端业务判断（HTTPException(404)），因此把响应体也展示出来便于排查
            const QString respText = QString::fromUtf8(response).trimmed();
            CustomMessageBox::warning(this, QString::fromUtf8(u8"保存失败"),
                                 QString::fromUtf8(u8"任教科目保存失败：%1（HTTP %2）%3")
                                     .arg(err)
                                     .arg(httpStatus)
                                     .arg(respText.isEmpty() ? QString() : ("\n" + respText)));
            reply->deleteLater();
            manager->deleteLater();
            return;
        }

        qDebug() << "teach-subjects server response:" << QString::fromUtf8(response);

        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &parseError);
        if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
            CustomMessageBox::warning(this, QString::fromUtf8(u8"保存失败"),
                                 QString::fromUtf8(u8"任教科目保存失败：解析服务端响应失败。"));
            reply->deleteLater();
            manager->deleteLater();
            return;
        }

        const QJsonObject rootObj = jsonDoc.object();
        // 兼容两种返回结构：{code,message,...} 或 {data:{code,message,...}}
        QJsonObject container = rootObj;
        if (rootObj.contains("data") && rootObj.value("data").isObject()) {
            container = rootObj.value("data").toObject();
        }

        const QJsonValue codeVal = container.value("code");
        int code = 0;
        if (codeVal.isDouble()) {
            code = codeVal.toInt();
        } else if (codeVal.isString()) {
            code = codeVal.toString().toInt();
        }
        const QString message = container.value("message").toString();

        if (code != 200) {
            CustomMessageBox::warning(this, QString::fromUtf8(u8"保存失败"),
                                 QString::fromUtf8(u8"任教科目保存失败：%1").arg(message.isEmpty() ? QString::number(code) : message));
            reply->deleteLater();
            manager->deleteLater();
            return;
        }

        // 保存成功后，视为已同步到服务端
        m_subjectsDirty = false;

        reply->deleteLater();
        manager->deleteLater();

        // 成功后再关闭窗口（直接调基类，避免递归触发本类 done()）
        QDialog::done(doneCode);
    });
}

void QGroupInfo::done(int r)
{
    // 普通群模式不涉及任教科目校验，直接关闭
    if (m_isNormalGroup) {
        QDialog::done(r);
        return;
    }

    // 班级群模式：所有关闭路径都要校验：当尝试关闭时，如果科目格式不合法则阻止关闭
    if (r == QDialog::Rejected) {
        // 关闭前保存任教科目（后端 user_id 使用 teacher_unique_id）
        postTeachSubjectsAndThenClose(r);
        return;
    }
    QDialog::done(r);
}

void QGroupInfo::initData(QString groupName, QString groupNumberId, bool iGroupOwner, QString classid)
{
    // 如果已经初始化过了，直接返回
    if (m_initialized) {
        return;
    }

    m_groupName = groupName;
    m_groupNumberId = groupNumberId;
    m_classId = classid;
    m_iGroupOwner = iGroupOwner;
    m_isNormalGroup = classid.trimmed().isEmpty();

    setWindowTitle(m_isNormalGroup ? QStringLiteral("群管理") : QStringLiteral("班级管理"));
    // 设置无边框窗口
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    resize(m_isNormalGroup ? 360 : 300, m_isNormalGroup ? 520 : 600);

    if (m_isNormalGroup) {
        setStyleSheet(
            "QDialog { background-color: #1E1F22; color: white; border: 1px solid rgba(255,255,255,0.14); font-weight: 600; } "
            "QPushButton { font-size:14px; color: white; background-color: rgba(255,255,255,0.06); border: 1px solid rgba(255,255,255,0.10); border-radius: 8px; font-weight: 700; } "
            "QPushButton:hover { background-color: rgba(255,255,255,0.10); } "
            "QLabel { font-size:13px; color: white; background: transparent; } "
            "QLineEdit { color: white; background-color: rgba(255,255,255,0.06); border: 1px solid rgba(255,255,255,0.10); border-radius: 8px; padding: 6px 10px; } "
            "QGroupBox { color: rgba(255,255,255,0.85); border: 1px solid rgba(255,255,255,0.08); border-radius: 10px; margin-top: 10px; } "
            "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 6px; } "
            "QCheckBox { color: rgba(255,255,255,0.90); }"
        );
    } else {
        setStyleSheet("QDialog { background-color: #5C5C5C; color: white; border: 1px solid #5C5C5C; font-weight: bold; } "
            "QPushButton { font-size:14px; color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
            "QLabel { font-size:14px; color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
            "QLineEdit { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
            "QTextEdit { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
            "QGroupBox { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
            "QTableWidget { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; gridline-color: #5C5C5C; font-weight: bold; } "
            "QTableWidget::item { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
            "QComboBox { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
            "QCheckBox { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
            "QRadioButton { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
            "QScrollArea { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
            "QListWidget { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
            "QSpinBox { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
            "QProgressBar { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
            "QSlider { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; }");
    }
    
    // 关闭按钮（右上角）
    m_closeButton = new QPushButton(this);
    m_closeButton->setIcon(QIcon(":/res/img/widget-close.png"));
    m_closeButton->setIconSize(QSize(22, 22));
    m_closeButton->setFixedSize(QSize(22, 22));
    m_closeButton->setStyleSheet("background: transparent;");
    m_closeButton->move(width() - 22, 0);
    m_closeButton->hide();
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 初始化REST API
    m_restAPI = new TIMRestAPI(this);
    // 注意：管理员账号信息在使用REST API时再设置，因为此时用户可能还未登录
    
    m_friendSelectDlg = new FriendSelectDialog(this);
    if (m_friendSelectDlg) {
        // 普通群：邀请成员操作走腾讯 SDK（不走自建服务器）
        m_friendSelectDlg->setUseTencentSDK(m_isNormalGroup);
    }
    // 连接成员邀请成功信号，刷新成员列表
    if (m_friendSelectDlg) {
        connect(m_friendSelectDlg, &FriendSelectDialog::membersInvitedSuccess, this, [this](const QString& groupId) {
            // 刷新成员列表（通知父窗口刷新）
            refreshMemberList(groupId);
        });
    }
    m_memberKickDlg = new MemberKickDialog(this);
    if (m_memberKickDlg) {
        // 普通群：踢人操作走腾讯 SDK（不走自建服务器）
        m_memberKickDlg->setUseTencentSDK(m_isNormalGroup);
    }
    // 连接成员踢出成功信号，刷新成员列表
    if (m_memberKickDlg) {
        connect(m_memberKickDlg, &MemberKickDialog::membersKickedSuccess, this, [this](const QString& groupId) {
            // 刷新成员列表（通知父窗口刷新）
            refreshMemberList(groupId);
        });
    }
    //m_classTeacherDelDlg = new ClassTeacherDelDialog(this);
    if (!m_isNormalGroup) {
        m_courseDlg = new CourseDialog();
        m_courseDlg->setWindowTitle("课程表");
        m_courseDlg->resize(800, 600);
        // 设置群组ID和班级ID
        m_courseDlg->setGroupId(groupNumberId);
        m_courseDlg->setClassId(classid);
        
        // 创建壁纸对话框
        m_wallpaperDlg = new WallpaperDialog(this);
        m_wallpaperDlg->setGroupId(groupNumberId);
        m_wallpaperDlg->setClassId(classid);
        
        // 创建值日表对话框
        m_dutyRosterDlg = new DutyRosterDialog(this);
        m_dutyRosterDlg->setGroupId(groupNumberId);
        m_dutyRosterDlg->setClassId(classid);
    } else {
        m_courseDlg = nullptr;
        m_wallpaperDlg = nullptr;
        m_dutyRosterDlg = nullptr;
    }

    // 设置一些课程
    //m_courseDlg->setCourse(1, 0, "数学");
    //m_courseDlg->setCourse(1, 1, "音乐");
    //m_courseDlg->setCourse(2, 1, "语文", true); // 高亮
    //m_courseDlg->setCourse(3, 4, "体育", true);

    // 顶部栏
    {
        QHBoxLayout* topLayout = new QHBoxLayout;
        topLayout->setContentsMargins(6, 0, 6, 0);

        if (m_isNormalGroup) {
            QLabel* lblTitle = new QLabel(QStringLiteral("群管理"), this);
            lblTitle->setStyleSheet("font-size: 15px; font-weight: 700;");
            topLayout->addWidget(lblTitle);
            topLayout->addStretch();
            if (m_closeButton) {
                m_closeButton->move(width() - 22, 0);
                m_closeButton->show();
                topLayout->addWidget(m_closeButton);
            }
        } else {
            QLabel* lblAvatar = new QLabel(this);
            lblAvatar->setFixedSize(50, 50);
            lblAvatar->setStyleSheet("background-color: lightgray; border-radius: 25px;");
            lblAvatar->setScaledContents(true);
            // 保存头像标签的指针，以便后续更新头像
            m_groupAvatarLabel = lblAvatar;
            
            QLabel* lblInfo = new QLabel(groupName + "\n" + groupNumberId, this);
            QPushButton* btnMore = new QPushButton("...", this);
            btnMore->setFixedSize(30, 30);
            topLayout->addWidget(lblAvatar);
            topLayout->addWidget(lblInfo, 1);
            topLayout->addStretch();
            topLayout->addWidget(btnMore);
        }

        mainLayout->addLayout(topLayout);
    }

    // 班级相关：普通群隐藏
    if (!m_isNormalGroup) {
        // 班级编号
        QLineEdit* editClassNum = new QLineEdit("2349235", this);
        editClassNum->setAlignment(Qt::AlignCenter);
        editClassNum->setStyleSheet("color:red; font-size:18px; font-weight:bold;");
        mainLayout->addWidget(editClassNum);

        QHBoxLayout* pHBoxLayut = new QHBoxLayout;
        pHBoxLayut->setSpacing(12);  // 增加按钮之间的间隔
        
        // 班级课程表按钮
        QPushButton* btnSchedule = new QPushButton("班级课程表", this);
        btnSchedule->setMinimumHeight(40);  // 增加按钮高度
        btnSchedule->setStyleSheet("background-color:green; color:white; font-weight:bold; font-size:14px; padding:8px 16px;");
        
        // 值日表按钮
        QPushButton* btnDuty = new QPushButton("值日表", this);
        btnDuty->setMinimumHeight(40);  // 增加按钮高度
        btnDuty->setStyleSheet("background-color:green; color:white; font-weight:bold; font-size:14px; padding:8px 16px;");
        
        // 壁纸按钮
        QPushButton* btnWallpaper = new QPushButton("壁纸", this);
        btnWallpaper->setMinimumHeight(40);  // 增加按钮高度
        btnWallpaper->setStyleSheet("background-color:red; color:white; font-weight:bold; font-size:14px; padding:8px 16px;");

        pHBoxLayut->addWidget(btnSchedule);
        pHBoxLayut->addWidget(btnDuty);
        pHBoxLayut->addWidget(btnWallpaper);
        pHBoxLayut->addStretch(2);
        pHBoxLayut->setContentsMargins(10, 15, 10, 25);
        mainLayout->addLayout(pHBoxLayut);

        connect(btnSchedule, &QPushButton::clicked, this, [=]() {
            qDebug() << "班级课程表按钮被点击！";
            if (m_courseDlg)
            {
                m_courseDlg->show();
            }
        });
        
        connect(btnDuty, &QPushButton::clicked, this, [=]() {
            qDebug() << "值日表按钮被点击！";
            if (!m_dutyRosterDlg) {
                m_dutyRosterDlg = new DutyRosterDialog(this);
            }
            if (m_dutyRosterDlg) {
                m_dutyRosterDlg->setGroupId(m_groupNumberId);
                m_dutyRosterDlg->setClassId(m_classId);
                m_dutyRosterDlg->show();
            }
        });
        
        connect(btnWallpaper, &QPushButton::clicked, this, [=]() {
            qDebug() << "壁纸按钮被点击！";
            if (m_wallpaperDlg)
            {
                m_wallpaperDlg->setGroupId(m_groupNumberId);
                m_wallpaperDlg->setClassId(m_classId);
                m_wallpaperDlg->show();
            }
        });
    }
    
    // 减少间距，使三排内容更靠近
    mainLayout->addSpacing(3);

    // 群成员列表
    QGroupBox* groupFriends = new QGroupBox(m_isNormalGroup ? QStringLiteral("群成员") : QStringLiteral("好友列表"), this);
    groupFriends->setMinimumHeight(m_isNormalGroup ? 30 : 20);
    QVBoxLayout* friendsLayout = new QVBoxLayout(groupFriends);
    // 减少上边距，使按钮更靠近QGroupBox标题
    friendsLayout->setContentsMargins(10, 5, 10, 5);
    friendsLayout->setSpacing(5);  // 减少间距

    // 班级群：沿用旧的圆圈布局；普通群：改为网格布局（头像上、名字下）
    circlesLayout = nullptr;
    if (m_isNormalGroup) {
        // 普通群：提供"添加好友/删除好友"按钮（与 + / - tile 同功能）
        QHBoxLayout* memberActionRow = new QHBoxLayout;
        // 减少上边距，使按钮更靠近QGroupBox标题
        memberActionRow->setContentsMargins(0, 0, 0, 0);
        memberActionRow->setSpacing(8);
        memberActionRow->addStretch();

        QPushButton* btnAddFriend = new QPushButton(QStringLiteral("添加好友"), groupFriends);
        QPushButton* btnDelFriend = new QPushButton(QStringLiteral("删除好友"), groupFriends);
        btnAddFriend->setCursor(Qt::PointingHandCursor);
        btnDelFriend->setCursor(Qt::PointingHandCursor);
        btnAddFriend->setFixedHeight(28);
        btnDelFriend->setFixedHeight(28);
        btnAddFriend->setStyleSheet("QPushButton{padding:0 12px;}");
        btnDelFriend->setStyleSheet("QPushButton{padding:0 12px;}");

        auto openInviteDlg = [this]() {
            if (!m_friendSelectDlg) return;
            if (m_friendSelectDlg->isHidden()) {
                QVector<QString> memberIds;
                for (const auto& member : m_groupMemberInfo) {
                    memberIds.append(member.member_id);
                }
                m_friendSelectDlg->setExcludedMemberIds(memberIds);
                m_friendSelectDlg->setGroupId(m_groupNumberId);
                m_friendSelectDlg->setGroupName(m_groupName);
                m_friendSelectDlg->InitData();
                m_friendSelectDlg->show();
            } else {
                m_friendSelectDlg->hide();
            }
        };

        auto openKickDlg = [this]() {
            if (!m_memberKickDlg) return;
            if (m_memberKickDlg->isHidden()) {
                m_memberKickDlg->setGroupId(m_groupNumberId);
                m_memberKickDlg->setGroupName(m_groupName);
                m_memberKickDlg->InitData(m_groupMemberInfo);
                m_memberKickDlg->show();
            } else {
                m_memberKickDlg->hide();
            }
        };

        connect(btnAddFriend, &QPushButton::clicked, this, openInviteDlg);
        connect(btnDelFriend, &QPushButton::clicked, this, openKickDlg);

        memberActionRow->addWidget(btnAddFriend);
        memberActionRow->addWidget(btnDelFriend);
        friendsLayout->addLayout(memberActionRow);

        m_memberScrollArea = new QScrollArea(groupFriends);
        m_memberScrollArea->setWidgetResizable(true);
        m_memberScrollArea->setFrameShape(QFrame::NoFrame);
        m_memberScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_memberScrollArea->setStyleSheet(
            "QScrollArea { background: transparent; }"
            "QWidget { background: transparent; }"
        );

        m_memberGridContainer = new QWidget(groupFriends);
        m_memberGridLayout = new QGridLayout(m_memberGridContainer);
        m_memberGridLayout->setContentsMargins(0, 0, 0, 0);
        m_memberGridLayout->setHorizontalSpacing(12);
        m_memberGridLayout->setVerticalSpacing(10);

        m_memberScrollArea->setWidget(m_memberGridContainer);
        friendsLayout->addWidget(m_memberScrollArea);
    } else {
        circlesLayout = new QHBoxLayout();
        circlesLayout->setSpacing(2);
        circlesLayout->setContentsMargins(5, 5, 5, 5);
    }

    //QString blueStyle = "background-color:blue; border-radius:15px; color:white; font-weight:bold;";
    //QString redStyle = "background-color:red; border-radius:15px; color:white; font-weight:bold;";

    //FriendButton* circleRed = new FriendButton("", this);
    //circleRed->setStyleSheet(redStyle);
    //FriendButton* circleBlue1 = new FriendButton("", this);
    //FriendButton* circleBlue2 = new FriendButton("", this);
    //FriendButton* circlePlus = new FriendButton("+", this);
    //FriendButton* circleMinus = new FriendButton("-", this);

    //// 接收右键菜单信号
    //connect(circleRed, &FriendButton::setLeaderRequested, this, []() {
    //    qDebug() << "设为班主任";
    //    });
    //connect(circleRed, &FriendButton::cancelLeaderRequested, this, []() {
    //    qDebug() << "取消班主任";
    //    });
    //connect(circleBlue1, &FriendButton::setLeaderRequested, this, []() {
    //    qDebug() << "设为班主任";
    //    });
    //connect(circleBlue1, &FriendButton::cancelLeaderRequested, this, []() {
    //    qDebug() << "取消班主任";
    //    });
    //connect(circleBlue2, &FriendButton::setLeaderRequested, this, []() {
    //    qDebug() << "设为班主任";
    //    });
    //connect(circleBlue2, &FriendButton::cancelLeaderRequested, this, []() {
    //    qDebug() << "取消班主任";
    //    });

    //circlesLayout->addWidget(circleRed);
    //circlesLayout->addWidget(circleBlue1);
    //circlesLayout->addWidget(circleBlue2);
    //circlesLayout->addWidget(circlePlus);
    //circlesLayout->addWidget(circleMinus);

    // 班级群：在初始化时就创建 + 和 - 按钮，确保它们总是可见
    // 普通群：+/- 按钮会在 renderNormalGroupMemberGrid() 里以网格 tile 的形式渲染
    if (!m_isNormalGroup && circlesLayout) {
        // 注意：按钮的父控件应该是 groupFriends，这样它们才会显示在正确的容器内
        m_circlePlus = new FriendButton("+", groupFriends);
        m_circlePlus->setFixedSize(50, 50);
        m_circlePlus->setMinimumSize(50, 50);
        m_circlePlus->setContextMenuEnabled(false);
        m_circlePlus->setVisible(true);
        circlesLayout->addWidget(m_circlePlus);

        m_circleMinus = new FriendButton("-", groupFriends);
        m_circleMinus->setFixedSize(50, 50);
        m_circleMinus->setMinimumSize(50, 50);
        m_circleMinus->setContextMenuEnabled(false);
        m_circleMinus->setVisible(true);
        circlesLayout->addWidget(m_circleMinus);

        // 连接按钮点击事件（这些连接会在 InitGroupMember 中重新设置，但先设置确保可用）
        connect(m_circlePlus, &FriendButton::clicked, this, [this]() {
            if (m_friendSelectDlg)
            {
                if (m_friendSelectDlg->isHidden())
                {
                    // 获取当前群组成员ID列表，用于排除
                    QVector<QString> memberIds;
                    for (const auto& member : m_groupMemberInfo)
                    {
                        memberIds.append(member.member_id);
                    }
                    m_friendSelectDlg->setExcludedMemberIds(memberIds);
                    m_friendSelectDlg->setGroupId(m_groupNumberId);
                    m_friendSelectDlg->setGroupName(m_groupName);
                    m_friendSelectDlg->InitData();
                    m_friendSelectDlg->show();
                }
                else
                {
                    m_friendSelectDlg->hide();
                }
            }
        });

        connect(m_circleMinus, &FriendButton::clicked, this, [this]() {
            if (m_memberKickDlg)
            {
                if (m_memberKickDlg->isHidden())
                {
                    m_memberKickDlg->setGroupId(m_groupNumberId);
                    m_memberKickDlg->setGroupName(m_groupName);
                    m_memberKickDlg->InitData(m_groupMemberInfo);
                    m_memberKickDlg->show();
                }
                else
                {
                    m_memberKickDlg->hide();
                }
            }
        });

        friendsLayout->addLayout(circlesLayout);
    }
    mainLayout->addWidget(groupFriends);

    // 普通群设置区：群聊名称 + 接收通知
    if (m_isNormalGroup) {
        QGroupBox* groupSettings = new QGroupBox(QStringLiteral(""), this);
        QVBoxLayout* settingsLayout = new QVBoxLayout(groupSettings);
        settingsLayout->setContentsMargins(12, 12, 12, 12);
        settingsLayout->setSpacing(10);

        // 群聊名称
        QHBoxLayout* nameRow = new QHBoxLayout;
        QLabel* lblName = new QLabel(QStringLiteral("群聊名称"), groupSettings);
        m_editGroupName = new QLineEdit(m_groupName, groupSettings);
        m_editGroupName->setReadOnly(true); // 先按截图“显示”，后续可开放编辑并接入 modifyGroupInfo
        nameRow->addWidget(lblName);
        nameRow->addStretch();
        nameRow->addWidget(m_editGroupName, 1);
        settingsLayout->addLayout(nameRow);

        // 接收通知
        QHBoxLayout* notifyRow = new QHBoxLayout;
        QLabel* lblNotify = new QLabel(QStringLiteral("接收通知"), groupSettings);
        m_chkReceiveNotify = new QCheckBox(groupSettings);
        m_chkReceiveNotify->setChecked(true);
        notifyRow->addWidget(lblNotify);
        notifyRow->addStretch();
        notifyRow->addWidget(m_chkReceiveNotify);
        settingsLayout->addLayout(notifyRow);

        connect(m_chkReceiveNotify, &QCheckBox::toggled, this, [this](bool on) {
            qDebug() << "普通群 接收通知 toggled:" << on << " groupId:" << m_groupNumberId;
            // TODO: 如需持久化/同步到服务器，可在这里接入对应接口
        });

        mainLayout->addWidget(groupSettings);
    }

    // 班级群：科目输入（普通群不显示）
    // 减少间距，使好友列表和科目更靠近（只在班级群时）
    if (!m_isNormalGroup) {
        mainLayout->addSpacing(1);
    }
    
    QGroupBox* groupSubject = nullptr;
    QVBoxLayout* subjectLayout = nullptr;
    if (!m_isNormalGroup) {
        groupSubject = new QGroupBox("科目", this);
        subjectLayout = new QVBoxLayout(groupSubject);
    }

    // 任教科目：tag/chip 形式（如：× 数学  × 语文  + 添加）
    if (!m_isNormalGroup) {
        m_subjectTagContainer = new QWidget(groupSubject);
        m_subjectTagLayout = new QHBoxLayout(m_subjectTagContainer);
    m_subjectTagLayout->setContentsMargins(0, 0, 0, 0);
    m_subjectTagLayout->setSpacing(4);

    // “+ 添加”按钮（始终在最后）
    m_addSubjectBtn = new QPushButton(QString::fromUtf8(u8"+ 添加"), groupSubject);
    m_addSubjectBtn->setCursor(Qt::PointingHandCursor);
    m_addSubjectBtn->setFixedHeight(28);
    m_addSubjectBtn->setStyleSheet(
        "QPushButton { background-color: rgba(0,0,0,0.18); color: #ffffff; border: 1px solid rgba(255,255,255,0.16); border-radius: 14px; padding: 0 12px; }"
        "QPushButton:hover { background-color: rgba(25,118,210,0.35); }"
    );

    auto addTagIfNonEmpty = [=](const QString& text) {
        const QString t = text.trimmed();
        if (t.isEmpty()) {
            CustomMessageBox::warning(this, QString::fromUtf8(u8"提示"), QString::fromUtf8(u8"请先输入科目文本，再添加。"));
            return;
        }
        m_subjectsDirty = true;
        QWidget* tag = makeSubjectTagWidget(t);
        // 插到 “+ 添加” 之前
        int insertPos = m_subjectTagLayout->count();
        if (m_addSubjectBtn) {
            insertPos = m_subjectTagLayout->indexOf(m_addSubjectBtn);
            if (insertPos < 0) insertPos = m_subjectTagLayout->count();
        }
        m_subjectTagLayout->insertWidget(insertPos, tag, 0, Qt::AlignVCenter);
    };

    connect(m_addSubjectBtn, &QPushButton::clicked, this, [=]() {
        // 创建自定义输入对话框（无标题栏，有关闭按钮）
        QDialog* inputDlg = new QDialog(this);
        inputDlg->setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog | Qt::WindowStaysOnTopHint);
        inputDlg->setAttribute(Qt::WA_TranslucentBackground);
        inputDlg->setFixedSize(400, 180);
        
        // 创建主容器
        QWidget* container = new QWidget(inputDlg);
        container->setStyleSheet(
            "QWidget { background-color: #282A2B; border: 1px solid #282A2B; border-radius: 8px; }"
        );
        
        QVBoxLayout* containerLayout = new QVBoxLayout(inputDlg);
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
        btnClose->setVisible(false); // 初始隐藏
        connect(btnClose, &QPushButton::clicked, inputDlg, &QDialog::reject);
        
        // 标题文本
        QLabel* titleLabel = new QLabel(QString::fromUtf8(u8"添加任教科目"), titleBar);
        titleLabel->setStyleSheet("color: #ffffff; font-size: 14px; font-weight: bold; background: transparent; border: none;");
        titleLabel->setAlignment(Qt::AlignCenter);
        
        titleLayout->addWidget(titleLabel, 1);
        titleLayout->addWidget(btnClose);
        
        // 内容区域
        QWidget* contentWidget = new QWidget(container);
        contentWidget->setStyleSheet("background-color: #282A2B; border-bottom-left-radius: 8px; border-bottom-right-radius: 8px;");
        
        QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
        mainLayout->setContentsMargins(15, 10, 15, 10);
        mainLayout->setSpacing(7);
        
        // 提示文本
        QLabel* promptLabel = new QLabel(QString::fromUtf8(u8"请输入科目名称："), contentWidget);
        promptLabel->setStyleSheet("color: white; font-size: 14px; background: transparent;");
        
        // 输入框
        QLineEdit* lineEdit = new QLineEdit(contentWidget);
        lineEdit->setStyleSheet(
            "QLineEdit {"
            "background-color: #1e1e1e;"
            "border: 1px solid #404040;"
            "border-radius: 6px;"
            "color: #ffffff;"
            "font-size: 13px;"
            "padding: 8px;"
            "}"
            "QLineEdit:focus {"
            "border: 1px solid #4169E1;"
            "}"
        );
        lineEdit->setPlaceholderText(QString::fromUtf8(u8"请输入科目名称"));
        
        mainLayout->addWidget(promptLabel);
        mainLayout->addWidget(lineEdit);
        
        // 按钮布局
        QHBoxLayout* btnLayout = new QHBoxLayout();
        btnLayout->addStretch();
        
        QPushButton* btnCancel = new QPushButton(QString::fromUtf8(u8"取消"), contentWidget);
        btnCancel->setFixedSize(80, 35);
        btnCancel->setStyleSheet(
            "QPushButton {"
            "background-color: #2D2E2D;"
            "color: #ffffff;"
            "border: none;"
            "border-radius: 6px;"
            "font-size: 13px;"
            "}"
            "QPushButton:hover {"
            "background-color: #3D3E3D;"
            "}"
            "QPushButton:pressed {"
            "background-color: #1D1E1D;"
            "}"
        );
        connect(btnCancel, &QPushButton::clicked, inputDlg, &QDialog::reject);
        
        QPushButton* btnOk = new QPushButton(QString::fromUtf8(u8"确定"), contentWidget);
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
        connect(btnOk, &QPushButton::clicked, inputDlg, &QDialog::accept);
        connect(lineEdit, &QLineEdit::returnPressed, inputDlg, &QDialog::accept);
        
        btnLayout->addWidget(btnCancel);
        btnLayout->addWidget(btnOk);
        mainLayout->addLayout(btnLayout);
        
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
        
        CloseButtonEventFilter* filter = new CloseButtonEventFilter(inputDlg, btnClose);
        inputDlg->installEventFilter(filter);
        
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
        
        DragEventFilter* dragFilter = new DragEventFilter(inputDlg, titleBar);
        inputDlg->installEventFilter(dragFilter);
        
        // 居中显示
        QScreen* screen = QApplication::primaryScreen();
        if (screen) {
            QRect screenGeometry = screen->geometry();
            int x = (screenGeometry.width() - inputDlg->width()) / 2;
            int y = (screenGeometry.height() - inputDlg->height()) / 2;
            inputDlg->move(x, y);
        }
        
        // 设置焦点到输入框
        lineEdit->setFocus();
        
        bool ok = (inputDlg->exec() == QDialog::Accepted);
        QString text = ok ? lineEdit->text().trimmed() : QString();
        inputDlg->deleteLater();
        
        if (!ok || text.isEmpty()) return;
        addTagIfNonEmpty(text);
    });

    // 初始不预置科目，等待 /groups/members 返回的 teach_subjects 来刷新；用户也可手动添加
    m_subjectTagLayout->addStretch();
    m_subjectTagLayout->addWidget(m_addSubjectBtn, 0, Qt::AlignVCenter);

        subjectLayout->addWidget(m_subjectTagContainer);

    // 提示信息：放到 “+ 按钮”这一行下面
        QLabel* subjectTip = new QLabel(QString::fromUtf8(u8"任教科目：点击 + 按钮新增；点击科目左侧“×”删除。"), groupSubject);
        subjectTip->setStyleSheet("color: rgba(255,255,255,0.85); font-size: 12px;");
        subjectTip->setWordWrap(true);
        subjectLayout->addWidget(subjectTip);

    // 顶部关闭按钮：关闭前校验科目格式（done() 已兜底，这里只是给更快的反馈）
        if (m_closeButton) {
            disconnect(m_closeButton, nullptr, nullptr, nullptr);
            connect(m_closeButton, &QPushButton::clicked, this, [=]() {
                if (!validateSubjectFormat(true)) return;
                this->reject();
            });
        }

        mainLayout->addWidget(groupSubject);
    }

    // 班级群：开启对讲（普通群不显示）
    if (!m_isNormalGroup) {
        m_intercomWidget = new IntercomControlWidget(this);
        connect(m_intercomWidget, &IntercomControlWidget::intercomToggled, this, [this](bool enabled) {
            qDebug() << "对讲开关状态:" << (enabled ? "开启" : "关闭");
            // 发出信号通知父窗口（ScheduleDialog）更新对讲按钮状态
            emit intercomEnabledChanged(enabled);
        });
        connect(m_intercomWidget, &IntercomControlWidget::buttonClicked, this, []() {
            qDebug() << "开启对讲按钮被点击";
        });
        mainLayout->addWidget(m_intercomWidget);
    } else {
        m_intercomWidget = nullptr;
    }

    // 解散群聊 / 退出群聊
    // 如果按钮已经存在，先删除它们（防止重复创建）
    if (m_btnDismiss) {
        m_btnDismiss->deleteLater();
        m_btnDismiss = nullptr;
    }
    if (m_btnExit) {
        m_btnExit->deleteLater();
        m_btnExit = nullptr;
    }
    
    QHBoxLayout* bottomBtns = new QHBoxLayout;
    m_btnDismiss = new QPushButton("解散群聊", this);
    m_btnExit = new QPushButton("退出群聊", this);
    bottomBtns->addWidget(m_btnDismiss);
    bottomBtns->addWidget(m_btnExit);

    bottomBtns->setContentsMargins(10, 20, 10, 15);
    mainLayout->addLayout(bottomBtns);
    
    // 初始状态：默认都禁用，等InitGroupMember调用后再更新
    m_btnDismiss->setEnabled(false);
    m_btnExit->setEnabled(false);
    
    // 连接退出群聊按钮的点击事件
    connect(m_btnExit, &QPushButton::clicked, this, &QGroupInfo::onExitGroupClicked);
    // 连接解散群聊按钮的点击事件
    connect(m_btnDismiss, &QPushButton::clicked, this, &QGroupInfo::onDismissGroupClicked);
    
    // 标记为已初始化
    m_initialized = true;
}

void QGroupInfo::setGroupFaceUrl(const QString& faceUrl)
{
    m_groupFaceUrl = faceUrl;
    
    if (m_groupAvatarLabel && !faceUrl.isEmpty()) {
        // 使用网络管理器下载头像
        QNetworkAccessManager* manager = new QNetworkAccessManager(this);
        QUrl imageUrl(faceUrl);
        QNetworkRequest request(imageUrl);
        QNetworkReply* reply = manager->get(request);
        
        connect(reply, &QNetworkReply::finished, [this, reply, manager]() {
            if (reply->error() == QNetworkReply::NoError) {
                QPixmap pixmap;
                pixmap.loadFromData(reply->readAll());
                if (!pixmap.isNull()) {
                    m_groupAvatarLabel->setPixmap(pixmap.scaled(50, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                }
            }
            reply->deleteLater();
            manager->deleteLater();
        });
    }
}

void QGroupInfo::InitGroupMember(QString group_id, QVector<GroupMemberInfo> groupMemberInfo)
{
    // 普通群：如果网格还没渲染过（首次打开、列表为空等），不要被“参数相同早退”挡住
    const bool needInitialNormalRender = m_isNormalGroup && m_memberGridLayout && (m_memberGridLayout->count() == 0);

    // 检查传入的参数是否与“上一次真正渲染到 UI 的快照”相同
    // 不能用 m_groupMemberInfo 来和入参比较：调用方常常直接传 m_groupMemberInfo 本体，这会导致永远相同从而早退，UI 无法刷新
    if (!needInitialNormalRender &&
        m_hasRenderedGroupMembers &&
        m_lastRenderedGroupId == group_id &&
        m_lastRenderedGroupMemberInfo.size() == groupMemberInfo.size()) {

        // 顺序无关：只要 member_id 对应的关键字段完全一致，就认为“参数相同”
        const bool isSame = sameMemberListUnorderedForRender(m_lastRenderedGroupMemberInfo, groupMemberInfo);

        // 注意：当两边成员数量都为 0 时，也要继续往下走（用于触发UI刷新/清理），不能被“参数相同”早退挡住
        if (isSame && !groupMemberInfo.isEmpty()) {
            qDebug() << "QGroupInfo::InitGroupMember 参数相同（与上次渲染一致），跳过更新。群组ID:"
                     << group_id << "，成员数量:" << groupMemberInfo.size();
            return;
        }
    }
    
    m_groupNumberId = group_id;
    m_groupMemberInfo = groupMemberInfo;

    qDebug() << "QGroupInfo::InitGroupMember 被调用，群组ID:" << group_id << "，成员数量:" << groupMemberInfo.size();

    // 普通群：使用网格布局展示（头像在上、名字在下、末尾添加/删除）
    if (m_isNormalGroup) {
        renderNormalGroupMemberGrid();
        updateButtonStates();

        // 记录上一次渲染快照
        m_hasRenderedGroupMembers = true;
        m_lastRenderedGroupId = group_id;
        m_lastRenderedGroupMemberInfo = groupMemberInfo;
        return;
    }

    // 确保 circlesLayout 已初始化
    if (!circlesLayout) {
        qWarning() << "circlesLayout 未初始化，无法显示好友列表！";
        return;
    }
    
    // 找到 groupFriends 容器，成员按钮的父控件应该是它
    QGroupBox* groupFriends = nullptr;
    QWidget* parentWidget = circlesLayout->parentWidget();
    if (parentWidget) {
        groupFriends = qobject_cast<QGroupBox*>(parentWidget);
    }
    if (!groupFriends) {
        // 如果找不到，尝试从 circlesLayout 的父布局查找
        QLayout* parentLayout = qobject_cast<QLayout*>(circlesLayout->parent());
        if (parentLayout && parentLayout->parentWidget()) {
            groupFriends = qobject_cast<QGroupBox*>(parentLayout->parentWidget());
        }
    }
    // 如果还是找不到，使用 this 作为父控件
    if (!groupFriends) {
        // 尝试通过对象名查找
        QList<QGroupBox*> groupBoxes = this->findChildren<QGroupBox*>();
        for (QGroupBox* gb : groupBoxes) {
            if (gb->title() == "好友列表") {
                groupFriends = gb;
                break;
            }
        }
    }
    // 最后使用 this 作为父控件
    QWidget* buttonParent = groupFriends ? static_cast<QWidget*>(groupFriends) : this;
    qDebug() << "成员按钮的父控件:" << (groupFriends ? "groupFriends" : "this");

    // 清空之前的成员圆圈（保留 + 和 - 按钮）
    {
        // 遍历布局，找到 + 和 - 按钮，删除其他成员圆圈
        QList<QLayoutItem*> itemsToRemove;
        for (int i = 0; i < circlesLayout->count(); i++) {
            QLayoutItem* item = circlesLayout->itemAt(i);
            if (item && item->widget()) {
                FriendButton* btn = qobject_cast<FriendButton*>(item->widget());
                if (btn) {
                    if (btn->text() == "+") {
                        m_circlePlus = btn;
                        m_circlePlus->setContextMenuEnabled(false); // 禁用右键菜单
                    } else if (btn->text() == "-") {
                        m_circleMinus = btn;
                        m_circleMinus->setContextMenuEnabled(false); // 禁用右键菜单
                    } else {
                        // 成员圆圈，标记为删除
                        itemsToRemove.append(item);
                    }
                }
            }
        }
        
        // 删除成员圆圈
        for (auto item : itemsToRemove) {
            circlesLayout->removeItem(item);
            if (item->widget()) {
                item->widget()->deleteLater();
            }
            delete item;
        }
        
        // 如果 + 和 - 按钮不存在，创建它们
        if (!m_circlePlus) {
            m_circlePlus = new FriendButton("+", buttonParent);
            // FriendButton 构造函数已设置 setFixedSize(50, 50)，这里确保尺寸正确
            m_circlePlus->setFixedSize(50, 50);
            m_circlePlus->setMinimumSize(50, 50);
            m_circlePlus->setContextMenuEnabled(false); // 禁用右键菜单
            connect(m_circlePlus, &FriendButton::clicked, this, [this]() {
                if (m_friendSelectDlg)
                {
                    if (m_friendSelectDlg->isHidden())
                    {
                        // 获取当前群组成员ID列表，用于排除
                        QVector<QString> memberIds;
                        for (const auto& member : m_groupMemberInfo)
                        {
                            memberIds.append(member.member_id);
                        }
                        m_friendSelectDlg->setExcludedMemberIds(memberIds);
                        m_friendSelectDlg->setGroupId(m_groupNumberId); // 设置群组ID
                        m_friendSelectDlg->setGroupName(m_groupName); // 设置群组名称
                        m_friendSelectDlg->InitData();
                        m_friendSelectDlg->show();
                    }
                    else
                    {
                        m_friendSelectDlg->hide();
                    }
                }
            });
            circlesLayout->addWidget(m_circlePlus);
        }
        if (!m_circleMinus) {
            m_circleMinus = new FriendButton("-", buttonParent);
            // FriendButton 构造函数已设置 setFixedSize(50, 50)，这里确保尺寸正确
            m_circleMinus->setFixedSize(50, 50);
            m_circleMinus->setMinimumSize(50, 50);
            m_circleMinus->setContextMenuEnabled(false); // 禁用右键菜单
            connect(m_circleMinus, &FriendButton::clicked, this, [this]() {
                if (m_memberKickDlg)
                {
                    if (m_memberKickDlg->isHidden())
                    {
                        // 设置群组ID和名称
                        m_memberKickDlg->setGroupId(m_groupNumberId);
                        m_memberKickDlg->setGroupName(m_groupName);
                        // 初始化成员列表数据（排除群主）
                        m_memberKickDlg->InitData(m_groupMemberInfo);
                        m_memberKickDlg->show();
                    }
                    else
                    {
                        m_memberKickDlg->hide();
                    }
                }
            });
            circlesLayout->addWidget(m_circleMinus);
        }
    }

    // 检查成员数据是否为空
    if (m_groupMemberInfo.isEmpty()) {
        qDebug() << "成员列表为空，不显示任何成员";
        updateButtonStates();
        // 根据当前用户的 is_voice_enabled 更新对讲开关状态
        updateIntercomState();

        // 记录上一次渲染快照（空列表也算一次有效的“清理渲染”）
        m_hasRenderedGroupMembers = true;
        m_lastRenderedGroupId = group_id;
        m_lastRenderedGroupMemberInfo = groupMemberInfo;
        return;
    }

    QString redStyle = "background-color:red; border-radius:25px; color:white; font-weight:bold;";
    QString blueStyle = "background-color:blue; border-radius:25px; color:white; font-weight:bold;";

    // 当前用户是否为群主：统一使用外部传入的 iGroupOwner（m_iGroupOwner）
    const bool isOwner = m_iGroupOwner;

    // 找到 + 按钮的位置（应该在倒数第二个位置，- 按钮在最后）
    int plusButtonIndex = -1;
    int minusButtonIndex = -1;
    for (int i = 0; i < circlesLayout->count(); ++i) {
        QLayoutItem* item = circlesLayout->itemAt(i);
        if (item && item->widget()) {
            FriendButton* btn = qobject_cast<FriendButton*>(item->widget());
            if (btn) {
                if (btn->text() == "+") {
                    plusButtonIndex = i;
                } else if (btn->text() == "-") {
                    minusButtonIndex = i;
                }
            }
        }
    }

    // 确定插入位置：在 + 按钮之前插入，如果找不到 + 按钮，则在布局开始位置插入
    int insertIndex = (plusButtonIndex >= 0) ? plusButtonIndex : 0;
    qDebug() << "插入位置计算：+ 按钮索引:" << plusButtonIndex << "，- 按钮索引:" << minusButtonIndex << "，最终插入位置:" << insertIndex;
    
    // 使用之前找到的 buttonParent（应该是 groupFriends）
    // 第一步：先添加群主
    for (auto iter : m_groupMemberInfo)
    {
        if (iter.member_role == "群主")
        {
            FriendButton* circleBtn = new FriendButton("", buttonParent);
            // FriendButton 构造函数已设置 setFixedSize(50, 50)，这里确保尺寸正确
            circleBtn->setFixedSize(50, 50);
            circleBtn->setMinimumSize(50, 50);
            circleBtn->setStyleSheet(redStyle); // 群主用红色圆圈
            
            // 设置成员角色
            circleBtn->setMemberRole(iter.member_role);
            
            // 只有当前用户是群主时，才启用右键菜单
            circleBtn->setContextMenuEnabled(m_iGroupOwner);
            
            // 接收右键菜单信号，传递成员ID
            QString memberId = iter.member_id;
            connect(circleBtn, &FriendButton::setLeaderRequested, this, [this, memberId]() {
                onSetLeaderRequested(memberId);
            });
            connect(circleBtn, &FriendButton::cancelLeaderRequested, this, [this, memberId]() {
                onCancelLeaderRequested(memberId);
            });
            
            // 设置按钮文本（完整成员名字）
            circleBtn->setText(iter.member_name);
            circleBtn->setProperty("member_id", iter.member_id);
            
            // 确保按钮可见并显示
            circleBtn->setVisible(true);
            circleBtn->show();
            
            // 在 + 按钮之前插入成员圆圈
            circlesLayout->insertWidget(insertIndex, circleBtn);
            insertIndex++; // 更新插入位置
            
            qDebug() << "添加群主按钮:" << iter.member_name << "，角色:" << iter.member_role 
                     << "，插入位置:" << (insertIndex - 1) << "，按钮尺寸:" << circleBtn->size() 
                     << "，是否可见:" << circleBtn->isVisible() << "，父窗口:" << circleBtn->parent();
            break; // 只添加第一个群主
        }
    }
    
    // 第二步：添加班级成员（user_id == classid，且不是群主）
    // 班级成员应该显示在第二位（第一位是群主）
    if (!m_classId.isEmpty()) {
        for (auto iter : m_groupMemberInfo)
        {
            // 判断是否是班级成员：user_id 等于 classid，且不是群主
            if (iter.member_id == m_classId && iter.member_role != "群主")
            {
                FriendButton* circleBtn = new FriendButton("", buttonParent);
                // FriendButton 构造函数已设置 setFixedSize(50, 50)，这里确保尺寸正确
                circleBtn->setFixedSize(50, 50);
                circleBtn->setMinimumSize(50, 50);
                circleBtn->setStyleSheet(blueStyle); // 班级成员用蓝色圆圈
                
                // 设置成员角色
                circleBtn->setMemberRole(iter.member_role);
                
                // 班级成员不启用右键菜单
                circleBtn->setContextMenuEnabled(false);
                
                // 设置按钮文本（完整成员名字）
                circleBtn->setText(iter.member_name);
                circleBtn->setProperty("member_id", iter.member_id);
                
                // 确保按钮可见并显示
                circleBtn->setVisible(true);
                circleBtn->show();
                
                // 在 + 按钮之前插入成员圆圈（在群主之后）
                circlesLayout->insertWidget(insertIndex, circleBtn);
                insertIndex++; // 更新插入位置
                
                qDebug() << "添加班级成员按钮:" << iter.member_name << "，user_id:" << iter.member_id 
                         << "，classid:" << m_classId << "，插入位置:" << (insertIndex - 1);
                break; // 只添加第一个班级成员
            }
        }
    }
    
    // 第三步：添加其他成员（非群主且非班级成员）
    for (auto iter : m_groupMemberInfo)
    {
        // 排除群主和班级成员
        bool isOwner = (iter.member_role == "群主");
        bool isClassMember = (!m_classId.isEmpty() && iter.member_id == m_classId);
        
        if (!isOwner && !isClassMember)
        {
            FriendButton* circleBtn = new FriendButton("", buttonParent);
            // FriendButton 构造函数已设置 setFixedSize(50, 50)，这里确保尺寸正确
            circleBtn->setFixedSize(50, 50);
            circleBtn->setMinimumSize(50, 50);
            circleBtn->setStyleSheet(blueStyle); // 其他成员用蓝色圆圈
            
            // 设置成员角色
            circleBtn->setMemberRole(iter.member_role);
            
            // 只有当前用户是群主时，才启用右键菜单
            circleBtn->setContextMenuEnabled(m_iGroupOwner);
            
            // 接收右键菜单信号，传递成员ID
            QString memberId = iter.member_id;
            connect(circleBtn, &FriendButton::setLeaderRequested, this, [this, memberId]() {
                onSetLeaderRequested(memberId);
            });
            connect(circleBtn, &FriendButton::cancelLeaderRequested, this, [this, memberId]() {
                onCancelLeaderRequested(memberId);
            });
            
            // 设置按钮文本（完整成员名字）
            circleBtn->setText(iter.member_name);
            circleBtn->setProperty("member_id", iter.member_id);
            
            // 确保按钮可见并显示
            circleBtn->setVisible(true);
            circleBtn->show();
            
            // 在 + 按钮之前插入成员圆圈
            circlesLayout->insertWidget(insertIndex, circleBtn);
            insertIndex++; // 更新插入位置
            
            qDebug() << "添加成员按钮:" << iter.member_name << "，角色:" << iter.member_role 
                     << "，插入位置:" << (insertIndex - 1) << "，按钮尺寸:" << circleBtn->size() 
                     << "，是否可见:" << circleBtn->isVisible() << "，父窗口:" << circleBtn->parent();
        }
    }
    
    // 确保所有按钮都可见并强制刷新布局
    qDebug() << "开始检查布局中的所有按钮，布局项总数:" << circlesLayout->count();
    /*for (int i = 0; i < circlesLayout->count(); ++i) {
        QLayoutItem* item = circlesLayout->itemAt(i);
        if (item && item->widget()) {
            QWidget* widget = item->widget();
            FriendButton* btn = qobject_cast<FriendButton*>(widget);
            if (btn) {
                widget->setVisible(true);
                widget->show();
                widget->raise();
                widget->update();
                qDebug() << "按钮[" << i << "] 文本:" << btn->text() << "，尺寸:" << widget->size() 
                         << "，可见:" << widget->isVisible() << "，父窗口:" << widget->parent();
            } else {
                widget->setVisible(true);
                widget->show();
                widget->update();
            }
        }
    }*/
    
    // 强制刷新布局和父容器
    //circlesLayout->update();
    //QWidget* parentWidget = circlesLayout->parentWidget();
    //if (parentWidget) {
    //    parentWidget->update();
    //    parentWidget->repaint();
    //    qDebug() << "父容器已刷新:" << parentWidget->objectName() << "，尺寸:" << parentWidget->size();
    //}
    //

    qDebug() << "好友列表显示完成，共添加" << m_groupMemberInfo.size() << "个成员，当前布局项数:" << circlesLayout->count();
    
    // 更新按钮状态（根据当前用户是否是群主）
    updateButtonStates();
    
    // 根据当前用户的 is_voice_enabled 更新对讲开关状态
    updateIntercomState();

    // 从成员列表中取出当前老师的 teach_subjects，刷新到“任教科目”区域
    // 如果用户正在编辑科目（m_subjectsDirty==true），则不覆盖本地编辑
    if (!m_subjectsDirty && m_subjectTagLayout) {
        UserInfo userInfo = CommonInfo::GetData();
        QString currentUserId = userInfo.teacher_unique_id;
        QStringList subjectsFromServer;
        for (const auto& member : m_groupMemberInfo) {
            if (member.member_id == currentUserId) {
                subjectsFromServer = member.teach_subjects;
                break;
            }
        }
        setTeachSubjectsInUI(subjectsFromServer);
    }

    // 刷新整个对话框
    this->update();
    this->repaint();

    // 记录上一次渲染快照（用于下次“参数相同早退”判断）
    m_hasRenderedGroupMembers = true;
    m_lastRenderedGroupId = group_id;
    m_lastRenderedGroupMemberInfo = groupMemberInfo;
}

QWidget* QGroupInfo::makeMemberTile(const QString& topText, const QString& bottomText, bool isActionTile)
{
    QWidget* tile = new QWidget(m_memberGridContainer ? m_memberGridContainer : this);
    QVBoxLayout* v = new QVBoxLayout(tile);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(6);

    FriendButton* btn = new FriendButton(topText, tile);
    btn->setFixedSize(50, 50);
    btn->setMinimumSize(50, 50);
    btn->setContextMenuEnabled(false);
    btn->setCursor(Qt::PointingHandCursor);
    if (isActionTile) {
        btn->setStyleSheet("background-color: rgba(255,255,255,0.06); border: 1px dashed rgba(255,255,255,0.20); border-radius:25px; color:white; font-weight:800;");
    } else {
        btn->setStyleSheet("background-color: rgba(255,255,255,0.10); border: 1px solid rgba(255,255,255,0.10); border-radius:25px; color:white; font-weight:800;");
    }

    QLabel* lbl = new QLabel(bottomText, tile);
    lbl->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    lbl->setFixedWidth(56);
    lbl->setWordWrap(false);
    lbl->setToolTip(bottomText);
    lbl->setStyleSheet("color: rgba(255,255,255,0.75); font-size: 11px;");

    v->addWidget(btn, 0, Qt::AlignHCenter);
    v->addWidget(lbl, 0, Qt::AlignHCenter);
    return tile;
}

void QGroupInfo::renderNormalGroupMemberGrid()
{
    if (!m_memberGridLayout || !m_memberGridContainer) {
        qWarning() << "普通群成员网格未初始化，无法渲染。";
        return;
    }

    // 清空旧的 tile
    while (m_memberGridLayout->count() > 0) {
        QLayoutItem* it = m_memberGridLayout->takeAt(0);
        if (it) {
            if (it->widget()) it->widget()->deleteLater();
            delete it;
        }
    }

    auto firstChar = [](const QString& name) -> QString {
        QString t = name.trimmed();
        if (t.isEmpty()) return "?";
        return t.left(1);
    };

    const int columns = 6; // 视觉上接近微信：一行 6 个（含 + / -）
    int idx = 0;

    // 成员 tile
    for (const auto& m : m_groupMemberInfo) {
        QWidget* tile = makeMemberTile(firstChar(m.member_name), m.member_name, false);
        // 让按钮点击可以显示成员信息（后续可扩展）
        if (FriendButton* btn = tile->findChild<FriendButton*>()) {
            btn->setProperty("member_id", m.member_id);
            btn->setToolTip(m.member_name);
        }

        const int row = idx / columns;
        const int col = idx % columns;
        m_memberGridLayout->addWidget(tile, row, col, Qt::AlignTop);
        idx++;
    }

    // 添加 / 删除 tile（固定最后两个）
    QWidget* addTile = makeMemberTile("+", QStringLiteral("添加"), true);
    if (FriendButton* addBtn = addTile->findChild<FriendButton*>()) {
        connect(addBtn, &QPushButton::clicked, this, [this]() {
            if (!m_friendSelectDlg) return;
            if (m_friendSelectDlg->isHidden())
            {
                QVector<QString> memberIds;
                for (const auto& member : m_groupMemberInfo)
                {
                    memberIds.append(member.member_id);
                }
                m_friendSelectDlg->setExcludedMemberIds(memberIds);
                m_friendSelectDlg->setGroupId(m_groupNumberId);
                m_friendSelectDlg->setGroupName(m_groupName);
                m_friendSelectDlg->InitData();
                m_friendSelectDlg->show();
            }
            else
            {
                m_friendSelectDlg->hide();
            }
        });
    }

    QWidget* delTile = makeMemberTile("-", QStringLiteral("删除"), true);
    if (FriendButton* delBtn = delTile->findChild<FriendButton*>()) {
        connect(delBtn, &QPushButton::clicked, this, [this]() {
            if (!m_memberKickDlg) return;
            if (m_memberKickDlg->isHidden())
            {
                m_memberKickDlg->setGroupId(m_groupNumberId);
                m_memberKickDlg->setGroupName(m_groupName);
                m_memberKickDlg->InitData(m_groupMemberInfo);
                m_memberKickDlg->show();
            }
            else
            {
                m_memberKickDlg->hide();
            }
        });
    }

    {
        const int row = idx / columns;
        const int col = idx % columns;
        m_memberGridLayout->addWidget(addTile, row, col, Qt::AlignTop);
        idx++;
    }
    {
        const int row = idx / columns;
        const int col = idx % columns;
        m_memberGridLayout->addWidget(delTile, row, col, Qt::AlignTop);
        idx++;
    }

    // 让底部留一点空白
    m_memberGridLayout->setRowStretch((idx / columns) + 1, 1);
}


void QGroupInfo::InitGroupMember()
{
    // circlesLayout 未初始化时，无法刷新成员圆圈
    if (!circlesLayout) {
        qWarning() << "circlesLayout 未初始化，无法显示好友列表！";
        return;
    }
    // 注意：当成员数量为 0 时也要继续往下走，用于清理旧UI（避免残留上一轮成员）

    //m_groupNumberId = group_id;
    //m_groupMemberInfo = groupMemberInfo;

    // 清空之前的成员圆圈（保留 + 和 - 按钮）
    if (circlesLayout) {
        // 遍历布局，找到 + 和 - 按钮，删除其他成员圆圈
        QList<QLayoutItem*> itemsToRemove;
        for (int i = 0; i < circlesLayout->count(); i++) {
            QLayoutItem* item = circlesLayout->itemAt(i);
            if (item && item->widget()) {
                FriendButton* btn = qobject_cast<FriendButton*>(item->widget());
                if (btn) {
                    if (btn->text() == "+") {
                        m_circlePlus = btn;
                        m_circlePlus->setContextMenuEnabled(false); // 禁用右键菜单
                    }
                    else if (btn->text() == "-") {
                        m_circleMinus = btn;
                        m_circleMinus->setContextMenuEnabled(false); // 禁用右键菜单
                    }
                    else {
                        // 成员圆圈，标记为删除
                        itemsToRemove.append(item);
                    }
                }
            }
        }

        // 删除成员圆圈
        for (auto item : itemsToRemove) {
            circlesLayout->removeItem(item);
            if (item->widget()) {
                item->widget()->deleteLater();
            }
            delete item;
        }

        // 找到 groupFriends 容器，按钮的父控件应该是它
        QGroupBox* groupFriends = nullptr;
        QWidget* parentWidget = circlesLayout->parentWidget();
        if (parentWidget) {
            groupFriends = qobject_cast<QGroupBox*>(parentWidget);
        }
        if (!groupFriends) {
            QLayout* parentLayout = qobject_cast<QLayout*>(circlesLayout->parent());
            if (parentLayout && parentLayout->parentWidget()) {
                groupFriends = qobject_cast<QGroupBox*>(parentLayout->parentWidget());
            }
        }
        if (!groupFriends) {
            QList<QGroupBox*> groupBoxes = this->findChildren<QGroupBox*>();
            for (QGroupBox* gb : groupBoxes) {
                if (gb->title() == "好友列表") {
                    groupFriends = gb;
                    break;
                }
            }
        }
        QWidget* buttonParent = groupFriends ? static_cast<QWidget*>(groupFriends) : this;
        
        // 如果 + 和 - 按钮不存在，创建它们
        if (!m_circlePlus) {
            m_circlePlus = new FriendButton("+", buttonParent);
            m_circlePlus->setFixedSize(50, 50);
            m_circlePlus->setMinimumSize(50, 50);
            m_circlePlus->setContextMenuEnabled(false); // 禁用右键菜单
            connect(m_circlePlus, &FriendButton::clicked, this, [this]() {
                if (m_friendSelectDlg)
                {
                    if (m_friendSelectDlg->isHidden())
                    {
                        // 获取当前群组成员ID列表，用于排除
                        QVector<QString> memberIds;
                        for (const auto& member : m_groupMemberInfo)
                        {
                            memberIds.append(member.member_id);
                        }
                        m_friendSelectDlg->setExcludedMemberIds(memberIds);
                        m_friendSelectDlg->setGroupId(m_groupNumberId); // 设置群组ID
                        m_friendSelectDlg->setGroupName(m_groupName); // 设置群组名称
                        m_friendSelectDlg->InitData();
                        m_friendSelectDlg->show();
                    }
                    else
                    {
                        m_friendSelectDlg->hide();
                    }
                }
                });
            circlesLayout->addWidget(m_circlePlus);
        }
        if (!m_circleMinus) {
            m_circleMinus = new FriendButton("-", buttonParent);
            // FriendButton 构造函数已设置 setFixedSize(50, 50)，这里确保尺寸正确
            m_circleMinus->setFixedSize(50, 50);
            m_circleMinus->setMinimumSize(50, 50);
            m_circleMinus->setContextMenuEnabled(false); // 禁用右键菜单
            connect(m_circleMinus, &FriendButton::clicked, this, [this]() {
                if (m_memberKickDlg)
                {
                    if (m_memberKickDlg->isHidden())
                    {
                        // 设置群组ID和名称
                        m_memberKickDlg->setGroupId(m_groupNumberId);
                        m_memberKickDlg->setGroupName(m_groupName);
                        // 初始化成员列表数据（排除群主）
                        m_memberKickDlg->InitData(m_groupMemberInfo);
                        m_memberKickDlg->show();
                    }
                    else
                    {
                        m_memberKickDlg->hide();
                    }
                }
                });
            circlesLayout->addWidget(m_circleMinus);
        }
    }

    QString redStyle = "background-color:red; border-radius:25px; color:white; font-weight:bold;";
    QString blueStyle = "background-color:blue; border-radius:25px; color:white; font-weight:bold;";

    // 当前用户是否为群主：统一使用外部传入的 iGroupOwner（m_iGroupOwner）
    const bool isOwner = m_iGroupOwner;

    // 找到 + 按钮的位置（应该在倒数第二个位置，- 按钮在最后）
    int plusButtonIndex = -1;
    int minusButtonIndex = -1;
    for (int i = 0; i < circlesLayout->count(); ++i) {
        QLayoutItem* item = circlesLayout->itemAt(i);
        if (item && item->widget()) {
            FriendButton* btn = qobject_cast<FriendButton*>(item->widget());
            if (btn) {
                if (btn->text() == "+") {
                    plusButtonIndex = i;
                } else if (btn->text() == "-") {
                    minusButtonIndex = i;
                }
            }
        }
    }

    // 找到 groupFriends 容器，成员按钮的父控件应该是它
    QGroupBox* groupFriends = nullptr;
    QWidget* parentWidget = circlesLayout->parentWidget();
    if (parentWidget) {
        groupFriends = qobject_cast<QGroupBox*>(parentWidget);
    }
    if (!groupFriends) {
        // 如果找不到，尝试从 circlesLayout 的父布局查找
        QLayout* parentLayout = qobject_cast<QLayout*>(circlesLayout->parent());
        if (parentLayout && parentLayout->parentWidget()) {
            groupFriends = qobject_cast<QGroupBox*>(parentLayout->parentWidget());
        }
    }
    // 如果还是找不到，使用 this 作为父控件
    if (!groupFriends) {
        // 尝试通过对象名查找
        QList<QGroupBox*> groupBoxes = this->findChildren<QGroupBox*>();
        for (QGroupBox* gb : groupBoxes) {
            if (gb->title() == "好友列表") {
                groupFriends = gb;
                break;
            }
        }
    }
    // 最后使用 this 作为父控件
    QWidget* buttonParent = groupFriends ? static_cast<QWidget*>(groupFriends) : this;
    qDebug() << "成员按钮的父控件:" << (groupFriends ? "groupFriends" : "this");
    
    // 确定插入位置：在 + 按钮之前插入，如果找不到 + 按钮，则在布局开始位置插入
    int insertIndex = (plusButtonIndex >= 0) ? plusButtonIndex : 0;
    qDebug() << "插入位置计算：+ 按钮索引:" << plusButtonIndex << "，- 按钮索引:" << minusButtonIndex << "，最终插入位置:" << insertIndex;
    
    for (auto iter : m_groupMemberInfo)
    {
        FriendButton* circleBtn = nullptr;
        if (iter.member_role == "群主")
        {
            circleBtn = new FriendButton("", buttonParent);
            // FriendButton 构造函数已设置 setFixedSize(50, 50)，这里确保尺寸正确
            circleBtn->setFixedSize(50, 50);
            circleBtn->setMinimumSize(50, 50);
            circleBtn->setStyleSheet(redStyle); // 群主用红色圆圈
        }
        else
        {
            circleBtn = new FriendButton("", buttonParent);
            // FriendButton 构造函数已设置 setFixedSize(50, 50)，这里确保尺寸正确
            circleBtn->setFixedSize(50, 50);
            circleBtn->setMinimumSize(50, 50);
            circleBtn->setStyleSheet(blueStyle); // 其他成员用蓝色圆圈
        }
        
        // 设置成员角色
        circleBtn->setMemberRole(iter.member_role);
        
        // 只有当前用户是群主时，才启用右键菜单
        circleBtn->setContextMenuEnabled(m_iGroupOwner);
        
        // 接收右键菜单信号，传递成员ID
        QString memberId = iter.member_id;
        connect(circleBtn, &FriendButton::setLeaderRequested, this, [this, memberId]() {
            onSetLeaderRequested(memberId);
        });
        connect(circleBtn, &FriendButton::cancelLeaderRequested, this, [this, memberId]() {
            onCancelLeaderRequested(memberId);
        });
        
        circleBtn->setVisible(true);
        circleBtn->show();
        circleBtn->setText(iter.member_name);
        circleBtn->setProperty("member_id", iter.member_id);
        
        // 在 + 按钮之前插入成员圆圈
        circlesLayout->insertWidget(insertIndex, circleBtn);
        insertIndex++; // 更新插入位置
        
        qDebug() << "添加成员按钮:" << iter.member_name << "，角色:" << iter.member_role << "，插入位置:" << (insertIndex - 1);
    }

    // 更新按钮状态（根据当前用户是否是群主）
    updateButtonStates();
    
    // 根据当前用户的 is_voice_enabled 更新对讲开关状态
    updateIntercomState();

    // 从成员列表中取出当前老师的 teach_subjects，刷新到“任教科目”区域
    // 如果用户正在编辑科目（m_subjectsDirty==true），则不覆盖本地编辑
    if (!m_subjectsDirty && m_subjectTagLayout) {
        UserInfo userInfo = CommonInfo::GetData();
        QString currentUserId = userInfo.teacher_unique_id;
        QStringList subjectsFromServer;
        for (const auto& member : m_groupMemberInfo) {
            if (member.member_id == currentUserId) {
                subjectsFromServer = member.teach_subjects;
                break;
            }
        }
        setTeachSubjectsInUI(subjectsFromServer);
    }
}

void QGroupInfo::updateIntercomState()
{
    if (!m_intercomWidget) return;
    
    // 获取当前用户信息
    UserInfo userInfo = CommonInfo::GetData();
    QString currentUserId = userInfo.teacher_unique_id;
    
    // 在成员列表中查找当前用户
    bool isVoiceEnabled = false;
    for (const auto& member : m_groupMemberInfo) {
        if (member.member_id == currentUserId) {
            isVoiceEnabled = member.is_voice_enabled;
            break;
        }
    }
    
    // 根据 is_voice_enabled 设置开关状态
    m_intercomWidget->setIntercomEnabled(isVoiceEnabled);
    
    qDebug() << "更新对讲开关状态，当前用户ID:" << currentUserId << "，is_voice_enabled:" << isVoiceEnabled;
}

void QGroupInfo::updateButtonStates()
{
    // 当前用户是否为群主：统一使用外部传入的 iGroupOwner（m_iGroupOwner）
    const bool isOwner = m_iGroupOwner;
    
    // 根据当前用户是否是群主来设置按钮状态
    if (m_btnDismiss && m_btnExit) {
        if (m_isNormalGroup) {
            // 普通群：按腾讯SDK规则，群主不能退出（只能解散）；成员可退出
            m_btnDismiss->setEnabled(isOwner);
            m_btnExit->setEnabled(!isOwner);
            qDebug() << "普通群按钮状态，isOwner:" << isOwner << "dismiss:" << m_btnDismiss->isEnabled() << "exit:" << m_btnExit->isEnabled();
        } else {
            // 班级群：保留现有逻辑（群主可转让后退出）
            if (isOwner) {
                m_btnDismiss->setEnabled(true);
                m_btnExit->setEnabled(true);
                qDebug() << "当前用户是群主，解散群聊按钮可用，退出群聊按钮可用（需要先转让群主）";
            } else {
                m_btnDismiss->setEnabled(false);
                m_btnExit->setEnabled(true);
                qDebug() << "当前用户不是群主，解散群聊按钮禁用，退出群聊按钮可用";
            }
        }
    }
}

void QGroupInfo::onExitGroupClicked()
{
    if (m_groupNumberId.isEmpty()) {
        CustomMessageBox::warning(this, "错误", "群组ID为空，无法退出群聊");
        return;
    }
    
    // 获取当前用户信息
    UserInfo userInfo = CommonInfo::GetData();
    QString userId = userInfo.teacher_unique_id;
    QString userName = userInfo.strName;
    
    // 当前用户是否为群主：统一使用外部传入的 iGroupOwner（m_iGroupOwner）
    const bool isOwner = m_iGroupOwner;

    // 普通群：不走自建服务器，直接调用腾讯 IM SDK
    if (m_isNormalGroup) {
        if (isOwner) {
            CustomMessageBox::information(this, "提示", "普通群群主不能退出群聊，请使用“解散群聊”。");
            return;
        }

        int ret = CustomMessageBox::question(this, "确认退出",
            QString("确定要退出群聊 \"%1\" 吗？").arg(m_groupName),
            CustomMessageBox::StandardButtons(CustomMessageBox::Yes | CustomMessageBox::No));
        if (ret != CustomMessageBox::Yes) {
            return;
        }

        struct QuitGroupSDKCbData {
            QPointer<QGroupInfo> dlg;
            QString groupId;
            QString userId;
        };
        QuitGroupSDKCbData* cbData = new QuitGroupSDKCbData;
        cbData->dlg = this;
        cbData->groupId = m_groupNumberId;
        cbData->userId = userId;

        const QByteArray gid = m_groupNumberId.toUtf8();
        int callRet = TIMGroupQuit(gid.constData(),
            [](int32_t code, const char* desc, const char* /*json_param*/, const void* user_data) {
                QuitGroupSDKCbData* d = (QuitGroupSDKCbData*)user_data;
                if (!d) return;
                if (!d->dlg) { delete d; return; }

                const QPointer<QGroupInfo> dlg = d->dlg;
                const QString groupId = d->groupId;
                const QString userId = d->userId;
                const QString errDesc = QString::fromUtf8(desc ? desc : "");

                if (dlg) {
                    QMetaObject::invokeMethod(dlg, [dlg, groupId, userId, code, errDesc]() {
                        if (!dlg) return;
                        if (code != 0) {
                            QString err = QString("退出群聊失败\n错误码: %1\n错误描述: %2").arg(code).arg(errDesc);
                            CustomMessageBox::warning(dlg, "退出失败", err);
                            return;
                        }

                        // 更新本地列表并通知外部
                        QVector<GroupMemberInfo> updated;
                        for (const auto& m : dlg->m_groupMemberInfo) {
                            if (m.member_id != userId) updated.append(m);
                        }
                        dlg->m_groupMemberInfo = updated;
                        dlg->InitGroupMember(groupId, dlg->m_groupMemberInfo);

                        emit dlg->memberLeftGroup(groupId, userId);
                        CustomMessageBox::information(dlg, "退出成功", QString("已成功退出群聊 \"%1\"！").arg(dlg->m_groupName));
                        dlg->accept();
                    }, Qt::QueuedConnection);
                }
                delete d;
            },
            cbData);

        if (callRet != TIM_SUCC) {
            delete cbData;
            CustomMessageBox::warning(this, "退出失败", QString("TIMGroupQuit 调用失败，错误码: %1").arg(callRet));
        }
        return;
    }
    
    // 如果是群主，需要检查是否有其他管理员
    if (isOwner) {
        // 查找所有管理员（排除群主自己）
        QVector<GroupMemberInfo> adminList;
        for (const auto& member : m_groupMemberInfo) {
            if (member.member_id != userId && member.member_role == "管理员") {
                adminList.append(member);
            }
        }
        
        // 如果没有其他管理员，提示错误
        if (adminList.isEmpty()) {
            CustomMessageBox::warning(this, "无法退出", "只有一个班主任。请先设置新班主任，再退出。");
            return;
        }
        
        // 有管理员，按好友列表顺序排序（使用成员名称排序）
        std::sort(adminList.begin(), adminList.end(), [](const GroupMemberInfo& a, const GroupMemberInfo& b) {
            return a.member_name < b.member_name;
        });
        
        // 获取第一个管理员
        QString newOwnerId = adminList[0].member_id;
        QString newOwnerName = adminList[0].member_name;
        
        // 确认对话框
        int ret = CustomMessageBox::question(this, "确认退出", 
            QString("确定要退出群聊 \"%1\" 吗？\n\n退出前将自动将群主身份转让给 %2（第一个管理员）。").arg(m_groupName).arg(newOwnerName),
            CustomMessageBox::StandardButtons(CustomMessageBox::Yes | CustomMessageBox::No));
        
        if (ret != CustomMessageBox::Yes) {
            return;
        }
        
        // 转让群主并退出
        transferOwnerAndQuit(newOwnerId, newOwnerName);
        return;
    }
    
    // 不是群主，直接退出
    // 确认对话框
    int ret = CustomMessageBox::question(this, "确认退出", 
        QString("确定要退出群聊 \"%1\" 吗？").arg(m_groupName),
        CustomMessageBox::StandardButtons(CustomMessageBox::Yes | CustomMessageBox::No));
    
    if (ret != CustomMessageBox::Yes) {
        return;
    }
    
    qDebug() << "开始退出群聊，群组ID:" << m_groupNumberId << "，用户ID:" << userId;
    
    // 构造回调数据结构
    struct ExitGroupCallbackData {
        QGroupInfo* dlg;
        QString groupId;
        QString userId;
        QString userName;
    };
    
    ExitGroupCallbackData* callbackData = new ExitGroupCallbackData;
    callbackData->dlg = this;
    callbackData->groupId = m_groupNumberId;
    callbackData->userId = userId;
    callbackData->userName = userName;
    
    // 检查REST API是否初始化
    if (!m_restAPI) {
        CustomMessageBox::critical(this, "错误", "REST API未初始化！");
        delete callbackData;
        return;
    }
    
    // 在使用REST API前设置管理员账号信息
    // 注意：REST API需要使用应用管理员账号，使用当前登录用户的teacher_unique_id
    std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
    if (!adminUserId.empty()) {
        std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
        m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
    }
    
    // 调用REST API退出群聊接口
    m_restAPI->quitGroup(m_groupNumberId, userId,
        [=](int errorCode, const QString& errorDesc, const QJsonObject& result) {
            if (errorCode != 0) {
                QString errorMsg = QString("退出群聊失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
                qDebug() << errorMsg;
                CustomMessageBox::critical(callbackData->dlg, "退出失败", errorMsg);
                delete callbackData;
                return;
            }
            
            qDebug() << "REST API退出群聊成功:" << callbackData->groupId;
            
            // 立即从本地成员列表中移除当前用户
            QVector<GroupMemberInfo> updatedMemberList;
            QString leftUserId = callbackData->userId; // 记录退出的用户ID
            for (const auto& member : callbackData->dlg->m_groupMemberInfo) {
                if (member.member_id != callbackData->userId) {
                    updatedMemberList.append(member);
                }
            }
            callbackData->dlg->m_groupMemberInfo = updatedMemberList;
            
            // 立即更新UI（移除退出的成员）
            callbackData->dlg->InitGroupMember(callbackData->dlg->m_groupNumberId, callbackData->dlg->m_groupMemberInfo);
            
            // REST API成功，现在调用自己的服务器接口（传递退出的用户ID，用于后续处理）
            callbackData->dlg->sendExitGroupRequestToServer(callbackData->groupId, callbackData->userId, leftUserId);
            
            // 释放回调数据
            delete callbackData;
        });
}

void QGroupInfo::sendExitGroupRequestToServer(const QString& groupId, const QString& userId, const QString& leftUserId)
{
    // 构造发送到服务器的JSON数据
    QJsonObject requestData;
    requestData["group_id"] = groupId;
    requestData["user_id"] = userId;
    
    // 转换为JSON字符串
    QJsonDocument doc(requestData);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 发送POST请求到服务器
    QString url = "http://47.100.126.194:5000/groups/leave";
    
    // 使用QNetworkAccessManager发送POST请求
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest networkRequest;
    networkRequest.setUrl(QUrl(url));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = manager->post(networkRequest, jsonData);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qDebug() << "服务器响应:" << QString::fromUtf8(response);
            
            // 解析响应
            QJsonParseError parseError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &parseError);
            if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                if (obj["code"].toInt() == 200) {
                    qDebug() << "退出群聊请求已发送到服务器";
                    
                    // 服务器响应成功，发出成员退出群聊信号，通知父窗口刷新成员列表（传递退出的用户ID）
                    emit this->memberLeftGroup(groupId, leftUserId);
                    
                    // 显示成功消息
                    CustomMessageBox::information(this, "退出成功", 
                        QString("已成功退出群聊 \"%1\"！").arg(m_groupName));
                    
                    // 关闭对话框
                    this->accept();
                } else {
                    QString message = obj["message"].toString();
                    qDebug() << "服务器返回错误:" << message;
                    CustomMessageBox::warning(this, "退出失败", 
                        QString("服务器返回错误: %1").arg(message));
                }
            } else {
                qDebug() << "解析服务器响应失败";
                CustomMessageBox::warning(this, "退出失败", "解析服务器响应失败");
            }
        } else {
            qDebug() << "发送退出群聊请求到服务器失败:" << reply->errorString();
            CustomMessageBox::warning(this, "退出失败", 
                QString("网络错误: %1").arg(reply->errorString()));
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

void QGroupInfo::onDismissGroupClicked()
{
    if (m_groupNumberId.isEmpty()) {
        CustomMessageBox::warning(this, "错误", "群组ID为空，无法解散群聊");
        return;
    }
    
    // 确认对话框
    int ret = CustomMessageBox::question(this, "确认解散", 
        QString("确定要解散群聊 \"%1\" 吗？\n解散后所有成员将被移除，此操作不可恢复！").arg(m_groupName),
        CustomMessageBox::StandardButtons(CustomMessageBox::Yes | CustomMessageBox::No));
    
    if (ret != CustomMessageBox::Yes) {
        return;
    }

    // 普通群：不走自建服务器，直接调用腾讯 IM SDK
    if (m_isNormalGroup) {
        struct DeleteGroupSDKCbData {
            QPointer<QGroupInfo> dlg;
            QString groupId;
        };
        DeleteGroupSDKCbData* cbData = new DeleteGroupSDKCbData;
        cbData->dlg = this;
        cbData->groupId = m_groupNumberId;

        const QByteArray gid = m_groupNumberId.toUtf8();
        int callRet = TIMGroupDelete(gid.constData(),
            [](int32_t code, const char* desc, const char* /*json_param*/, const void* user_data) {
                DeleteGroupSDKCbData* d = (DeleteGroupSDKCbData*)user_data;
                if (!d) return;
                if (!d->dlg) { delete d; return; }

                const QPointer<QGroupInfo> dlg = d->dlg;
                const QString groupId = d->groupId;
                const QString errDesc = QString::fromUtf8(desc ? desc : "");

                if (dlg) {
                    QMetaObject::invokeMethod(dlg, [dlg, groupId, code, errDesc]() {
                        if (!dlg) return;
                        if (code != 0) {
                            QString err = QString("解散群聊失败\n错误码: %1\n错误描述: %2").arg(code).arg(errDesc);
                            CustomMessageBox::warning(dlg, "解散失败", err);
                            return;
                        }

                        emit dlg->groupDismissed(groupId);
                        CustomMessageBox::information(dlg, "解散成功", QString("已成功解散群聊 \"%1\"！").arg(dlg->m_groupName));
                        dlg->accept();
                    }, Qt::QueuedConnection);
                }
                delete d;
            },
            cbData);

        if (callRet != TIM_SUCC) {
            delete cbData;
            CustomMessageBox::warning(this, "解散失败", QString("TIMGroupDelete 调用失败，错误码: %1").arg(callRet));
        }
        return;
    }
    
    // 获取当前用户信息
    UserInfo userInfo = CommonInfo::GetData();
    QString userId = userInfo.teacher_unique_id;
    QString userName = userInfo.strName;
    
    qDebug() << "开始解散群聊，群组ID:" << m_groupNumberId << "，用户ID:" << userId;
    
    // 使用腾讯IM接口解散群组
    if (!m_restAPI) {
        CustomMessageBox::warning(this, "错误", "REST API未初始化");
        return;
    }
    
    // 设置管理员账号信息（REST API需要使用应用管理员账号）
    std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
    if (adminUserId.empty()) {
        CustomMessageBox::warning(this, "错误", "管理员账号ID未设置，无法调用REST API");
        return;
    }
    std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
    m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
    
    m_restAPI->destroyGroup(m_groupNumberId, [this](int errorCode, const QString& errorDesc, const QJsonObject& result) {
        if (errorCode != 0) {
            QString errorMsg = QString("解散群聊失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
            qDebug() << errorMsg;
            CustomMessageBox::warning(this, "解散失败", errorMsg);
            return;
        }
        
        qDebug() << "解散群聊成功，响应结果:" << result;
        
        // 发出群聊解散信号，通知父窗口刷新群列表
        emit this->groupDismissed(m_groupNumberId);
        
        // 显示成功消息
        CustomMessageBox::information(this, "解散成功", 
            QString("已成功解散群聊 \"%1\"！").arg(m_groupName));
        
        // 关闭对话框
        this->accept();
    });
    
    // 原来调用自己服务器接口的代码（已注释）
    // sendDismissGroupRequestToServer(m_groupNumberId, userId);
}

void QGroupInfo::sendDismissGroupRequestToServer(const QString& groupId, const QString& userId)
{
    // 构造发送到服务器的JSON数据
    QJsonObject requestData;
    requestData["group_id"] = groupId;
    requestData["user_id"] = userId;
    
    // 转换为JSON字符串
    QJsonDocument doc(requestData);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 发送POST请求到服务器（假设解散接口为 /groups/dismiss，如果不同请修改）
    QString url = "http://47.100.126.194:5000/groups/dismiss";
    
    // 使用QNetworkAccessManager发送POST请求
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest networkRequest;
    networkRequest.setUrl(QUrl(url));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = manager->post(networkRequest, jsonData);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qDebug() << "服务器响应:" << QString::fromUtf8(response);
            
            // 解析响应
            QJsonParseError parseError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &parseError);
            if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                if (obj["code"].toInt() == 200) {
                    qDebug() << "解散群聊请求已发送到服务器";
                    
                    // 服务器响应成功，发出群聊解散信号，通知父窗口刷新群列表
                    emit this->groupDismissed(groupId);
                    
                    // 显示成功消息
                    CustomMessageBox::information(this, "解散成功", 
                        QString("已成功解散群聊 \"%1\"！").arg(m_groupName));
                    
                    // 关闭对话框
                    this->accept();
                } else {
                    QString message = obj["message"].toString();
                    qDebug() << "服务器返回错误:" << message;
                    CustomMessageBox::warning(this, "解散失败", 
                        QString("服务器返回错误: %1").arg(message));
                }
            } else {
                qDebug() << "解析服务器响应失败";
                CustomMessageBox::warning(this, "解散失败", "解析服务器响应失败");
            }
        } else {
            qDebug() << "发送解散群聊请求到服务器失败:" << reply->errorString();
            CustomMessageBox::warning(this, "解散失败", 
                QString("网络错误: %1").arg(reply->errorString()));
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

void QGroupInfo::refreshMemberList(const QString& groupId)
{
    // 通知父窗口（ScheduleDialog）刷新成员列表
    emit membersRefreshed(groupId);
    qDebug() << "发出成员列表刷新信号，群组ID:" << groupId;

    // 普通群：直接通过腾讯IM SDK 刷新成员列表，避免依赖外部窗口转发
    if (m_isNormalGroup) {
        fetchGroupMemberListFromSDK(groupId);
    }
}

void QGroupInfo::fetchGroupMemberListFromREST(const QString& groupId)
{
    if (!m_restAPI) {
        qWarning() << "REST API未初始化，无法获取群成员列表";
        return;
    }
    
    // 设置管理员账号信息
    std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
    if (!adminUserId.empty()) {
        std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
        m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
    } else {
        qWarning() << "管理员账号未设置，无法获取群成员列表";
        return;
    }
    
    // 使用REST API获取群成员列表
    m_restAPI->getGroupMemberList(groupId, 100, 0, 
        [this](int errorCode, const QString& errorDesc, const QJsonObject& result) {
            if (errorCode != 0) {
                qDebug() << "获取群成员列表失败，错误码:" << errorCode << "，描述:" << errorDesc;
                return;
            }

            // DEBUG: 打印完整返回，方便定位“成员名字”字段（仅 Debug 构建输出，避免 Release 刷屏）
#ifdef QT_DEBUG
            qDebug().noquote() << "getGroupMemberList REST raw result:\n"
                               << QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Indented));
#endif
            // 解析成员列表
            QVector<GroupMemberInfo> memberList;
            QJsonArray memberArray = result["MemberList"].toArray();

            // DEBUG: 打印 MemberList 前几项的关键字段（避免日志过大，只打前3个；仅 Debug 构建输出）
#ifdef QT_DEBUG
            {
                const int debugN = qMin(3, memberArray.size());
                for (int i = 0; i < debugN; ++i) {
                    const QJsonObject o = memberArray.at(i).toObject();
                    qDebug() << "MemberList[" << i << "] keys:" << o.keys();
                    qDebug() << "MemberList[" << i << "] Member_Account:" << o.value("Member_Account").toString()
                             << "NameCard:" << o.value("NameCard").toString()
                             << "Nick:" << o.value("Nick").toString();
                }
            }
#endif
            
            for (const QJsonValue& value : memberArray) {
                if (!value.isObject()) continue;
                
                QJsonObject memberObj = value.toObject();
                GroupMemberInfo memberInfo;
                
                // 获取成员账号
                memberInfo.member_id = memberObj["Member_Account"].toString();
                
                // 获取成员角色
                QString role = memberObj["Role"].toString();
                if (role == "Owner") {
                    memberInfo.member_role = "群主";
                } else if (role == "Admin") {
                    memberInfo.member_role = "管理员";
                } else {
                    memberInfo.member_role = "成员";
                }
                
                // 获取成员名称（如果有）
                // 常见字段：NameCard(群名片) / Nick(昵称)。优先 NameCard，其次 Nick，最后退回 member_id
                const QString nameCard = memberObj.value("NameCard").toString();
                const QString nick = memberObj.value("Nick").toString();
                if (!nameCard.isEmpty()) {
                    memberInfo.member_name = nameCard;
                } else if (!nick.isEmpty()) {
                    memberInfo.member_name = nick;
                } else {
                    // 如果没有群名片，使用账号ID
                    memberInfo.member_name = memberInfo.member_id;
                }
                
                // 初始化 is_voice_enabled（REST API 可能不包含此字段，默认为 false）
                memberInfo.is_voice_enabled = true;
                
                memberList.append(memberInfo);
            }
            
            // 合并更新成员列表（保留原有成员名称等信息）
            // 从REST API获取的成员列表可能没有成员名称，所以需要合并而不是直接替换
            int existingCount = m_groupMemberInfo.size();
            int newCount = 0;
            
            for (const GroupMemberInfo& newMember : memberList) {
                // 检查 m_groupMemberInfo 中是否已存在该成员
                bool found = false;
                for (GroupMemberInfo& existingMember : m_groupMemberInfo) {
                    if (existingMember.member_id == newMember.member_id) {
                        // 成员已存在，更新角色信息（保留原有的成员名称和 is_voice_enabled）
                        existingMember.member_role = newMember.member_role;
                        // 注意：REST API 获取的成员信息中 is_voice_enabled 被初始化为 false
                        // 如果原有成员信息中有正确的 is_voice_enabled 值，应该保留原有值
                        // 这里不更新 is_voice_enabled，保留服务器返回的原始值
                        found = true;
                        break;
                    }
                }
                
                // 如果成员不存在，添加到列表中
                if (!found) {
                    m_groupMemberInfo.append(newMember);
                    newCount++;
                }
            }
            
            // 刷新UI（使用更新后的完整成员列表）
            InitGroupMember(m_groupNumberId, m_groupMemberInfo);
            
            // 根据当前用户的 is_voice_enabled 更新对讲开关状态
            updateIntercomState();
            
            qDebug() << "成功更新群成员列表，原有" << existingCount << "个成员，新增" 
                     << newCount << "个成员，当前共" << m_groupMemberInfo.size() << "个成员";
        });
}

void QGroupInfo::fetchGroupMemberListFromSDK(const QString& groupId)
{
    if (groupId.trimmed().isEmpty()) return;

    // 首次打开先渲染一次（至少显示添加/删除），避免空白
    if (m_isNormalGroup && m_memberGridLayout && m_memberGridLayout->count() == 0) {
        renderNormalGroupMemberGrid();
    }

    GroupMemberFetchSDKData* data = new GroupMemberFetchSDKData;
    data->dlg = this;
    data->groupId = groupId;
    data->members.clear();
    startFetchMembersFromSDK(data, 0);
}

void QGroupInfo::onSetLeaderRequested(const QString& memberId)
{
    // 当前用户是否为群主：统一使用外部传入的 iGroupOwner（m_iGroupOwner）
    if (!m_iGroupOwner) {
        CustomMessageBox::warning(this, "权限不足", "只有群主可以设置管理员！");
        return;
    }
    
    // 检查成员是否在群组中
    bool isMemberInGroup = false;
    bool isAlreadyAdmin = false;
    QString memberName;
    for (const auto& member : m_groupMemberInfo) {
        if (member.member_id == memberId) {
            isMemberInGroup = true;
            memberName = member.member_name;
            if (member.member_role == "管理员") {
                isAlreadyAdmin = true;
            }
            break;
        }
    }
    
    // 如果成员不在群组中，提示错误
    if (!isMemberInGroup) {
        CustomMessageBox::warning(this, "操作失败", 
            QString("无法设置管理员：用户 %1 还不是群成员。\n\n请先邀请该用户加入群组，然后再设置管理员。").arg(memberId));
        return;
    }
    
    if (isAlreadyAdmin) {
        CustomMessageBox::information(this, "提示", "该成员已经是管理员！");
        return;
    }
    
    // 检查REST API是否初始化
    if (!m_restAPI) {
        CustomMessageBox::critical(this, "错误", "REST API未初始化！");
        return;
    }
    
    // 设置管理员账号信息
    std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
    if (!adminUserId.empty()) {
        std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
        m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
    } else {
        CustomMessageBox::critical(this, "错误", "管理员账号未设置！");
        return;
    }
    
    // 先获取群组信息，检查群组类型
    QStringList groupIdList;
    groupIdList.append(m_groupNumberId);
    m_restAPI->getGroupInfo(groupIdList, [this, memberId, memberName](int errorCode, const QString& errorDesc, const QJsonObject& result) {
        if (errorCode != 0) {
            QString errorMsg = QString("获取群组信息失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
            qDebug() << errorMsg;
            CustomMessageBox::critical(this, "错误", errorMsg);
            return;
        }
        
        // 检查群组类型
        QJsonArray groupArray = result["GroupInfo"].toArray();
        if (groupArray.isEmpty()) {
            CustomMessageBox::critical(this, "错误", "无法获取群组信息！");
            return;
        }
        
        QJsonObject groupInfo = groupArray[0].toObject();
        QString groupType = groupInfo["Type"].toString();
        
        // Private群组不支持设置管理员
        if (groupType == "Private") {
            CustomMessageBox::warning(this, "不支持的操作", 
                "私有群（Private）不支持设置管理员功能。\n\n只有公开群（Public）、聊天室（ChatRoom）和直播群（AVChatRoom）支持设置管理员。");
            return;
        }
        
        // 群组类型支持设置管理员，继续执行设置操作
        m_restAPI->modifyGroupMemberInfo(m_groupNumberId, memberId, "Admin", -1, QString(),
            [this, memberId, memberName](int errorCode, const QString& errorDesc, const QJsonObject& result) {
                if (errorCode != 0) {
                    QString errorMsg;
                    // 特殊处理10004错误码（群组类型不支持设置管理员）
                    if (errorCode == 10004) {
                        errorMsg = QString("设置管理员失败\n错误码: %1\n错误描述: %2\n\n该群组类型不支持设置管理员功能。\n只有公开群（Public）、聊天室（ChatRoom）和直播群（AVChatRoom）支持设置管理员。").arg(errorCode).arg(errorDesc);
                    } else if (errorDesc.contains("must be member", Qt::CaseInsensitive) || 
                               errorDesc.contains("不是群成员", Qt::CaseInsensitive)) {
                        // 处理"modify account must be member"错误：用户不是群成员
                        errorMsg = QString("设置管理员失败\n错误码: %1\n错误描述: %2\n\n用户 %3 还不是群成员，无法设置管理员。\n请先邀请该用户加入群组，然后再设置管理员。")
                            .arg(errorCode).arg(errorDesc).arg(memberId);
                    } else {
                        errorMsg = QString("设置管理员失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
                    }
                    qDebug() << errorMsg;
                    CustomMessageBox::critical(this, "设置失败", errorMsg);
                    return;
                }
            
            qDebug() << "REST API设置管理员成功，成员ID:" << memberId;
            
            // 更新本地成员列表中的角色
            for (auto& member : m_groupMemberInfo) {
                if (member.member_id == memberId) {
                    member.member_role = "管理员";
                    break;
                }
            }
            
            // 刷新UI
            InitGroupMember(m_groupNumberId, m_groupMemberInfo);
            
            // REST API成功，现在调用自己的服务器接口
            sendSetAdminRoleRequestToServer(m_groupNumberId, memberId, "管理员");
            
            // 刷新成员列表（通知父窗口刷新）
            refreshMemberList(m_groupNumberId);
            
            CustomMessageBox::information(this, "成功", QString("已将 %1 设为管理员（班主任）！").arg(memberName));
        });
    });
}

void QGroupInfo::onCancelLeaderRequested(const QString& memberId)
{
    // 当前用户是否为群主：统一使用外部传入的 iGroupOwner（m_iGroupOwner）
    if (!m_iGroupOwner) {
        CustomMessageBox::warning(this, "权限不足", "只有群主可以取消管理员！");
        return;
    }
    
    // 检查成员是否是管理员
    bool isAdmin = false;
    QString memberName;
    for (const auto& member : m_groupMemberInfo) {
        if (member.member_id == memberId) {
            memberName = member.member_name;
            if (member.member_role == "管理员") {
                isAdmin = true;
            }
            break;
        }
    }
    
    if (!isAdmin) {
        CustomMessageBox::information(this, "提示", "该成员不是管理员！");
        return;
    }
    
    // 检查REST API是否初始化
    if (!m_restAPI) {
        CustomMessageBox::critical(this, "错误", "REST API未初始化！");
        return;
    }
    
    // 设置管理员账号信息
    std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
    if (!adminUserId.empty()) {
        std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
        m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
    } else {
        CustomMessageBox::critical(this, "错误", "管理员账号未设置！");
        return;
    }
    
    // 先获取群组信息，检查群组类型
    QStringList groupIdList;
    groupIdList.append(m_groupNumberId);
    m_restAPI->getGroupInfo(groupIdList, [this, memberId, memberName](int errorCode, const QString& errorDesc, const QJsonObject& result) {
        if (errorCode != 0) {
            QString errorMsg = QString("获取群组信息失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
            qDebug() << errorMsg;
            CustomMessageBox::critical(this, "错误", errorMsg);
            return;
        }
        
        // 检查群组类型
        QJsonArray groupArray = result["GroupInfo"].toArray();
        if (groupArray.isEmpty()) {
            CustomMessageBox::critical(this, "错误", "无法获取群组信息！");
            return;
        }
        
        QJsonObject groupInfo = groupArray[0].toObject();
        QString groupType = groupInfo["Type"].toString();
        
        // Private群组不支持取消管理员
        if (groupType == "Private") {
            CustomMessageBox::warning(this, "不支持的操作", 
                "私有群（Private）不支持管理员功能。\n\n只有公开群（Public）、聊天室（ChatRoom）和直播群（AVChatRoom）支持管理员功能。");
            return;
        }
        
        // 群组类型支持管理员功能，继续执行取消操作
        m_restAPI->modifyGroupMemberInfo(m_groupNumberId, memberId, "Member", -1, QString(),
            [this, memberId, memberName](int errorCode, const QString& errorDesc, const QJsonObject& result) {
                if (errorCode != 0) {
                    QString errorMsg;
                    // 特殊处理10004错误码（群组类型不支持设置管理员）
                    if (errorCode == 10004) {
                        errorMsg = QString("取消管理员失败\n错误码: %1\n错误描述: %2\n\n该群组类型不支持管理员功能。\n只有公开群（Public）、聊天室（ChatRoom）和直播群（AVChatRoom）支持管理员功能。").arg(errorCode).arg(errorDesc);
                    } else {
                        errorMsg = QString("取消管理员失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
                    }
                    qDebug() << errorMsg;
                    CustomMessageBox::critical(this, "取消失败", errorMsg);
                    return;
                }
            
            qDebug() << "REST API取消管理员成功，成员ID:" << memberId;
            
            // 更新本地成员列表中的角色
            for (auto& member : m_groupMemberInfo) {
                if (member.member_id == memberId) {
                    member.member_role = "成员";
                    break;
                }
            }
            
            // 刷新UI
            InitGroupMember(m_groupNumberId, m_groupMemberInfo);
            
            // REST API成功，现在调用自己的服务器接口
            sendSetAdminRoleRequestToServer(m_groupNumberId, memberId, "成员");
            
            // 刷新成员列表（通知父窗口刷新）
            refreshMemberList(m_groupNumberId);
            
            CustomMessageBox::information(this, "成功", QString("已取消 %1 的管理员身份（班主任）！").arg(memberName));
        });
    });
}

void QGroupInfo::sendSetAdminRoleRequestToServer(const QString& groupId, const QString& memberId, const QString& role)
{
    // 构造发送到服务器的JSON数据
    QJsonObject requestData;
    requestData["group_id"] = groupId;
    requestData["user_id"] = memberId;
    requestData["role"] = role; // "管理员" 或 "成员"
    
    // 转换为JSON字符串
    QJsonDocument doc(requestData);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 发送POST请求到服务器
    QString url = "http://47.100.126.194:5000/groups/set_admin_role";
    
    // 使用QNetworkAccessManager发送POST请求
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest networkRequest;
    networkRequest.setUrl(QUrl(url));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = manager->post(networkRequest, jsonData);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qDebug() << "设置管理员角色服务器响应:" << QString::fromUtf8(response);
            
            // 解析响应
            QJsonParseError parseError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &parseError);
            if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                if (obj["code"].toInt() == 200) {
                    qDebug() << "设置管理员角色请求已发送到服务器，群组ID:" << groupId << "，成员ID:" << memberId << "，角色:" << role;
                } else {
                    QString message = obj["message"].toString();
                    qDebug() << "服务器返回错误:" << message;
                    // 不显示错误消息框，因为腾讯IM已经成功，只是服务器同步失败
                }
            } else {
                qDebug() << "解析服务器响应失败";
            }
        } else {
            qDebug() << "发送设置管理员角色请求到服务器失败:" << reply->errorString();
            // 不显示错误消息框，因为腾讯IM已经成功，只是服务器同步失败
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

void QGroupInfo::transferOwnerAndQuit(const QString& newOwnerId, const QString& newOwnerName)
{
    // 检查REST API是否初始化
    if (!m_restAPI) {
        CustomMessageBox::critical(this, "错误", "REST API未初始化！");
        return;
    }
    
    // 获取当前用户信息
    UserInfo userInfo = CommonInfo::GetData();
    QString currentUserId = userInfo.teacher_unique_id;
    
    // 设置管理员账号信息
    std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
    if (!adminUserId.empty()) {
        std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
        m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
    } else {
        CustomMessageBox::critical(this, "错误", "管理员账号未设置！");
        return;
    }
    
    // 调用腾讯IM REST API转让群主
    m_restAPI->changeGroupOwner(m_groupNumberId, newOwnerId,
        [this, newOwnerId, newOwnerName, currentUserId](int errorCode, const QString& errorDesc, const QJsonObject& result) {
            if (errorCode != 0) {
                QString errorMsg = QString("转让群主失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
                qDebug() << errorMsg;
                CustomMessageBox::critical(this, "转让失败", errorMsg);
                return;
            }
            
            qDebug() << "REST API转让群主成功，新群主ID:" << newOwnerId;
            
            // 更新本地成员列表中的角色
            for (auto& member : m_groupMemberInfo) {
                if (member.member_id == newOwnerId) {
                    member.member_role = "群主";
                } else if (member.member_id == currentUserId) {
                    member.member_role = "成员"; // 原群主变为普通成员
                }
            }
            
            // 调用自己的服务器接口，修改新管理员为群主，并让原群主退出群组
            // 服务器接口会处理这两个操作，所以这里只需要调用服务器接口
            sendTransferOwnerRequestToServer(m_groupNumberId, currentUserId, newOwnerId);
            
            // 转让成功后，原群主退出群聊（腾讯IM层面）
            m_restAPI->quitGroup(m_groupNumberId, currentUserId,
                [this, currentUserId, newOwnerName](int errorCode, const QString& errorDesc, const QJsonObject& result) {
                    if (errorCode != 0) {
                        QString errorMsg = QString("退出群聊失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
                        qDebug() << errorMsg;
                        CustomMessageBox::critical(this, "退出失败", errorMsg);
                        return;
                    }
                    
                    qDebug() << "REST API退出群聊成功，用户ID:" << currentUserId;
                    
                    // 立即从本地成员列表中移除当前用户
                    QVector<GroupMemberInfo> updatedMemberList;
                    for (const auto& member : m_groupMemberInfo) {
                        if (member.member_id != currentUserId) {
                            updatedMemberList.append(member);
                        }
                    }
                    m_groupMemberInfo = updatedMemberList;
                    
                    // 立即更新UI（移除退出的成员）
                    InitGroupMember(m_groupNumberId, m_groupMemberInfo);
                    
                    // 刷新成员列表（通知父窗口刷新）
                    refreshMemberList(m_groupNumberId);
                    
                    // 发出成员退出群聊信号，通知父窗口刷新群列表
                    emit this->memberLeftGroup(m_groupNumberId, currentUserId);
                    
                    // 显示成功消息
                    CustomMessageBox::information(this, "退出成功", 
                        QString("已成功退出群聊 \"%1\"！\n群主身份已转让给 %2。").arg(m_groupName).arg(newOwnerName));
                    
                    // 关闭对话框
                    this->accept();
                });
        });
}

void QGroupInfo::sendTransferOwnerRequestToServer(const QString& groupId, const QString& oldOwnerId, const QString& newOwnerId)
{
    // 构造发送到服务器的JSON数据
    QJsonObject requestData;
    requestData["group_id"] = groupId;
    requestData["old_owner_id"] = oldOwnerId;
    requestData["new_owner_id"] = newOwnerId;
    
    // 转换为JSON字符串
    QJsonDocument doc(requestData);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 发送POST请求到服务器
    QString url = "http://47.100.126.194:5000/groups/transfer_owner";
    
    // 使用QNetworkAccessManager发送POST请求
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest networkRequest;
    networkRequest.setUrl(QUrl(url));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = manager->post(networkRequest, jsonData);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qDebug() << "转让群主服务器响应:" << QString::fromUtf8(response);
            
            // 解析响应
            QJsonParseError parseError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &parseError);
            if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                int code = obj["code"].toInt();
                QString message = obj["message"].toString();
                
                if (code == 200) {
                    // 解析data字段
                    if (obj.contains("data") && obj["data"].isObject()) {
                        QJsonObject dataObj = obj["data"].toObject();
                        QString groupIdFromServer = dataObj["group_id"].toString();
                        QString groupNameFromServer = dataObj["group_name"].toString();
                        QString oldOwnerIdFromServer = dataObj["old_owner_id"].toString();
                        QString newOwnerIdFromServer = dataObj["new_owner_id"].toString();
                        QString newOwnerNameFromServer = dataObj["new_owner_name"].toString();
                        
                        qDebug() << "转让群主请求已发送到服务器成功";
                        qDebug() << "群组ID:" << groupIdFromServer;
                        qDebug() << "群组名称:" << groupNameFromServer;
                        qDebug() << "原群主ID:" << oldOwnerIdFromServer;
                        qDebug() << "新群主ID:" << newOwnerIdFromServer;
                        qDebug() << "新群主名称:" << newOwnerNameFromServer;
                    } else {
                        qDebug() << "转让群主请求已发送到服务器，群组ID:" << groupId << "，原群主ID:" << oldOwnerId << "，新群主ID:" << newOwnerId;
                    }
                } else {
                    qDebug() << "服务器返回错误，code:" << code << "，message:" << message;
                    // 不显示错误消息框，因为腾讯IM已经成功，只是服务器同步失败
                }
            } else {
                qDebug() << "解析服务器响应失败";
            }
        } else {
            qDebug() << "发送转让群主请求到服务器失败:" << reply->errorString();
            // 不显示错误消息框，因为腾讯IM已经成功，只是服务器同步失败
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

QGroupInfo::~QGroupInfo()
{}

void QGroupInfo::enterEvent(QEvent* event)
{
    QDialog::enterEvent(event);
    if (m_closeButton)
        m_closeButton->show();
}

void QGroupInfo::leaveEvent(QEvent* event)
{
    QDialog::leaveEvent(event);
    if (m_closeButton)
        m_closeButton->hide();
}

void QGroupInfo::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    if (m_closeButton)
        m_closeButton->move(this->width() - 22, 0);
}

void QGroupInfo::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
    }
    QDialog::mousePressEvent(event);
}

void QGroupInfo::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
    }
    QDialog::mouseMoveEvent(event);
}

void QGroupInfo::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
    QDialog::mouseReleaseEvent(event);
}

// ==================== CustomMessageBox 实现 ====================

Q_DECLARE_OPERATORS_FOR_FLAGS(CustomMessageBox::StandardButtons)

CustomMessageBox::CustomMessageBox(QWidget* parent)
    : QDialog(parent)
    , m_titleBar(nullptr)
    , m_titleLabel(nullptr)
    , m_closeButton(nullptr)
    , m_textLabel(nullptr)
    , m_okButton(nullptr)
    , m_yesButton(nullptr)
    , m_noButton(nullptr)
    , m_cancelButton(nullptr)
    , m_icon(NoIcon)
    , m_buttons(StandardButtons(Ok))
    , m_result(NoButton)
    , m_dragging(false)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(400, 150);
    setupUI();
}

CustomMessageBox::~CustomMessageBox()
{
}

void CustomMessageBox::setTitle(const QString& title)
{
    m_title = title;
    if (m_titleLabel) {
        m_titleLabel->setText(title);
    }
}

void CustomMessageBox::setText(const QString& text)
{
    m_text = text;
    if (m_textLabel) {
        m_textLabel->setText(text);
        // 根据文本长度调整窗口大小
        QFontMetrics fm(m_textLabel->font());
        QRect textRect = fm.boundingRect(QRect(0, 0, 360, 0), Qt::TextWordWrap, text);
        int textHeight = textRect.height();
        int minHeight = 120 + qMax(0, textHeight - 30);
        setFixedHeight(minHeight);
    }
}

void CustomMessageBox::setIcon(Icon icon)
{
    m_icon = icon;
    // 可以根据图标类型设置不同的样式
}

void CustomMessageBox::setStandardButtons(StandardButtons buttons)
{
    m_buttons = buttons;
    
    if (m_okButton) m_okButton->setVisible(buttons & Ok);
    if (m_yesButton) m_yesButton->setVisible(buttons & Yes);
    if (m_noButton) m_noButton->setVisible(buttons & No);
    if (m_cancelButton) m_cancelButton->setVisible(buttons & Cancel);
}

void CustomMessageBox::setupUI()
{
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
    m_titleBar = new QWidget(container);
    m_titleBar->setFixedHeight(40);
    m_titleBar->setStyleSheet("background-color: #282A2B; border-top-left-radius: 8px; border-top-right-radius: 8px;");
    
    QHBoxLayout* titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(15, 0, 15, 0);
    titleLayout->setSpacing(10);
    
    // 关闭按钮（初始隐藏）
    m_closeButton = new QPushButton("×", m_titleBar);
    m_closeButton->setFixedSize(24, 24);
    m_closeButton->setStyleSheet(
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
    m_closeButton->setVisible(false);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
    
    // 标题文本
    m_titleLabel = new QLabel("", m_titleBar);
    m_titleLabel->setStyleSheet("color: #ffffff; font-size: 14px; font-weight: bold; background: transparent; border: none;");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    
    titleLayout->addWidget(m_titleLabel, 1);
    titleLayout->addWidget(m_closeButton);
    
    // 内容区域
    QWidget* contentWidget = new QWidget(container);
    contentWidget->setStyleSheet("background-color: #282A2B; border-bottom-left-radius: 8px; border-bottom-right-radius: 8px;");
    
    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setContentsMargins(20, 15, 20, 15);
    mainLayout->setSpacing(15);
    
    // 消息文本
    m_textLabel = new QLabel("", contentWidget);
    m_textLabel->setStyleSheet("color: white; font-size: 14px; background: transparent;");
    m_textLabel->setAlignment(Qt::AlignCenter);
    m_textLabel->setWordWrap(true);
    m_textLabel->setTextFormat(Qt::PlainText);
    
    mainLayout->addWidget(m_textLabel, 1);
    
    // 按钮布局
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    btnLayout->addStretch();
    
    // 确定按钮
    m_okButton = new QPushButton(QString::fromUtf8(u8"确定"), contentWidget);
    m_okButton->setFixedSize(80, 35);
    m_okButton->setStyleSheet(
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
    connect(m_okButton, &QPushButton::clicked, this, [this]() { m_result = Ok; accept(); });
    
    // 是按钮
    m_yesButton = new QPushButton(QString::fromUtf8(u8"是"), contentWidget);
    m_yesButton->setFixedSize(80, 35);
    m_yesButton->setStyleSheet(
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
    connect(m_yesButton, &QPushButton::clicked, this, [this]() { m_result = Yes; accept(); });
    
    // 否按钮
    m_noButton = new QPushButton(QString::fromUtf8(u8"否"), contentWidget);
    m_noButton->setFixedSize(80, 35);
    m_noButton->setStyleSheet(
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
    connect(m_noButton, &QPushButton::clicked, this, [this]() { m_result = No; reject(); });
    
    // 取消按钮
    m_cancelButton = new QPushButton(QString::fromUtf8(u8"取消"), contentWidget);
    m_cancelButton->setFixedSize(80, 35);
    m_cancelButton->setStyleSheet(
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
    connect(m_cancelButton, &QPushButton::clicked, this, [this]() { m_result = Cancel; reject(); });
    
    btnLayout->addWidget(m_okButton);
    btnLayout->addWidget(m_yesButton);
    btnLayout->addWidget(m_noButton);
    btnLayout->addWidget(m_cancelButton);
    btnLayout->addStretch();
    
    mainLayout->addLayout(btnLayout);
    
    // 主布局
    QVBoxLayout* containerMainLayout = new QVBoxLayout(container);
    containerMainLayout->setContentsMargins(0, 0, 0, 0);
    containerMainLayout->setSpacing(0);
    containerMainLayout->addWidget(m_titleBar);
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
    
    CloseButtonEventFilter* filter = new CloseButtonEventFilter(this, m_closeButton);
    installEventFilter(filter);
    
    // 初始隐藏所有按钮
    m_okButton->setVisible(false);
    m_yesButton->setVisible(false);
    m_noButton->setVisible(false);
    m_cancelButton->setVisible(false);
}

void CustomMessageBox::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        QPoint pos = mapFromGlobal(event->globalPos());
        if (m_titleBar && m_titleBar->geometry().contains(pos)) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        }
    }
    QDialog::mousePressEvent(event);
}

void CustomMessageBox::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
    }
    QDialog::mouseMoveEvent(event);
}

void CustomMessageBox::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
    QDialog::mouseReleaseEvent(event);
}

void CustomMessageBox::onButtonClicked()
{
    // 已在 connect 中处理
}

CustomMessageBox::StandardButton CustomMessageBox::information(QWidget* parent, const QString& title, const QString& text, StandardButtons buttons)
{
    CustomMessageBox msgBox(parent);
    msgBox.setTitle(title);
    msgBox.setText(text);
    msgBox.setIcon(Information);
    msgBox.setStandardButtons(buttons);
    
    // 居中显示
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - msgBox.width()) / 2;
        int y = (screenGeometry.height() - msgBox.height()) / 2;
        msgBox.move(x, y);
    }
    
    if (msgBox.exec() == QDialog::Accepted) {
        return msgBox.m_result;
    }
    return NoButton;
}

CustomMessageBox::StandardButton CustomMessageBox::warning(QWidget* parent, const QString& title, const QString& text, StandardButtons buttons)
{
    CustomMessageBox msgBox(parent);
    msgBox.setTitle(title);
    msgBox.setText(text);
    msgBox.setIcon(Warning);
    msgBox.setStandardButtons(buttons);
    
    // 居中显示
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - msgBox.width()) / 2;
        int y = (screenGeometry.height() - msgBox.height()) / 2;
        msgBox.move(x, y);
    }
    
    if (msgBox.exec() == QDialog::Accepted) {
        return msgBox.m_result;
    }
    return NoButton;
}

CustomMessageBox::StandardButton CustomMessageBox::critical(QWidget* parent, const QString& title, const QString& text, StandardButtons buttons)
{
    CustomMessageBox msgBox(parent);
    msgBox.setTitle(title);
    msgBox.setText(text);
    msgBox.setIcon(Critical);
    msgBox.setStandardButtons(buttons);
    
    // 居中显示
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - msgBox.width()) / 2;
        int y = (screenGeometry.height() - msgBox.height()) / 2;
        msgBox.move(x, y);
    }
    
    if (msgBox.exec() == QDialog::Accepted) {
        return msgBox.m_result;
    }
    return NoButton;
}

CustomMessageBox::StandardButton CustomMessageBox::question(QWidget* parent, const QString& title, const QString& text, StandardButtons buttons)
{
    CustomMessageBox msgBox(parent);
    msgBox.setTitle(title);
    msgBox.setText(text);
    msgBox.setIcon(Question);
    msgBox.setStandardButtons(buttons);
    
    // 居中显示
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - msgBox.width()) / 2;
        int y = (screenGeometry.height() - msgBox.height()) / 2;
        msgBox.move(x, y);
    }
    
    if (msgBox.exec() == QDialog::Accepted) {
        return msgBox.m_result;
    }
    return NoButton;
}

