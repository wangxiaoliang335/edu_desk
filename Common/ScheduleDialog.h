#pragma once
#include "RtmpMediaStreamer.h"
#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QVariant>
#include <QFrame>
#include <qfiledialog.h>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <qdebug.h>
#include <qlineedit.h>
#include <QWidget>
#include <QObject>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QEvent>
#include <QIcon>
#include <QAudioInput>
#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QIODevice>
#include <QTimer>
#include <qprogressbar.h>
#include <QPoint>
#include <QBrush>
#include <QColor>
#include <QUrl>
#include <QUrlQuery>
#include <QDate>
#include <QPainter>
#include <QPainterPath>
#include <QRegion>
#include <QGroupInfo.h>
#include <Windows.h>
#include <QRegularExpression>
#include "CustomListDialog.h"
#include "ClickableLabel.h"
#include "TAHttpHandler.h"
#include "ChatDialog.h"
#include "CommonInfo.h"
#include "QGroupInfo.h"
#include "TAHttpHandler.h"
#include "ArrangeSeatDialog.h"
#include "GroupNotifyDialog.h"
#include "QXlsx/header/xlsxdocument.h"
#include "QXlsx/header/xlsxworksheet.h"
#include "QXlsx/header/xlsxcell.h"
#include "QXlsx/header/xlsxglobal.h"
QT_BEGIN_NAMESPACE_XLSX
QT_END_NAMESPACE_XLSX
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
// 前向声明，避免循环依赖
class HeatmapSegmentDialog;
class HeatmapViewDialog;
// SegmentRange 定义在 HeatmapTypes.h 中
#include "HeatmapTypes.h"
#include <random>
#include <algorithm>
#include <exception>

//// 学生信息结构（用于排座）
//#ifndef STUDENT_INFO_DEFINED
//#define STUDENT_INFO_DEFINED
//struct StudentInfo {
//    QString id;      // 学号
//    QString name;    // 姓名
//    double score;    // 成绩（用于排序）
//    int originalIndex; // 原始索引
//    QMap<QString, double> attributes; // 多个属性值（如"背诵"、"语文"等）
//};
//#endif

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
	ScheduleDialog(QString classid, QWidget* parent = nullptr, TaQTWebSocket* pWs = NULL) : QDialog(parent)
	{
		setWindowTitle("课程表");
		// 设置无边框窗口
		setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
		// 座位区域：10列×121像素=1210像素宽，6行×50像素=300像素高
		// 窗口大小：宽度1238（1251×0.99），高度669（676×0.99）
        resize(1238, 669);
        m_cornerRadius = 16;
        updateMask();
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
		
		// 关闭按钮（右上角）
		closeButton = new QPushButton(this);
		closeButton->setIcon(QIcon(":/res/img/widget-close.png"));
		closeButton->setIconSize(QSize(22, 22));
		closeButton->setFixedSize(QSize(22, 22));
		closeButton->setStyleSheet("background: transparent;");
        closeButton->move(width() - closeButton->width() - 4, 4);
		closeButton->hide();
		connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);

		m_classid = classid;

		m_httpHandler = new TAHttpHandler(this);
		if (m_httpHandler)
		{
			connect(m_httpHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
				//成功消息就不发送了
				QJsonDocument jsonDoc = QJsonDocument::fromJson(responseString.toUtf8());
				if (jsonDoc.isObject()) {
					QJsonObject obj = jsonDoc.object();
					if (obj["friends"].isArray())
					{
						QJsonArray friendsArray = obj.value("friends").toArray();
						//fLayout->addLayout(makeRowBtn("教师", QString::number(friendsArray.size()), "blue", "white"));
						for (int i = 0; i < friendsArray.size(); i++)
						{
							QJsonObject friendObj = friendsArray.at(i).toObject();

							// teacher_info 对象
							QJsonObject teacherInfo = friendObj.value("teacher_info").toObject();
							int id = teacherInfo.value("id").toInt();
							QString name = teacherInfo.value("name").toString();
							QString subject = teacherInfo.value("subject").toString();
							QString idCard = teacherInfo.value("id_card").toString();

							// user_details 对象
							QJsonObject userDetails = friendObj.value("user_details").toObject();
							QString phone = userDetails.value("phone").toString();
							QString uname = userDetails.value("name").toString();
							QString sex = userDetails.value("sex").toString();

							/********************************************/
							QString avatar = userDetails.value("avatar").toString();
							QString strIdNumber = userDetails.value("id_number").toString();
							QString avatarBase64 = userDetails.value("avatar_base64").toString();

							// 没有文件名就用手机号或ID代替
							if (avatar.isEmpty())
								avatar = userDetails.value("id_number").toString() + "_" + ".png";

							// 从最后一个 "/" 之后开始截取
							QString fileName = avatar.section('/', -1);  // "320506197910016493_.png"
							QString saveDir = QCoreApplication::applicationDirPath() + "/avatars/" + strIdNumber; // 保存图片目录
							QDir().mkpath(saveDir);
							QString filePath = saveDir + "/" + fileName;

							if (avatarBase64.isEmpty()) {
								qWarning() << "No avatar data for" << filePath;
								//continue;
							}
							//m_userInfo.strHeadImagePath = filePath;

							// Base64 解码成图片二进制数据
							QByteArray imageData = QByteArray::fromBase64(avatarBase64.toUtf8());

							// 写入文件（覆盖旧的）
							QFile file(filePath);
							if (!file.open(QIODevice::WriteOnly)) {
								qWarning() << "Cannot open file for writing:" << filePath;
								//continue;
							}
							file.write(imageData);
							file.close();

							/*if (fLayout)
							{
								fLayout->addLayout(makePairBtn(filePath, name, "green", "white", "", false));
							}*/
							/********************************************/
						}

						QJsonObject oTmp = obj["data"].toObject();
						QString strTmp = oTmp["message"].toString();
						qDebug() << "status:" << oTmp["code"].toString();
						qDebug() << "msg:" << oTmp["message"].toString(); // 如果 msg 是中文，也能正常输出
						//errLabel->setText(strTmp);
						//user_id = oTmp["user_id"].toInt();
					}
					else if (obj["data"].isObject())
					{
						QJsonObject dataObj = obj["data"].toObject();
						
						// 处理成绩表数据（/student-scores/get 接口返回）
						// 数据格式：{"code": 200, "data": {"scores": [...]}}
						if (dataObj.contains("scores") && dataObj["scores"].isArray())
						{
							QJsonArray scoresArray = dataObj["scores"].toArray();
							m_students.clear();
							
							for (int i = 0; i < scoresArray.size(); i++)
							{
								QJsonObject scoreObj = scoresArray[i].toObject();
								
								StudentInfo student;
								student.id = scoreObj["student_id"].toString();
								student.name = scoreObj["student_name"].toString();
								student.originalIndex = i;
								
								// 读取各科成绩并填充到 attributes 中（处理 null 值）
								if (scoreObj.contains("chinese") && !scoreObj["chinese"].isNull()) {
									double chinese = scoreObj["chinese"].toDouble();
									student.attributes["语文"] = chinese;
								}
								if (scoreObj.contains("math") && !scoreObj["math"].isNull()) {
									double math = scoreObj["math"].toDouble();
									student.attributes["数学"] = math;
								}
								if (scoreObj.contains("english") && !scoreObj["english"].isNull()) {
									double english = scoreObj["english"].toDouble();
									student.attributes["英语"] = english;
								}
								
								// 读取总分（字段名是 total_score）
								if (scoreObj.contains("total_score") && !scoreObj["total_score"].isNull()) {
									double total = scoreObj["total_score"].toDouble();
									student.attributes["总分"] = total;
									student.score = total; // 使用总分作为排序依据
								} else {
									// 如果没有总分，计算总分
									double total = 0;
									if (student.attributes.contains("语文")) {
										total += student.attributes["语文"];
									}
									if (student.attributes.contains("数学")) {
										total += student.attributes["数学"];
									}
									if (student.attributes.contains("英语")) {
										total += student.attributes["英语"];
									}
									student.attributes["总分"] = total;
									student.score = total;
								}
								
							m_students.append(student);
						}
						
						qDebug() << "从服务器获取成绩表成功，学生数量:" << m_students.size();
						
						// 自动刷新座位表，使用"正序"排序（按成绩从高到低）
						if (!m_students.isEmpty() && seatTable) {
							arrangeSeats(m_students, "正序");
							qDebug() << "已自动刷新座位表，使用正序排序";
						}
						}
						// 处理 /groups/members 接口返回的成员列表
						else if (dataObj.contains("group_id") && dataObj["members"].isArray())
						{
							QString group_id = dataObj["group_id"].toString();
							
							m_groupMemberInfo.clear(); // 清空之前的成员列表
							
							QJsonArray memberArray = dataObj["members"].toArray();
							for (int i = 0; i < memberArray.size(); i++)
							{
								QJsonObject memberObj = memberArray[i].toObject();
								
								// 获取成员信息
								QString member_id = memberObj["user_id"].toString();
								QString member_name = memberObj["user_name"].toString();
								if (member_name.isEmpty()) {
									member_name = memberObj["member_name"].toString();
								}
								
								// 判断角色：self_role 400 表示群主（Owner），1 表示普通成员（Normal）
								int self_role = memberObj["self_role"].toInt();
								QString member_role = "成员"; // 默认是成员
								if (self_role == 400) { // kTIMMemberRole_Owner
									member_role = "群主";
								}
								
								// 如果接口返回的是 role 字符串，也可以直接使用
								if (memberObj.contains("role")) {
									QString roleStr = memberObj["role"].toString();
									if (roleStr == "owner" || roleStr == "Owner" || roleStr == "群主") {
										member_role = "群主";
									}
								}
								
								GroupMemberInfo groupMemInfo;
								groupMemInfo.member_id = member_id;
								groupMemInfo.member_name = member_name;
								groupMemInfo.member_role = member_role;
								m_groupMemberInfo.append(groupMemInfo);
							}

							// 设置到 m_groupInfo 对话框的好友列表
							if (m_groupInfo)
							{
								m_groupInfo->InitGroupMember(group_id, m_groupMemberInfo);
							}
						}
						else if (dataObj["joingroups"].isArray())
						{
							//gJoinLayout->addLayout(makeRowBtn("加入", "3", "orange", "black"));
							QJsonArray groupArray = dataObj["joingroups"].toArray();
							for (int i = 0; i < groupArray.size(); i++)
							{
								QJsonObject groupObj = groupArray.at(i).toObject();
								QString unique_group_id = groupObj.value("unique_group_id").toString();
								QString avatar_base64 = groupObj.value("avatar_base64").toString();
								QString headImage_path = groupObj.value("headImage_path").toString();
								//// 假设 avatarBase64 是 QString 类型，从 HTTP 返回的 JSON 中拿到
								//QByteArray imageData = QByteArray::fromBase64(avatar_base64.toUtf8());
								//QPixmap pixmap;
								//pixmap.loadFromData(imageData);

								// 从最后一个 "/" 之后开始截取
								QString fileName = headImage_path.section('/', -1);  // "320506197910016493_.png"
								QString saveDir = QCoreApplication::applicationDirPath() + "/group_images/" + unique_group_id; // 保存图片目录
								QDir().mkpath(saveDir);
								QString filePath = saveDir + "/" + fileName;

								if (avatar_base64.isEmpty()) {
									qWarning() << "No avatar data for" << filePath;
									//continue;
								}
								//m_userInfo.strHeadImagePath = filePath;

								// Base64 解码成图片二进制数据
								QByteArray imageData = QByteArray::fromBase64(avatar_base64.toUtf8());

								// 写入文件（覆盖旧的）
								QFile file(filePath);
								if (!file.open(QIODevice::WriteOnly)) {
									qWarning() << "Cannot open file for writing:" << filePath;
									//continue;
								}
								file.write(imageData);
								file.close();

								//lblAvatar->setPixmap(pixmap.scaled(lblAvatar->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
								//lblAvatar->setScaledContents(true);

								//gJoinLayout->addLayout(makePairBtn(filePath, groupObj.value("nickname").toString(), "white", "red", unique_group_id, false));
							}
						}
					}
				}
				else
				{
					//errLabel->setText("网络错误");
				}
			});

			connect(m_httpHandler, &TAHttpHandler::failed, this, [=](const QString& errResponseString) {
				{
					QJsonDocument jsonDoc = QJsonDocument::fromJson(errResponseString.toUtf8());
					if (jsonDoc.isObject()) {
						QJsonObject obj = jsonDoc.object();
						if (obj["data"].isObject())
						{
							QJsonObject oTmp = obj["data"].toObject();
							QString strTmp = oTmp["message"].toString();
							qDebug() << "status:" << oTmp["code"].toString();
							qDebug() << "msg:" << oTmp["message"].toString(); // 如果 msg 是中文，也能正常输出
							//errLabel->setText(strTmp);
						}
					}
					/*else
					{
						errLabel->setText("网络错误");
					}*/
				}
			});
		}

		m_pWs = pWs;
		m_chatDlg = new ChatDialog(this, m_pWs);
		customListDlg = new CustomListDialog(m_classid, this);
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 40, 20, 20);
        mainLayout->setSpacing(12);
		m_groupInfo = new QGroupInfo(this);
		
		// 连接成员退出群聊信号，当有成员退出时刷新成员列表
		connect(m_groupInfo, &QGroupInfo::memberLeftGroup, this, [this](const QString& groupId, const QString& leftUserId) {
			// 如果退出的是当前群组
			if (groupId == m_unique_group_id) {
				// 获取当前用户信息，判断是否是当前用户退出
				UserInfo userInfo = CommonInfo::GetData();
				QString currentUserId = userInfo.teacher_unique_id;
				
				// 先从本地成员列表中移除退出的用户（同步更新ScheduleDialog的成员列表）
				QVector<GroupMemberInfo> updatedMemberList;
				for (const auto& member : m_groupMemberInfo) {
					if (member.member_id != leftUserId) {
						updatedMemberList.append(member);
					}
				}
				m_groupMemberInfo = updatedMemberList; // 更新ScheduleDialog的成员列表
				
				// 判断是否是当前用户退出
				bool isCurrentUserLeft = (leftUserId == currentUserId);
				
				if (isCurrentUserLeft) {
					// 当前用户退出，关闭窗口并刷新父窗口
					qDebug() << "当前用户退出群聊，关闭群聊窗口，群组ID:" << groupId;
					
					// 发出信号通知父窗口刷新群列表
					emit this->groupLeft(groupId);
					
					// 关闭当前窗口
					this->close();
				} else {
					// 其他成员退出，只刷新成员列表UI（本地列表已更新，只需更新UI）
					qDebug() << "检测到成员退出群聊，更新成员列表UI，群组ID:" << groupId << "，退出用户ID:" << leftUserId;
					
					// 更新QGroupInfo的UI（使用更新后的成员列表）
					if (m_groupInfo) {
						m_groupInfo->InitGroupMember(groupId, m_groupMemberInfo);
					}
					
					// 可选：重新从服务器获取成员列表以确保数据同步
					if (m_httpHandler && !m_unique_group_id.isEmpty()) {
						QUrl url("http://47.100.126.194:5000/groups/members");
						QUrlQuery query;
						query.addQueryItem("group_id", m_unique_group_id);
						url.setQuery(query);
						m_httpHandler->get(url.toString());
					}
				}
			}
		});
		
		// 连接群聊解散信号，当群聊被解散时关闭窗口并刷新父窗口
		connect(m_groupInfo, &QGroupInfo::membersRefreshed, this, [this](const QString& groupId) {
			// 如果刷新的是当前群组
			if (groupId == m_unique_group_id) {
				qDebug() << "收到成员列表刷新信号，刷新成员列表，群组ID:" << groupId;
				refreshMemberList(groupId);
			}
		});
		
		connect(m_groupInfo, &QGroupInfo::groupDismissed, this, [this](const QString& groupId) {
			// 如果解散的是当前群组
			if (groupId == m_unique_group_id) {
				qDebug() << "群聊被解散，关闭群聊窗口，群组ID:" << groupId;
				
				// 发出信号通知父窗口刷新群列表
				emit this->groupLeft(groupId);
				
				// 关闭当前窗口
				this->close();
			}
		});
		
		// 连接新成员入群信号，自动刷新成员列表
		// GroupNotifyDialog是单例，通过静态方法获取实例并连接信号
		// 使用QTimer延迟连接，确保GroupNotifyDialog已经创建
		QTimer::singleShot(500, this, [this]() {
			// 通过GroupNotifyDialog的静态方法获取实例
			GroupNotifyDialog* notifyDlg = GroupNotifyDialog::instance();
			if (notifyDlg) {
				connect(notifyDlg, &GroupNotifyDialog::newMemberJoined, this, [this](const QString& groupId, const QStringList& memberIds) {
					// 如果新成员加入的是当前群组，自动刷新成员列表
					if (groupId == m_unique_group_id) {
						qDebug() << "检测到新成员入群，自动刷新成员列表，群组ID:" << groupId << "，新成员:" << memberIds.join(", ");
						refreshMemberList(groupId);
					}
				});
				qDebug() << "已连接GroupNotifyDialog的新成员入群信号";
			} else {
				qWarning() << "GroupNotifyDialog实例不存在，无法连接新成员入群信号";
			}
		});

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

	// 班级群功能按钮（普通群不显示）
	QPushButton* btnSeat = new QPushButton("座次表");
	QPushButton* btnCam = new QPushButton("摄像头");
	btnTalk = new QPushButton("按住开始对讲");
	QPushButton* btnMsg = new QPushButton("通知");
	QPushButton* btnTask = new QPushButton("作业");
	QString greenStyle = "background-color: green; color: white; padding: 4px 8px;";
	btnSeat->setStyleSheet(greenStyle);
	btnCam->setStyleSheet(greenStyle);
	btnTalk->setStyleSheet(greenStyle);
	btnMsg->setStyleSheet(greenStyle);
	btnTask->setStyleSheet(greenStyle);
	
	// 保存按钮指针，用于根据群组类型显示/隐藏
	m_btnSeat = btnSeat;
	m_btnCam = btnCam;
	m_btnMsg = btnMsg;
	m_btnTask = btnTask;

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
			if (m_groupInfo && m_groupInfo->isHidden())
			{
				m_groupInfo->InitGroupMember();
				m_groupInfo->show();
			}
			else if (m_groupInfo && !m_groupInfo->isHidden())
			{
				m_groupInfo->hide();
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
		
		// 连接作业按钮（教师端编辑作业）
		connectHomeworkButton(btnTask);

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


		// ===== 新增中排按钮 =====
		QHBoxLayout* middleBtnLayout = new QHBoxLayout;
		//QString greenStyle = "background-color: green; color: white; padding: 4px 8px;";

		QPushButton* btnRandom = new QPushButton("随机点名");
		QPushButton* btnAnalyse = new QPushButton("分断");
		QPushButton* btnHeatmap = new QPushButton("热力图");
		QPushButton* btnArrange = new QPushButton("排座");
		QPushButton* btnImportSeat = new QPushButton("导入学生信息");
		QPushButton* btnMoreBottom = new QPushButton("...");
		btnMoreBottom->setFixedSize(48, 24);
		btnMoreBottom->setText("...");
		btnMoreBottom->setStyleSheet(
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

		connect(btnMoreBottom, &QPushButton::clicked, this, [=]() {
			if (customListDlg && customListDlg->isHidden())
			{
				customListDlg->show();
			}
			else if (customListDlg && !customListDlg->isHidden())
			{
				customListDlg->hide();
			}
		});

		btnRandom->setStyleSheet(greenStyle);
		btnAnalyse->setStyleSheet(greenStyle);
		btnHeatmap->setStyleSheet(greenStyle);
		btnArrange->setStyleSheet(greenStyle);
		btnImportSeat->setStyleSheet(greenStyle);

		// 作业展示按钮（班级端快捷按钮）
		QPushButton* btnHomeworkView = new QPushButton("作业");
		btnHomeworkView->setStyleSheet(greenStyle);
		connect(btnHomeworkView, &QPushButton::clicked, this, [this]() {
			showHomeworkViewDialog();
		});
		
		middleBtnLayout->addStretch();
		middleBtnLayout->addWidget(btnRandom);
		middleBtnLayout->addWidget(btnAnalyse);
		middleBtnLayout->addWidget(btnHeatmap);
		middleBtnLayout->addWidget(btnArrange);
		middleBtnLayout->addWidget(btnImportSeat);
		middleBtnLayout->addWidget(btnHomeworkView); // 添加快捷按钮
		middleBtnLayout->addStretch();
		middleBtnLayout->addWidget(btnMoreBottom);

		mainLayout->addLayout(middleBtnLayout);

		// 创建排座对话框
		arrangeSeatDlg = new ArrangeSeatDialog(this);

		// 连接排座按钮点击事件
		connect(btnArrange, &QPushButton::clicked, this, [=]() {
			if (arrangeSeatDlg && arrangeSeatDlg->isHidden()) {
				arrangeSeatDlg->show();
			} else if (arrangeSeatDlg && !arrangeSeatDlg->isHidden()) {
				arrangeSeatDlg->hide();
			} else {
				arrangeSeatDlg = new ArrangeSeatDialog(this);
				arrangeSeatDlg->show();
			}
		});
		
		// 连接随机点名按钮点击事件
		connectRandomCallButton(btnRandom, seatTable);
		
		// 连接导入学生信息按钮点击事件
		connect(btnImportSeat, &QPushButton::clicked, this, [=]() {
			importSeatTable();
		});
		
		// 连接热力图按钮点击事件
		connect(btnHeatmap, &QPushButton::clicked, this, [=]() {
			// 检查是否有学生数据（必须上传期中成绩）
			if (m_students.isEmpty()) {
				QMessageBox::information(this, "提示", "请先上传期中成绩表！");
				return;
			}
			
			// 显示热力图选择对话框
			QDialog* typeDialog = new QDialog(this);
			typeDialog->setWindowTitle("选择热力图类型");
			typeDialog->setModal(true);
			typeDialog->resize(300, 150);
			
			QVBoxLayout* typeLayout = new QVBoxLayout(typeDialog);
			QLabel* lblTitle = new QLabel("请选择热力图类型：");
			typeLayout->addWidget(lblTitle);
			
			QPushButton* btnSegment = new QPushButton("分段图1（每一段一种颜色）");
			QPushButton* btnGradient = new QPushButton("热力图2（颜色渐变）");
			QPushButton* btnCancel = new QPushButton("取消");
			
			typeLayout->addWidget(btnSegment);
			typeLayout->addWidget(btnGradient);
			typeLayout->addWidget(btnCancel);
			
			connect(btnSegment, &QPushButton::clicked, typeDialog, [=]() {
				typeDialog->accept();
				this->showSegmentDialog();
			});
			
			connect(btnGradient, &QPushButton::clicked, typeDialog, [=]() {
				typeDialog->accept();
				this->showGradientHeatmap();
			});
			
			connect(btnCancel, &QPushButton::clicked, typeDialog, &QDialog::reject);
			
			typeDialog->exec();
			typeDialog->deleteLater();
		});

		// ===== 讲台区域 =====
		QHBoxLayout* podiumLayout = new QHBoxLayout;
		QPushButton* btnPodium = new QPushButton("讲台");
		btnPodium->setStyleSheet(greenStyle);
		btnPodium->setFixedHeight(30);
		btnPodium->setFixedWidth(80);
		podiumLayout->addStretch();
		podiumLayout->addWidget(btnPodium);
		podiumLayout->addStretch();
		mainLayout->addLayout(podiumLayout);


		// 座位表格区域 - 重新设计座位布局
		// 第1行：4个座位（过道两侧各2个）
		// 第2-8行：每行8个座位（左列3个 + 中列3个 + 右列2个，中间有两条过道）
		// 总共60个座位
		seatTable = new QTableWidget(8, 10); // 8行，10列（包含过道列）
		seatTable->horizontalHeader()->setVisible(false);
		seatTable->verticalHeader()->setVisible(false);
		seatTable->setStyleSheet(
			"QTableWidget { "
			"background-color: #5C5C5C; "
			"gridline-color: #5C5C5C; "
			"border: 1px solid #5C5C5C; "
			"}"
			"QTableWidget::item { "
			"background-color: transparent; "
			"padding: 5px; "
			"}"
		);
		
		// 设置列宽和行高（宽度扩大15%，高度扩大7%）
		for (int col = 0; col < 10; ++col) {
			seatTable->setColumnWidth(col, 115); // 列宽：100 × 1.15 = 115
		}
		for (int row = 0; row < 8; ++row) {
			seatTable->setRowHeight(row, 59); // 行高：55 × 1.07 = 59
		}
		
		// 初始化所有单元格，为每个单元格创建按钮
		for (int row = 0; row < 8; ++row) {
			for (int col = 0; col < 10; ++col) {
				QPushButton* btn = new QPushButton("");
				btn->setStyleSheet(
					"QPushButton { "
					"background-color: #dc3545; "
					"color: white; "
					"border: 1px solid #ccc; "
					"border-radius: 4px; "
					"padding: 5px; "
					"font-size: 12px; "
					"}"
					"QPushButton:hover { "
					"background-color: #c82333; "
					"}"
					"QPushButton:pressed { "
					"background-color: #bd2130; "
					"}"
				);
				btn->setProperty("row", row);
				btn->setProperty("col", col);
				
				// 连接按钮点击事件
				connect(btn, &QPushButton::clicked, this, [=]() {
					qDebug() << "座位按钮被点击: 行" << row << "列" << col;
				});
				
				seatTable->setCellWidget(row, col, btn);
			}
		}
		
		// 第1行布局：4个座位（过道两侧各2个）
		// 左侧2个座位：列0-1，右侧2个座位：列8-9，中间列2-7合并为过道
		seatTable->setSpan(0, 2, 1, 6); // 合并第1行的列2-7作为中央过道
		QPushButton* aisle1Btn = qobject_cast<QPushButton*>(seatTable->cellWidget(0, 2));
		if (aisle1Btn) {
			aisle1Btn->setEnabled(false);
			aisle1Btn->setStyleSheet(
				"QPushButton { "
				"background-color: #f0f0f0; "
				"border: 1px solid #ccc; "
				"}"
			);
		}
		
		// 第2-8行布局：每行8个座位
		// 左列3个：列0-2，过道列3，中列3个：列4-6，过道列7，右列2个：列8-9
		for (int row = 1; row < 8; ++row) {
			// 左列座位：列0-2
			for (int col = 0; col < 3; ++col) {
				QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
				if (btn) {
					btn->setText(""); // 可以显示座位号或学生姓名
					btn->setProperty("isSeat", true);
				}
			}
			
			// 左过道：列3
			QPushButton* aisleLeftBtn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, 3));
			if (aisleLeftBtn) {
				aisleLeftBtn->setEnabled(false);
				aisleLeftBtn->setStyleSheet(
					"QPushButton { "
					"background-color: #f0f0f0; "
					"border: 1px solid #ccc; "
					"}"
				);
			}
			
			// 中列座位：列4-6
			for (int col = 4; col < 7; ++col) {
				QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
				if (btn) {
					btn->setText("");
					btn->setProperty("isSeat", true);
				}
			}
			
			// 右过道：列7
			QPushButton* aisleRightBtn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, 7));
			if (aisleRightBtn) {
				aisleRightBtn->setEnabled(false);
				aisleRightBtn->setStyleSheet(
					"QPushButton { "
					"background-color: #f0f0f0; "
					"border: 1px solid #ccc; "
					"}"
				);
			}
			
			// 右列座位：列8-9
			for (int col = 8; col < 10; ++col) {
				QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
				if (btn) {
					btn->setText("");
					btn->setProperty("isSeat", true);
				}
			}
		}
		
		// 第1行的座位
		// 左侧2个座位：列0-1
		for (int col = 0; col < 2; ++col) {
			QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(0, col));
			if (btn) {
				btn->setText("");
				btn->setProperty("isSeat", true);
			}
		}
		// 右侧2个座位：列8-9
		for (int col = 8; col < 10; ++col) {
			QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(0, col));
			if (btn) {
				btn->setText("");
				btn->setProperty("isSeat", true);
			}
		}
		
		// 设置表格选择模式
		seatTable->setSelectionBehavior(QAbstractItemView::SelectItems);
		seatTable->setSelectionMode(QAbstractItemView::SingleSelection);
		seatTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
		
		// 设置固定高度，确保能容纳所有行而不出现滚动条
		// 8行 × 59像素 = 472像素，加上边框等额外空间，设置为503像素（扩大7%）
		seatTable->setFixedHeight(503);
		seatTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用垂直滚动条
		
		// 设置固定宽度，确保能容纳所有列而不出现水平滚动条
		// 10列 × 115像素 = 1150像素，加上边框等额外空间，设置为1173像素（扩大15%）
		seatTable->setFixedWidth(1173);
		seatTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用水平滚动条
		
		// 创建水平布局使seatTable居中
		QHBoxLayout* seatTableLayout = new QHBoxLayout;
		seatTableLayout->addStretch();
		seatTableLayout->addWidget(seatTable);
		seatTableLayout->addStretch();
		mainLayout->addLayout(seatTableLayout);

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
				m_chatDlg->show();
			}
		});

		mainLayout->addWidget(inputWidget);

		// 黄色圆圈数字
		QLabel* lblNum = new QLabel("3");
		lblNum->setAlignment(Qt::AlignCenter);
		lblNum->setFixedSize(30, 30);
		lblNum->setStyleSheet("background-color: yellow; color: red; font-weight: bold; font-size: 16px; border-radius: 15px;");
		mainLayout->addWidget(lblNum, 0, Qt::AlignRight);

		//// ===== 音量进度条 =====
		//m_volumeBar = new QProgressBar();
		//m_volumeBar->setRange(0, 100);
		//m_volumeBar->setValue(0);
		//m_volumeBar->setTextVisible(false);
		//m_volumeBar->setStyleSheet(
		//	"QProgressBar { height:8px; border:1px solid gray; background:lightgray; }"
		//	"QProgressBar::chunk { background-color: red; }");
		//mainLayout->addWidget(m_volumeBar);


		//// 底部右下角黄色圆圈数字
		//QLabel* lblNum = new QLabel("3");
		//lblNum->setAlignment(Qt::AlignCenter);
		//lblNum->setFixedSize(30, 30);
		//lblNum->setStyleSheet("background-color: yellow; color: red; font-weight: bold; font-size: 16px; border-radius: 15px;");
		//mainLayout->addWidget(lblNum, 0, Qt::AlignRight);

		// 信号与槽连接（C++11 lambda）
		//connect(btnTalk, &QPushButton::clicked, this, &ScheduleDialog::onBtnTalkClicked);
		
		// 初始化 RTMP 推流器（取代 WebRTC）
		m_rtmpStreamer = new RtmpMediaStreamer(this);
		m_rtmpStreamer->setSrsServer(QStringLiteral("47.100.126.194"), 1935);
		connect(m_rtmpStreamer, &RtmpMediaStreamer::logMessage, this, [](const QString& log) {
			qDebug() << "[RTMP]" << log;
		});
		connect(m_rtmpStreamer, &RtmpMediaStreamer::errorOccurred, this, [](const QString& err) {
			qWarning() << "[RTMP][Error]" << err;
		});
		
		// 按下按钮 -> 开始 RTMP 推流
		connect(btnTalk, &QPushButton::pressed, this, [=]() {
			pressStartMs = QDateTime::currentMSecsSinceEpoch();
			btnTalk->setStyleSheet("background-color: red; color: white; padding: 4px 8px; font-size:14px;");
			btnTalk->setText("录音中...松开结束");
			qDebug() << "开始对讲（按钮按下）- 使用 RTMP 推流";
			
			// 注释掉原来的音频采集和发送代码（可能后面还会用到）
			/*
			start();
			// 发送开始包（flag=0）
			QByteArray empty;
			encodeAndSend(empty, 0);
			*/
			
			#if 0
			// 使用 WebRTC 连接到 SRS 服务器（已暂时停用）
			// ...
			#else
			// 使用 RTMP 协议推流到 SRS，仅采集本地音频
			if (m_unique_group_id.isEmpty() || m_userId.isEmpty()) {
				qWarning() << "RTMP 推流失败：群组ID或用户ID为空";
				btnTalk->setStyleSheet("background-color: green; color: white; padding: 4px 8px; font-size:14px;");
				btnTalk->setText("按住开始对讲");
				return;
			}

			if (!m_rtmpStreamer) {
				qWarning() << "RTMP 推流失败：推流器未初始化";
				btnTalk->setStyleSheet("background-color: green; color: white; padding: 4px 8px; font-size:14px;");
				btnTalk->setText("按住开始对讲");
				return;
			}

			QAudioDeviceInfo audioInfo = QAudioDeviceInfo::defaultInputDevice();
			if (audioInfo.isNull()) {
				qWarning() << "RTMP 推流失败：未检测到可用麦克风";
				btnTalk->setStyleSheet("background-color: green; color: white; padding: 4px 8px; font-size:14px;");
				btnTalk->setText("按住开始对讲");
				return;
			}

			auto sanitizeId = [](const QString& src) -> QString {
				QString safe = src;
				static const QRegularExpression invalidPattern(QStringLiteral("[^A-Za-z0-9_\\-]"));
				return safe.replace(invalidPattern, QStringLiteral("_"));
			};

			QString streamName = QStringLiteral("stream_%1_%2")
				.arg(sanitizeId(m_unique_group_id), sanitizeId(m_userId));
			m_rtmpStreamer->setStreamKey(streamName);
			
			QAudioFormat preferredFormat;
			preferredFormat.setSampleRate(44100);
			preferredFormat.setChannelCount(1);
			preferredFormat.setSampleSize(16);
			preferredFormat.setCodec("audio/pcm");
			preferredFormat.setByteOrder(QAudioFormat::LittleEndian);
			preferredFormat.setSampleType(QAudioFormat::SignedInt);

			if (!startAudioCapture(preferredFormat)) {
				qWarning() << "音频采集启动失败";
				btnTalk->setStyleSheet("background-color: green; color: white; padding: 4px 8px; font-size:14px;");
				btnTalk->setText("按住开始对讲");
				return;
			}

			m_rtmpStreamer->setAudioFormat(m_audioFormat.sampleRate(), m_audioFormat.channelCount());

			if (!m_rtmpStreamer->start()) {
				stopAudioCapture();
				qWarning() << "RTMP 推流启动失败";
				btnTalk->setStyleSheet("background-color: green; color: white; padding: 4px 8px; font-size:14px;");
				btnTalk->setText("按住开始对讲");
				return;
			}
			qDebug() << "RTMP 推流已启动，流名称:" << streamName;
			#endif
		});

		// 松开按钮 -> 停止 RTMP 推流
		connect(btnTalk, &QPushButton::released, this, [=]() {
			qint64 releaseMs = QDateTime::currentMSecsSinceEpoch();
			qint64 duration = releaseMs - pressStartMs;

			// 注释掉原来的音频采集和发送代码（可能后面还会用到）
			/*
			// 发送结束包（flag=2）
			QByteArray empty;
			encodeAndSend(empty, 2);
			stopAudioCapture();  // 停止采集
			*/

			#if 0
			// 断开 WebRTC 连接（已停用）
			if (m_webrtcAudioSender) {
				m_webrtcAudioSender->disconnectFromServer();
				qDebug() << "WebRTC 连接已断开";
			}
			#else
			stopAudioCapture();
			if (m_rtmpStreamer && m_rtmpStreamer->isRunning()) {
				m_rtmpStreamer->stop();
				qDebug() << "RTMP 推流已停止";
			}
			#endif

			if (duration < 500) {
				qDebug() << "录音时间过短(" << duration << "ms)，丢弃";
			}
			else {
				qDebug() << "录音完成，时长:" << duration << "ms，音视频已通过 RTMP 推送";
			}

			btnTalk->setStyleSheet("background-color: green; color: white; padding: 4px 8px; font-size:14px;");
			btnTalk->setText("按住开始对讲");
		});
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

	void InitWebSocket()
	{
		if (m_chatDlg)
		{
			m_chatDlg->InitWebSocket();
		}
	}

	void setNoticeMsg(QList<Notification> listNoticeMsg)
	{
		if (m_chatDlg)
		{
			m_chatDlg->setNoticeMsg(listNoticeMsg);
		}
	}

signals:
	void groupLeft(const QString& groupId); // 群聊退出信号，通知父窗口刷新群列表

public:
	// 排座功能：根据学生数据自动排座
	// method: "随机排座", "正序", "倒序", "2人组排座", "4人组排座", "6人组排座"
	void arrangeSeats(const QList<StudentInfo>& students, const QString& method = "随机排座");
	void importSeatTable(); // 导入座位表格
	void importSeatFromExcel(const QString& filePath); // 从Excel导入座位表
	void importSeatFromCsv(const QString& filePath); // 从CSV导入座位表
	void fillSeatTableFromData(const QList<QStringList>& dataRows); // 将数据填充到座位表
	void uploadSeatTableToServer(); // 上传座位表到服务器
	
	// 热力图相关方法
	void showSegmentDialog(); // 显示分段区间设置对话框
	void showGradientHeatmap(); // 显示渐变热力图
	void setSegments(const QList<struct SegmentRange>& segments); // 设置分段区间
	
	void InitData(QString groupName, QString unique_group_id, QString classid, bool iGroupOwner, bool isClassGroup = true)
	{
		m_groupName = groupName;
		m_unique_group_id = unique_group_id;
		m_classid = classid;
		m_iGroupOwner = iGroupOwner;
		m_isClassGroup = isClassGroup; // 保存群组类型
		
		// 根据群组类型显示/隐藏班级群功能按钮
		if (m_btnSeat) m_btnSeat->setVisible(isClassGroup);
		if (m_btnCam) m_btnCam->setVisible(isClassGroup);
		if (m_btnMsg) m_btnMsg->setVisible(isClassGroup);
		if (m_btnTask) m_btnTask->setVisible(isClassGroup);
		// btnTalk 对讲功能也只在班级群显示
		if (btnTalk) btnTalk->setVisible(isClassGroup);
		
		if (m_lblClass)
		{
			m_lblClass->setText(groupName);
		}
		m_chatDlg->InitData(m_unique_group_id, iGroupOwner);
		UserInfo userInfo = CommonInfo::GetData();
		m_userId = userInfo.teacher_unique_id;
		m_userName = userInfo.strName;

		if (m_groupInfo)
		{
			m_groupInfo->initData(groupName, unique_group_id, classid);
			
			// 优先使用REST API获取群成员列表（从腾讯云IM直接获取，数据更准确）
			// 如果REST API失败，可以回退到使用自己的服务器接口
			m_groupInfo->fetchGroupMemberListFromREST(unique_group_id);
		}

		// 备用方案：调用自己的服务器接口获取群组成员列表
		// 注意：如果使用REST API成功，这个接口可以作为数据同步的备用方案
		if (m_httpHandler && !unique_group_id.isEmpty())
		{
			// 使用QUrl和QUrlQuery来正确编码URL参数（特别是#等特殊字符）
			QUrl url("http://47.100.126.194:5000/groups/members");
			QUrlQuery query;
			query.addQueryItem("group_id", unique_group_id);
			url.setQuery(query);
			m_httpHandler->get(url.toString());
		}
		
		// 从服务器获取成绩表数据（如果是班级群）
		if (m_httpHandler && isClassGroup && !classid.isEmpty())
		{
			// 根据当前日期计算学期
			QDate currentDate = QDate::currentDate();
			int year = currentDate.year();
			int month = currentDate.month();
			
			QString term;
			// 9月-1月是上学期（第一学期），2月-8月是下学期（第二学期）
			if (month >= 9 || month <= 1) {
				if (month >= 9) {
					term = QString("%1-%2-1").arg(year).arg(year + 1);
				} else {
					term = QString("%1-%2-1").arg(year - 1).arg(year);
				}
			} else {
				term = QString("%1-%2-2").arg(year - 1).arg(year);
			}
			
			// 获取成绩表数据
			QUrl url("http://47.100.126.194:5000/student-scores/get");
			QUrlQuery query;
			query.addQueryItem("class_id", classid);
			query.addQueryItem("exam_name", "期中考试");
			query.addQueryItem("term", term);
			url.setQuery(query);
			m_httpHandler->get(url.toString());
		}
	}

	// 刷新成员列表（优先使用REST API从腾讯云IM获取最新成员列表）
	void refreshMemberList(QString groupId)
	{
		m_unique_group_id = groupId;
		
		// 优先使用REST API获取群成员列表
		if (m_groupInfo && !m_unique_group_id.isEmpty())
		{
			m_groupInfo->fetchGroupMemberListFromREST(m_unique_group_id);
		}
		
		// 备用方案：调用自己的服务器接口获取群组成员列表
		if (m_httpHandler && !m_unique_group_id.isEmpty())
		{
			// 使用QUrl和QUrlQuery来正确编码URL参数（特别是#等特殊字符）
			QUrl url("http://47.100.126.194:5000/groups/members");
			QUrlQuery query;
			query.addQueryItem("group_id", m_unique_group_id);
			url.setQuery(query);
			m_httpHandler->get(url.toString());
			qDebug() << "刷新成员列表，群组ID:" << m_unique_group_id;
		}
	}

	bool startAudioCapture(const QAudioFormat& preferredFormat) {
		QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
		if (!info.isNull()) {
			qDebug() << "当前输入设备:" << info.deviceName();
		}

		QAudioFormat fmt = preferredFormat;
		if (!info.isFormatSupported(fmt)) {
			qWarning() << "输入设备不支持请求的格式，使用 nearestFormat";
			fmt = info.nearestFormat(fmt);
		}

		stopAudioCapture();

		audioInput = new QAudioInput(info, fmt, this);
		audioInput->setBufferSize(4096);

		inputDevice = audioInput->start();
		if (!inputDevice) {
			qCritical() << "AudioInput start() 失败";
			delete audioInput;
			audioInput = nullptr;
			return false;
		}

		m_audioFormat = fmt;

		connect(inputDevice, &QIODevice::readyRead, this, &ScheduleDialog::onReadyRead);
		qDebug() << "AudioInput 已启动，采样率:" << fmt.sampleRate() << "声道:" << fmt.channelCount();
		return true;
	}

	void stopAudioCapture() {
		if (inputDevice) {
			inputDevice->disconnect(this);
			inputDevice = nullptr;
		}
		if (audioInput) {
			audioInput->stop();
			delete audioInput;
			audioInput = nullptr;
		}
	}

private slots:
	void onReadyRead() {
		if (!inputDevice) {
			return;
		}
		QByteArray data = inputDevice->readAll();
		if (data.isEmpty()) {
			return;
		}
		if (m_rtmpStreamer && m_rtmpStreamer->isRunning()) {
			m_rtmpStreamer->pushPcm(data);
		}
	}

protected:
	void enterEvent(QEvent* event) override
	{
		QDialog::enterEvent(event);
		if (closeButton)
			closeButton->show();
	}

	void leaveEvent(QEvent* event) override
	{
		QDialog::leaveEvent(event);
		if (closeButton)
			closeButton->hide();
	}

	void resizeEvent(QResizeEvent* event) override
	{
		QDialog::resizeEvent(event);
        if (closeButton)
            closeButton->move(this->width() - closeButton->width() - 4, 4);
        updateMask();
	}

	void mousePressEvent(QMouseEvent* event) override
	{
		if (event->button() == Qt::LeftButton) {
			m_dragging = true;
			m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
		}
		QDialog::mousePressEvent(event);
	}

	void mouseMoveEvent(QMouseEvent* event) override
	{
		if (m_dragging && (event->buttons() & Qt::LeftButton)) {
			move(event->globalPos() - m_dragStartPos);
		}
		QDialog::mouseMoveEvent(event);
	}

	void mouseReleaseEvent(QMouseEvent* event) override
	{
		if (event->button() == Qt::LeftButton) {
			m_dragging = false;
		}
		QDialog::mouseReleaseEvent(event);
	}

    void paintEvent(QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor("#5C5C5C"));
        painter.setPen(Qt::NoPen);

        QPainterPath path;
        path.addRoundedRect(rect(), m_cornerRadius, m_cornerRadius);
        painter.drawPath(path);
    }

private:
	bool m_dragging = false; // 是否正在拖动
	QPoint m_dragStartPos; // 拖动起始位置
	QPushButton* closeButton = nullptr; // 关闭按钮
    int m_cornerRadius = 16;

    void updateMask()
    {
        QPainterPath path;
        path.addRoundedRect(rect(), m_cornerRadius, m_cornerRadius);
        setMask(QRegion(path.toFillPolygon().toPolygon()));
    }
	CustomListDialog* customListDlg = NULL;
	QLabel* m_lblClass = NULL;
	QString m_groupName;
	QString m_unique_group_id;
	TAHttpHandler* m_taHttpHandler = NULL;
	ChatDialog* m_chatDlg = NULL;
	TaQTWebSocket* m_pWs = NULL;
	bool m_iGroupOwner = false;
	QString m_classid;
	QAudioInput* audioInput = nullptr;
	QIODevice* inputDevice = nullptr;
	QAudioFormat m_audioFormat;
	QString m_userId;
	QString m_userName;
	QPushButton* btnTalk = NULL;
	// 班级群功能按钮指针（普通群不显示）
	QPushButton* m_btnSeat = nullptr;
	QPushButton* m_btnCam = nullptr;
	QPushButton* m_btnMsg = nullptr;
	QPushButton* m_btnTask = nullptr;
	bool m_isClassGroup = true; // 默认为班级群
	QGroupInfo* m_groupInfo;
	TAHttpHandler* m_httpHandler = NULL;
	// 用于记录按下开始时间
	qint64 pressStartMs = 0;
	//QProgressBar* m_volumeBar = nullptr;
	//QFile localRecordFile;
	//bool isLocalRecording = false;
	// RTMP 推流器
	RtmpMediaStreamer* m_rtmpStreamer = nullptr;
	QVector<GroupMemberInfo>  m_groupMemberInfo;
	QTableWidget* seatTable = nullptr; // 座位表格
	ArrangeSeatDialog* arrangeSeatDlg = nullptr; // 排座对话框
	QList<StudentInfo> m_students; // 学生数据
	
	// 热力图相关
	class HeatmapSegmentDialog* heatmapSegmentDlg = nullptr; // 分段区间对话框
	class HeatmapViewDialog* heatmapViewDlg = nullptr; // 热力图显示窗口
	QList<struct SegmentRange> m_segments; // 分段区间列表
	int m_heatmapType = 1; // 1=分段，2=渐变
	
	// 随机点名相关
	class RandomCallDialog* randomCallDlg = nullptr; // 随机点名对话框
	void connectRandomCallButton(QPushButton* btnRandom, QTableWidget* seatTable); // 连接随机点名按钮
	
	// 作业相关
	class HomeworkEditDialog* homeworkEditDlg = nullptr; // 编辑作业对话框
	class HomeworkViewDialog* homeworkViewDlg = nullptr; // 展示作业对话框
	void connectHomeworkButton(QPushButton* btnTask); // 连接作业按钮（教师端）
	void showHomeworkViewDialog(); // 显示作业展示窗口（班级端）
	
	// 更新座位颜色（根据分段或渐变）
	void updateSeatColors();
};

// 实现排座方法
inline void ScheduleDialog::arrangeSeats(const QList<StudentInfo>& students, const QString& method)
{
	qDebug() << "arrangeSeats 被调用，学生数量:" << students.size() << "，排座方式:" << method;
	
	if (!seatTable) {
		qDebug() << "错误：seatTable 为空！";
		return;
	}
	
	qDebug() << "seatTable 存在，开始排座...";
	
	m_students = students;
	
	// 获取所有座位按钮（标记为 isSeat 的按钮），按行列顺序排列
	QList<QPushButton*> seatButtons;
	for (int row = 0; row < 8; ++row) {
		for (int col = 0; col < 10; ++col) {
			QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
			if (btn && btn->property("isSeat").toBool()) {
				seatButtons.append(btn);
			}
		}
	}
	
	qDebug() << "找到座位按钮数量:" << seatButtons.size();
	
	// 如果学生数量为0，清空所有座位
	if (students.isEmpty()) {
		for (QPushButton* btn : seatButtons) {
			btn->setText("");
			btn->setProperty("studentId", QVariant(""));
			btn->setProperty("studentName", QVariant(""));
		}
		return;
	}
	
	QList<StudentInfo> arrangedStudents;
	
	// 根据排座方式处理学生数据
	if (method == "正序") {
		// 按成绩从高到低排序
		arrangedStudents = students;
		std::sort(arrangedStudents.begin(), arrangedStudents.end(), 
			[](const StudentInfo& a, const StudentInfo& b) {
				return a.score > b.score; // 降序
			});
	} else if (method == "倒序") {
		// 按成绩从低到高排序
		arrangedStudents = students;
		std::sort(arrangedStudents.begin(), arrangedStudents.end(), 
			[](const StudentInfo& a, const StudentInfo& b) {
				return a.score < b.score; // 升序
			});
	} else if (method == "随机排座") {
		// 随机打乱学生顺序
		arrangedStudents = students;
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(arrangedStudents.begin(), arrangedStudents.end(), g);
	} else if (method == "2人组排座") {
		// 2人组：同桌帮扶
		// 按成绩降序排列，然后相邻两人一组
		arrangedStudents = students;
		std::sort(arrangedStudents.begin(), arrangedStudents.end(), 
			[](const StudentInfo& a, const StudentInfo& b) {
				return a.score > b.score;
			});
		// 将相邻两人配对，然后打乱配对顺序
		QList<QList<StudentInfo>> pairs;
		for (int i = 0; i < arrangedStudents.size() - 1; i += 2) {
			QList<StudentInfo> pair;
			pair.append(arrangedStudents[i]);
			pair.append(arrangedStudents[i + 1]);
			pairs.append(pair);
		}
		// 如果学生数量为奇数，最后一个单独一组
		if (arrangedStudents.size() % 2 == 1) {
			QList<StudentInfo> pair;
			pair.append(arrangedStudents.last());
			pairs.append(pair);
		}
		// 打乱配对顺序
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(pairs.begin(), pairs.end(), g);
		// 展开配对
		arrangedStudents.clear();
		for (const QList<StudentInfo>& pair : pairs) {
			arrangedStudents.append(pair);
		}
	} else if (method == "4人组排座" || method == "6人组排座") {
		// 4人组或6人组：按成绩降序排列，然后分组
		int groupSize = (method == "4人组排座") ? 4 : 6;
		arrangedStudents = students;
		std::sort(arrangedStudents.begin(), arrangedStudents.end(), 
			[](const StudentInfo& a, const StudentInfo& b) {
				return a.score > b.score;
			});
		
		// 计算完整组数和剩余人数
		int totalStudents = arrangedStudents.size();
		int fullGroups = totalStudents / groupSize;
		int remaining = totalStudents % groupSize;
		
		// 将学生分成 groupSize 等份
		QList<QList<StudentInfo>> levels;
		levels.reserve(groupSize);
		for (int i = 0; i < groupSize; ++i) {
			levels.append(QList<StudentInfo>());
		}
		
		// 将前 fullGroups * groupSize 个学生分配到等份中
		for (int i = 0; i < fullGroups * groupSize; ++i) {
			int levelIndex = i % groupSize;
			levels[levelIndex].append(arrangedStudents[i]);
		}
		
		// 从每个等份中随机选取一个学生组成一组
		QList<QList<StudentInfo>> groups;
		std::random_device rd;
		std::mt19937 g(rd());
		
		for (int groupIdx = 0; groupIdx < fullGroups; ++groupIdx) {
			QList<StudentInfo> group;
			for (int levelIdx = 0; levelIdx < groupSize; ++levelIdx) {
				if (!levels[levelIdx].isEmpty()) {
					// 随机选择一个学生
					std::uniform_int_distribution<> dis(0, levels[levelIdx].size() - 1);
					int randomIdx = dis(g);
					group.append(levels[levelIdx][randomIdx]);
					levels[levelIdx].removeAt(randomIdx);
				}
			}
			groups.append(group);
		}
		
		// 剩余学生组成最后一组
		if (remaining > 0) {
			QList<StudentInfo> lastGroup;
			for (int i = fullGroups * groupSize; i < totalStudents; ++i) {
				lastGroup.append(arrangedStudents[i]);
			}
			groups.append(lastGroup);
		}
		
		// 打乱组顺序
		std::shuffle(groups.begin(), groups.end(), g);
		
		// 展开组
		arrangedStudents.clear();
		for (const QList<StudentInfo>& group : groups) {
			arrangedStudents.append(group);
		}
	} else {
		// 默认随机排座
		arrangedStudents = students;
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(arrangedStudents.begin(), arrangedStudents.end(), g);
	}
	
	// 将学生分配到座位上（从左到右，从上到下）
	int studentIndex = 0;
	for (QPushButton* btn : seatButtons) {
		if (studentIndex < arrangedStudents.size()) {
			const StudentInfo& student = arrangedStudents[studentIndex];
			btn->setText(student.name); // 显示学生姓名
			btn->setProperty("studentId", QVariant(student.id)); // 设置学号属性
			btn->setProperty("studentName", QVariant(student.name));
			qDebug() << "分配座位:" << student.name << "到按钮";
			studentIndex++;
		} else {
			btn->setText("");
			btn->setProperty("studentId", QVariant(""));
			btn->setProperty("studentName", QVariant(""));
		}
	}
	
	qDebug() << "排座完成，共分配" << studentIndex << "个学生";
}

// 导入座位表格
inline void ScheduleDialog::importSeatTable()
{
	QString fileName = QFileDialog::getOpenFileName(
		this,
		QString::fromUtf8(u8"选择座位表文件"),
		QString(),
		QString::fromUtf8(u8"Excel 文件 (*.xlsx *.xls);;CSV 文件 (*.csv);;所有文件 (*.*)")
	);
	
	if (fileName.isEmpty()) {
		return;
	}
	
	// 判断文件类型
	if (fileName.endsWith(".xlsx", Qt::CaseInsensitive) || fileName.endsWith(".xls", Qt::CaseInsensitive)) {
		importSeatFromExcel(fileName);
	} else if (fileName.endsWith(".csv", Qt::CaseInsensitive)) {
		importSeatFromCsv(fileName);
	} else {
		QMessageBox::warning(this, QString::fromUtf8(u8"提示"), 
			QString::fromUtf8(u8"不支持的文件格式，请选择 .xlsx、.xls 或 .csv 文件"));
	}
}

// 从Excel导入座位表
inline void ScheduleDialog::importSeatFromExcel(const QString& filePath)
{
	using namespace QXlsx;
	
	Document xlsx(filePath, this);
	if (!xlsx.isLoadPackage()) {
		QMessageBox::critical(this, QString::fromUtf8(u8"错误"), 
			QString::fromUtf8(u8"无法打开 Excel 文件：") + filePath);
		return;
	}
	
	// 获取第一个工作表
	QStringList sheetNames = xlsx.sheetNames();
	if (sheetNames.isEmpty()) {
		QMessageBox::warning(this, QString::fromUtf8(u8"提示"), 
			QString::fromUtf8(u8"Excel 文件中没有工作表"));
		return;
	}
	
	// 读取数据范围：从第4行、第2列开始，最多16行、12列
	// 如果一行或一列都是空格，就忽略
	QList<QStringList> dataRows;
	int startRow = 4; // 从第4行开始
	int startCol = 2; // 从第2列开始
	int maxRows = 16; // 最多16行
	int maxCols = 12; // 最多12列
	
	// 先读取所有数据
	QList<QStringList> allDataRows;
	for (int row = startRow; row < startRow + maxRows; ++row) {
		QStringList rowData;
		bool rowHasData = false;
		for (int col = startCol; col < startCol + maxCols; ++col) {
			QVariant cellValue = xlsx.read(row, col);
			QString cellText = cellValue.toString().trimmed();
			rowData.append(cellText);
			if (!cellText.isEmpty()) {
				rowHasData = true;
			}
		}
		// 如果这一行有数据，才添加到结果中
		if (rowHasData) {
			allDataRows.append(rowData);
		}
	}
	
	// 过滤掉全空的列
	if (!allDataRows.isEmpty()) {
		QList<int> validCols; // 有效列的索引
		for (int col = 0; col < allDataRows[0].size(); ++col) {
			bool colHasData = false;
			for (int row = 0; row < allDataRows.size(); ++row) {
				if (!allDataRows[row][col].isEmpty()) {
					colHasData = true;
					break;
				}
			}
			if (colHasData) {
				validCols.append(col);
			}
		}
		
		// 只保留有效列的数据
		for (int row = 0; row < allDataRows.size(); ++row) {
			QStringList validRowData;
			for (int colIdx : validCols) {
				validRowData.append(allDataRows[row][colIdx]);
			}
			dataRows.append(validRowData);
		}
	}
	
	fillSeatTableFromData(dataRows);
	QMessageBox::information(this, QString::fromUtf8(u8"导入成功"), 
		QString::fromUtf8(u8"座位表导入成功！"));
	
	// 导入成功后自动上传到服务器
	uploadSeatTableToServer();
}

// 从CSV导入座位表
inline void ScheduleDialog::importSeatFromCsv(const QString& filePath)
{
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QMessageBox::critical(this, QString::fromUtf8(u8"错误"), 
			QString::fromUtf8(u8"无法打开 CSV 文件：") + filePath);
		return;
	}
	
	QTextStream in(&file);
	in.setCodec("UTF-8");
	
	// 读取所有行（跳过前3行，从第4行开始）
	QList<QStringList> allLines;
	int lineNumber = 0;
	while (!in.atEnd() && lineNumber < 20) { // 读取足够多的行
		QString line = in.readLine();
		lineNumber++;
		
		// 跳过前3行（行号1-3）
		if (lineNumber < 4) {
			continue;
		}
		
		QStringList rowData;
		
		// 简单的CSV解析（按逗号分割，处理引号）
		QString current;
		bool inQuotes = false;
		for (int i = 0; i < line.length(); ++i) {
			QChar ch = line[i];
			if (ch == '"') {
				inQuotes = !inQuotes;
			} else if (ch == ',' && !inQuotes) {
				rowData.append(current.trimmed());
				current.clear();
			} else {
				current += ch;
			}
		}
		rowData.append(current.trimmed());
		
		// 跳过第1列（索引0），从第2列开始（索引1）
		if (rowData.size() > 1) {
			rowData = rowData.mid(1); // 从第2列开始
		} else {
			rowData.clear();
		}
		
		// 最多取12列
		if (rowData.size() > 12) {
			rowData = rowData.mid(0, 12);
		}
		
		allLines.append(rowData);
		
		// 最多读取16行数据（加上前3行，总共不超过20行）
		if (lineNumber - 3 >= 16) {
			break;
		}
	}
	
	file.close();
	
	// 过滤掉全空的行
	QList<QStringList> allDataRows;
	for (const QStringList& rowData : allLines) {
		bool rowHasData = false;
		for (const QString& cell : rowData) {
			if (!cell.isEmpty()) {
				rowHasData = true;
				break;
			}
		}
		if (rowHasData) {
			allDataRows.append(rowData);
		}
	}
	
	// 过滤掉全空的列
	QList<QStringList> dataRows;
	if (!allDataRows.isEmpty()) {
		QList<int> validCols; // 有效列的索引
		int maxColCount = 0;
		for (const QStringList& row : allDataRows) {
			if (row.size() > maxColCount) {
				maxColCount = row.size();
			}
		}
		
		for (int col = 0; col < maxColCount; ++col) {
			bool colHasData = false;
			for (int row = 0; row < allDataRows.size(); ++row) {
				if (col < allDataRows[row].size() && !allDataRows[row][col].isEmpty()) {
					colHasData = true;
					break;
				}
			}
			if (colHasData) {
				validCols.append(col);
			}
		}
		
		// 只保留有效列的数据
		for (int row = 0; row < allDataRows.size(); ++row) {
			QStringList validRowData;
			for (int colIdx : validCols) {
				if (colIdx < allDataRows[row].size()) {
					validRowData.append(allDataRows[row][colIdx]);
				} else {
					validRowData.append("");
				}
			}
			dataRows.append(validRowData);
		}
	}
	
	fillSeatTableFromData(dataRows);
	QMessageBox::information(this, QString::fromUtf8(u8"导入成功"), 
		QString::fromUtf8(u8"座位表导入成功！"));
	
	// 导入成功后自动上传到服务器
	uploadSeatTableToServer();
}

// 将数据填充到座位表
inline void ScheduleDialog::fillSeatTableFromData(const QList<QStringList>& dataRows)
{
	if (!seatTable) {
		QMessageBox::warning(this, QString::fromUtf8(u8"错误"), 
			QString::fromUtf8(u8"座位表未初始化"));
		return;
	}
	
	// 清空所有座位
	for (int row = 0; row < 8; ++row) {
		for (int col = 0; col < 10; ++col) {
			QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
			if (btn && btn->property("isSeat").toBool()) {
				btn->setText("");
				btn->setProperty("studentId", QVariant());
				btn->setProperty("studentName", QVariant());
			}
		}
	}
	
	if (dataRows.isEmpty()) {
		qDebug() << "导入的数据为空";
		return;
	}
	
	// 映射关系：导入的数据映射到seatTable的实际座位位置
	// Excel的讲台在下面，所以Excel的最后一行（离讲台最近）应该映射到seatTable的第一行（窗口上面的第一排）
	// 行顺序反转：最后一行 -> seatTable第1行，第一行 -> seatTable最后一行
	// 最多处理8行数据（seatTable只有8行）
	int maxRows = qMin(dataRows.size(), 8);
	
	for (int importRow = 0; importRow < maxRows; ++importRow) {
		const QStringList& rowData = dataRows[importRow];
		// 反转行顺序：最后一行（importRow=dataRows.size()-1）-> seatTable第1行（seatTableRow=0）
		// 计算实际在数据中的位置（从后往前）
		int actualDataRow = dataRows.size() - 1 - importRow;
		const QStringList& actualRowData = dataRows[actualDataRow];
		// seatTable的行号（0-7）
		int seatTableRow = importRow;
		
		if (seatTableRow == 0) {
			// 第1行：左侧2个座位（列0-1），右侧2个座位（列8-9）
			// Excel原始布局：左座位 | 左走道 | 中走道 | 右座位 | 右走道
			// Excel过滤走道后，数据按"左-右"顺序：列0-1是左座位，列2-3是右座位
			// 左座位：Excel列0-1 -> seatTable列0-1
			for (int i = 0; i < 2 && i < actualRowData.size(); ++i) {
				if (!actualRowData[i].isEmpty()) {
					QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(0, i));
					if (btn && btn->property("isSeat").toBool()) {
						btn->setText(actualRowData[i]);
						btn->setProperty("studentName", actualRowData[i]);
						QStringList parts = actualRowData[i].split(QRegExp("[\\s-]+"));
						if (parts.size() >= 2) {
							btn->setProperty("studentId", parts[1]);
						}
						btn->update();
					}
				}
			}
			// 右座位：Excel列2-3 -> seatTable列8-9
			for (int i = 2; i < 4 && i < actualRowData.size(); ++i) {
				if (!actualRowData[i].isEmpty()) {
					int seatTableCol = i + 6; // 2->8, 3->9
					QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(0, seatTableCol));
					if (btn && btn->property("isSeat").toBool()) {
						btn->setText(actualRowData[i]);
						btn->setProperty("studentName", actualRowData[i]);
						QStringList parts = actualRowData[i].split(QRegExp("[\\s-]+"));
						if (parts.size() >= 2) {
							btn->setProperty("studentId", parts[1]);
						}
						btn->update();
					}
				}
			}
		} else {
			// 第2-8行：左列3个（列0-2），中列3个（列4-6），右列2个（列8-9）
			// Excel原始布局：左座位 | 左走道 | 中左座位 | 中走道 | 中右座位 | 右走道 | 右座位
			// Excel过滤走道后，数据按"左-中左-中右-右"顺序
			// 假设：左座位3个，中左座位2个，中右座位1个，右座位2个（共8个座位）
			// 左座位：Excel列0-2 -> seatTable列0-2
			for (int i = 0; i < 3 && i < actualRowData.size(); ++i) {
				if (!actualRowData[i].isEmpty()) {
					QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(seatTableRow, i));
					if (btn && btn->property("isSeat").toBool()) {
						btn->setText(actualRowData[i]);
						btn->setProperty("studentName", actualRowData[i]);
						QStringList parts = actualRowData[i].split(QRegExp("[\\s-]+"));
						if (parts.size() >= 2) {
							btn->setProperty("studentId", parts[1]);
						}
						btn->update();
					}
				}
			}
			// 中左座位：Excel列3-4 -> seatTable列4-5（中座位的前两个）
			for (int i = 3; i < 5 && i < actualRowData.size(); ++i) {
				if (!actualRowData[i].isEmpty()) {
					int seatTableCol = i + 1; // 3->4, 4->5
					QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(seatTableRow, seatTableCol));
					if (btn && btn->property("isSeat").toBool()) {
						btn->setText(actualRowData[i]);
						btn->setProperty("studentName", actualRowData[i]);
						QStringList parts = actualRowData[i].split(QRegExp("[\\s-]+"));
						if (parts.size() >= 2) {
							btn->setProperty("studentId", parts[1]);
						}
						btn->update();
					}
				}
			}
			// 中右座位：Excel列5 -> seatTable列6（中座位的最后一个）
			if (actualRowData.size() > 5 && !actualRowData[5].isEmpty()) {
				QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(seatTableRow, 6));
				if (btn && btn->property("isSeat").toBool()) {
					btn->setText(actualRowData[5]);
					btn->setProperty("studentName", actualRowData[5]);
					QStringList parts = actualRowData[5].split(QRegExp("[\\s-]+"));
					if (parts.size() >= 2) {
						btn->setProperty("studentId", parts[1]);
					}
					btn->update();
				}
			}
			// 右座位：Excel列6-7 -> seatTable列8-9
			for (int i = 6; i < 8 && i < actualRowData.size(); ++i) {
				if (!actualRowData[i].isEmpty()) {
					int seatTableCol = i + 2; // 6->8, 7->9
					QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(seatTableRow, seatTableCol));
					if (btn && btn->property("isSeat").toBool()) {
						btn->setText(actualRowData[i]);
						btn->setProperty("studentName", actualRowData[i]);
						QStringList parts = actualRowData[i].split(QRegExp("[\\s-]+"));
						if (parts.size() >= 2) {
							btn->setProperty("studentId", parts[1]);
						}
						btn->update();
					}
				}
			}
		}
	}
	
	// 刷新整个表格以确保界面更新
	seatTable->update();
	seatTable->repaint();
	qDebug() << "座位表数据已刷新，共导入" << dataRows.size() << "行数据";
}

// 上传座位表到服务器
inline void ScheduleDialog::uploadSeatTableToServer()
{
	if (!seatTable || m_classid.isEmpty()) {
		QMessageBox::warning(this, QString::fromUtf8(u8"错误"), 
			QString::fromUtf8(u8"座位表未初始化或班级ID为空"));
		return;
	}
	
	if (!m_httpHandler) {
		QMessageBox::warning(this, QString::fromUtf8(u8"错误"), 
			QString::fromUtf8(u8"HTTP处理器未初始化"));
		return;
	}
	
	// 从seatTable提取座位数据
	QJsonArray seatsArray;
	for (int row = 0; row < 8; ++row) {
		for (int col = 0; col < 10; ++col) {
			QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
			if (btn && btn->property("isSeat").toBool()) {
				QString studentName = btn->text().trimmed();
				if (!studentName.isEmpty()) {
					QJsonObject seatObj;
					seatObj["row"] = row + 1; // 行号从1开始
					seatObj["col"] = col + 1; // 列号从1开始
					seatObj["student_name"] = studentName;
					
					// 解析学生姓名和编号（如"刘峻源8-4"）
					QStringList parts = studentName.split(QRegExp("[\\s-]+"));
					if (parts.size() >= 1) {
						seatObj["name"] = parts[0]; // 姓名
						if (parts.size() >= 2) {
							seatObj["student_id"] = parts[1]; // 编号
						}
					}
					
					seatsArray.append(seatObj);
				}
			}
		}
	}
	
	if (seatsArray.isEmpty()) {
		QMessageBox::warning(this, QString::fromUtf8(u8"提示"), 
			QString::fromUtf8(u8"座位表中没有学生数据"));
		return;
	}
	
	// 构造请求JSON
	QJsonObject requestObj;
	requestObj["class_id"] = m_classid;
	requestObj["seats"] = seatsArray;
	
	QJsonDocument doc(requestObj);
	QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
	
	// 发送POST请求
	QString url = "http://47.100.126.194:5000/seat-arrangement/save";
	
	// 使用TAHttpHandler发送请求
	connect(m_httpHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
		m_httpHandler->disconnect(this);
		
		QJsonDocument respDoc = QJsonDocument::fromJson(responseString.toUtf8());
		if (respDoc.isObject()) {
			QJsonObject obj = respDoc.object();
			int code = obj.value("code").toInt(-1);
			if (code == 200 || code == 0) {
				QMessageBox::information(this, QString::fromUtf8(u8"上传成功"), 
					QString::fromUtf8(u8"座位表已成功上传到服务器！"));
			} else {
				QString errorMsg = obj.value("message").toString();
				QMessageBox::warning(this, QString::fromUtf8(u8"上传失败"), 
					QString::fromUtf8(u8"服务器返回错误：%1").arg(errorMsg));
			}
		} else {
			QMessageBox::information(this, QString::fromUtf8(u8"上传成功"), 
				QString::fromUtf8(u8"座位表已成功上传到服务器！"));
		}
	});
	
	connect(m_httpHandler, &TAHttpHandler::failed, this, [=](const QString& error) {
		m_httpHandler->disconnect(this);
		QMessageBox::critical(this, QString::fromUtf8(u8"上传失败"), 
			QString::fromUtf8(u8"网络错误：%1").arg(error));
	});
	
	m_httpHandler->post(url, jsonData);
	qDebug() << "开始上传座位表到服务器，班级ID:" << m_classid << "，座位数量:" << seatsArray.size();
}

// 热力图相关方法的实现在 ScheduleDialog_Heatmap.cpp 中
