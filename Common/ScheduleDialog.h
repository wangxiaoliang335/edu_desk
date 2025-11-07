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
#include <QAudioInput>
#include <QAudioFormat>
#include <QIODevice>
#include <qprogressbar.h>
#include <QPoint>
#include <QBrush>
#include <QColor>
#include <QUrl>
#include <QUrlQuery>
#include <QGroupInfo.h>
#include <Windows.h>
#include "CustomListDialog.h"
#include "ClickableLabel.h"
#include "TAHttpHandler.h"
#include "ChatDialog.h"
#include "CommonInfo.h"
#include "QGroupInfo.h"
#include "TAHttpHandler.h"
#include "ArrangeSeatDialog.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

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
	ScheduleDialog(QWidget* parent = nullptr, TaQTWebSocket* pWs = NULL) : QDialog(parent)
	{
		setWindowTitle("课程表");
		resize(700, 500);
		setStyleSheet("QPushButton { font-size:14px; } QLabel { font-size:14px; }");
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
						QString group_id = dataObj["group_id"].toString();
						
						// 处理 /groups/members 接口返回的成员列表
						if (dataObj["members"].isArray())
						{
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
		customListDlg = new CustomListDialog(this);
		QVBoxLayout* mainLayout = new QVBoxLayout(this);
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
		btnTalk = new QPushButton("按住开始对讲");
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

		middleBtnLayout->addStretch();
		middleBtnLayout->addWidget(btnRandom);
		middleBtnLayout->addWidget(btnAnalyse);
		middleBtnLayout->addWidget(btnHeatmap);
		middleBtnLayout->addWidget(btnArrange);
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
		// 第2-4行：每行9个座位（左列3个 + 中列3个 + 右列3个，中间有两条过道）
		// 总共31个座位
		seatTable = new QTableWidget(4, 11); // 4行，11列（包含过道列）
		seatTable->horizontalHeader()->setVisible(false);
		seatTable->verticalHeader()->setVisible(false);
		seatTable->setStyleSheet(
			"QTableWidget { "
			"background-color: #f5f5dc; "
			"gridline-color: #ccc; "
			"border: 1px solid #ccc; "
			"}"
			"QTableWidget::item { "
			"padding: 5px; "
			"}"
		);
		
		// 设置列宽和行高
		for (int col = 0; col < 11; ++col) {
			seatTable->setColumnWidth(col, 50);
		}
		for (int row = 0; row < 4; ++row) {
			seatTable->setRowHeight(row, 50);
		}
		
		// 初始化所有单元格，为每个单元格创建按钮
		for (int row = 0; row < 4; ++row) {
			for (int col = 0; col < 11; ++col) {
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
		// 左侧2个座位：列0-1，右侧2个座位：列9-10，中间列2-8合并为过道
		seatTable->setSpan(0, 2, 1, 7); // 合并第1行的列2-8作为中央过道
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
		
		// 第2-4行布局：每行9个座位
		// 左列3个：列0-2，过道列3，中列3个：列4-6，过道列7，右列3个：列8-10
		for (int row = 1; row < 4; ++row) {
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
			
			// 右列座位：列8-10
			for (int col = 8; col < 11; ++col) {
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
		// 右侧2个座位：列9-10
		for (int col = 9; col < 11; ++col) {
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
		
		mainLayout->addWidget(seatTable);

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
		// 按下按钮 -> 开始采集
		connect(btnTalk, &QPushButton::pressed, this, [=]() {
			pressStartMs = QDateTime::currentMSecsSinceEpoch();
			btnTalk->setStyleSheet("background-color: red; color: white; padding: 4px 8px; font-size:14px;");
			btnTalk->setText("录音中...松开结束");
			qDebug() << "开始对讲（按钮按下）";
			start();

			// 发送开始包（flag=0）
			QByteArray empty;
			encodeAndSend(empty, 0);
			//btnTalk->setText("松开结束对讲");
			});

		// 松开按钮 -> 停止采集
		connect(btnTalk, &QPushButton::released, this, [=]() {
			/*stop();
			btnTalk->setText("按住开始对讲");*/
			qint64 releaseMs = QDateTime::currentMSecsSinceEpoch();
			qint64 duration = releaseMs - pressStartMs;

			// 发送结束包（flag=2）
			QByteArray empty;
			encodeAndSend(empty, 2);

			stop();  // 停止采集

			if (duration < 500) {
				qDebug() << "录音时间过短(" << duration << "ms)，丢弃";
				// 这里可以直接 return 或做短音频忽略逻辑
			}
			else {
				qDebug() << "录音完成，时长:" << duration << "ms，已发送音频";
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
	void InitData(QString groupName, QString unique_group_id, bool iGroupOwner)
	{
		m_groupName = groupName;
		m_unique_group_id = unique_group_id;
		m_iGroupOwner = iGroupOwner;
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
			m_groupInfo->initData(groupName, unique_group_id);
		}

		// 调用获取群组成员列表的接口
		if (m_httpHandler && !unique_group_id.isEmpty())
		{
			// 使用QUrl和QUrlQuery来正确编码URL参数（特别是#等特殊字符）
			QUrl url("http://47.100.126.194:5000/groups/members");
			QUrlQuery query;
			query.addQueryItem("group_id", unique_group_id);
			url.setQuery(query);
			m_httpHandler->get(url.toString());
		}
	}

	// 刷新成员列表（从服务器获取最新成员列表）
	void refreshMemberList(QString groupId)
	{
		m_unique_group_id = groupId;
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

	void start() {
		// 枚举可用输入设备
		QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
		qDebug() << "===== 可用麦克风设备列表 =====";
		for (auto& dev : devices) {
			qDebug() << "设备:" << dev.deviceName();
		}

		// 准备采样格式
		QAudioFormat fmt;
		fmt.setSampleRate(44100);
		fmt.setChannelCount(2);
		fmt.setSampleSize(16);
		fmt.setCodec("audio/pcm");
		fmt.setByteOrder(QAudioFormat::LittleEndian);
		fmt.setSampleType(QAudioFormat::SignedInt);

		// 检查当前默认设备
		QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
		qDebug() << "默认输入设备:" << info.deviceName();

		// 如果不支持，回退到最近格式
		if (!info.isFormatSupported(fmt)) {
			qWarning() << "当前设备不支持 44100Hz 立体声 S16 格式, 使用 nearestFormat";
			fmt = info.nearestFormat(fmt);
		}

		// 打印最终使用的格式
		qDebug() << "使用格式:"
			<< fmt.sampleRate() << "Hz"
			<< fmt.channelCount() << "声道"
			<< fmt.sampleSize() << "bit"
			<< fmt.codec();

		// 创建输入实例
		audioInput = new QAudioInput(info, fmt, this);

		// 让 readyRead 提前触发
		audioInput->setBufferSize(4096);

		// 初始化编码器
		initEncoder();

		// 启动采集
		inputDevice = audioInput->start();
		if (!inputDevice) {
			qCritical() << "❌ AudioInput start() 失败，可能是系统权限或设备问题";
			return;
		}
		else {
			qDebug() << "✅ AudioInput 已启动，等待 readyRead 事件...";
		}

		// 绑定 readyRead
		connect(inputDevice, &QIODevice::readyRead, this, &ScheduleDialog::onReadyRead);

		// 额外定时器监控（可选）
		QTimer::singleShot(3000, this, [=]() {
			if (audioInput && audioInput->state() != QAudio::ActiveState) {
				qWarning() << "⚠️ AudioInput 未处于 ActiveState, 当前状态:" << audioInput->state();
			}
			});

		//// 打开本地存储文件
		//localRecordFile.setFileName("test_local.aac"); // 也可以改成带时间戳
		//if (localRecordFile.open(QIODevice::WriteOnly)) {
		//	isLocalRecording = true;
		//	qDebug() << "✅ 本地录音文件已打开: test_local.aac";
		//}
		//else {
		//	qWarning() << "❌ 无法打开本地录音文件用于写入";
		//	isLocalRecording = false;
		//}
	}

	void stop() {
		if (audioInput) { audioInput->stop(); delete audioInput; audioInput = nullptr; }
		if (codecCtx) avcodec_free_context(&codecCtx);
		if (frame) av_frame_free(&frame);
		if (pkt) av_packet_free(&pkt);
		if (swrCtx) swr_free(&swrCtx);

		//if (isLocalRecording) {
		//	localRecordFile.close();
		//	isLocalRecording = false;
		//	qDebug() << "📁 本地录音文件已关闭";
		//}
	}

	void addADTSHeader(char* buf, int packetLen, int profile, int sampleRate, int channels)
	{
		int freqIdx;
		switch (sampleRate) {
		case 96000: freqIdx = 0; break;
		case 88200: freqIdx = 1; break;
		case 64000: freqIdx = 2; break;
		case 48000: freqIdx = 3; break;
		case 44100: freqIdx = 4; break;
		case 32000: freqIdx = 5; break;
		case 24000: freqIdx = 6; break;
		case 22050: freqIdx = 7; break;
		case 16000: freqIdx = 8; break;
		case 12000: freqIdx = 9; break;
		case 11025: freqIdx = 10; break;
		case 8000:  freqIdx = 11; break;
		case 7350:  freqIdx = 12; break;
		default:    freqIdx = 4; break;
		}

		int fullLen = packetLen + 7;
		buf[0] = 0xFF;
		buf[1] = 0xF1;
		buf[2] = ((profile - 1) << 6) | (freqIdx << 2) | (channels >> 2);
		buf[3] = ((channels & 3) << 6) | ((fullLen >> 11) & 0x03);
		buf[4] = (fullLen >> 3) & 0xFF;
		buf[5] = ((fullLen & 7) << 5) | 0x1F;
		buf[6] = 0xFC;
	}

	void encodeAndSend(const QByteArray& pcm, quint8 flag) {
		if (flag == 0 || flag == 2)
		{
			QByteArray packet;
			QDataStream ds(&packet, QIODevice::WriteOnly);
			ds.setByteOrder(QDataStream::LittleEndian);

			// ===== 打包帧 =====
			quint8 frameType = 6; // 音频帧
			ds << frameType;

			// 加flag
			ds << flag;

			QByteArray groupIdBytes = m_unique_group_id.toUtf8();
			quint32 groupIdLen = groupIdBytes.size();
			ds << groupIdLen;
			ds.writeRawData(groupIdBytes.constData(), groupIdLen);

			QByteArray senderIdBytes = m_userId.toUtf8();
			quint32 senderIdLen = senderIdBytes.size();
			ds << senderIdLen;
			ds.writeRawData(senderIdBytes.constData(), senderIdLen);

			QByteArray senderNameBytes = m_userName.toUtf8();
			quint32 senderNameLen = senderNameBytes.size();
			ds << senderNameLen;
			ds.writeRawData(senderNameBytes.constData(), senderNameLen);

			quint64 ts = QDateTime::currentMSecsSinceEpoch();
			ds << ts;

			quint32 aacLen = pcm.size();
			ds << aacLen;
			ds.writeRawData(pcm.constData(), aacLen);

			//m_ws.sendBinaryMessage(packet);
			TaQTWebSocket::sendBinaryMessage(packet);
			return;
		}

		//sprintf(m_szTmp, "pcm size:%d\n", pcm.size());
		//OutputDebugStringA(m_szTmp);

		int16_t* pcmData = (int16_t*)pcm.data();
		int numSamples = pcm.size() / (2 * codecCtx->channels);
		const uint8_t* inData[1] = { (uint8_t*)pcmData };
		swr_convert(swrCtx, frame->data, frame->nb_samples, inData, numSamples);

		if (avcodec_send_frame(codecCtx, frame) >= 0) {
			while (avcodec_receive_packet(codecCtx, pkt) == 0) {
				QByteArray aacData((char*)pkt->data, pkt->size);

				QByteArray packet;
				QDataStream ds(&packet, QIODevice::WriteOnly);
				ds.setByteOrder(QDataStream::LittleEndian);

				// 构造带ADTS的包
				QByteArray aacWithADTS;
				aacWithADTS.resize(aacData.size() + 7);
				addADTSHeader(aacWithADTS.data(), aacData.size(), 2, 44100, 2); // LC, 44100Hz, stereo
				memcpy(aacWithADTS.data() + 7, aacData.constData(), aacData.size());

				//// 本地保存
				//if (isLocalRecording && localRecordFile.isOpen()) {
				//	localRecordFile.write(aacWithADTS);
				//}

				// ===== 打包帧 =====
				quint8 frameType = 6; // 音频帧
				ds << frameType;

				// 加flag
				ds << flag;

				QByteArray groupIdBytes = m_unique_group_id.toUtf8();
				quint32 groupIdLen = groupIdBytes.size();
				ds << groupIdLen;
				ds.writeRawData(groupIdBytes.constData(), groupIdLen);

				QByteArray senderIdBytes = m_userId.toUtf8();
				quint32 senderIdLen = senderIdBytes.size();
				ds << senderIdLen;
				ds.writeRawData(senderIdBytes.constData(), senderIdLen);

				QByteArray senderNameBytes = m_userName.toUtf8();
				quint32 senderNameLen = senderNameBytes.size();
				ds << senderNameLen;
				ds.writeRawData(senderNameBytes.constData(), senderNameLen);

				quint64 ts = QDateTime::currentMSecsSinceEpoch();
				ds << ts;

				quint32 aacLen = aacWithADTS.size();
				ds << aacLen;
				ds.writeRawData(aacWithADTS.constData(), aacWithADTS.size());

				//m_ws.sendBinaryMessage(packet);
				TaQTWebSocket::sendBinaryMessage(packet);
				// ===== 完成 =====
				av_packet_unref(pkt);
			}
		}
	}

	void initEncoder() {
		// 不再调用 avcodec_register_all()
		// 新版已自动注册
		av_log_set_level(AV_LOG_INFO);
		const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
		codecCtx = avcodec_alloc_context3(codec);
		codecCtx->bit_rate = 128000;
		codecCtx->sample_rate = 44100;
		codecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
		codecCtx->channels = 2;
		codecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;

		if (avcodec_open2(codecCtx, codec, NULL) < 0) qFatal("AAC encoder open fail");

		frame = av_frame_alloc();
		frame->nb_samples = codecCtx->frame_size;
		frame->format = codecCtx->sample_fmt;
		frame->channel_layout = codecCtx->channel_layout;
		av_frame_get_buffer(frame, 0);

		pkt = av_packet_alloc();

		swrCtx = swr_alloc_set_opts(NULL,
			codecCtx->channel_layout, codecCtx->sample_fmt, codecCtx->sample_rate,
			codecCtx->channel_layout, AV_SAMPLE_FMT_S16, codecCtx->sample_rate,
			0, NULL);
		swr_init(swrCtx);
	}

private slots:
	void onBtnTalkClicked() {
		qDebug() << "按钮被点击了!";
		if (false == m_isBeginTalk)
		{
			start();
			btnTalk->setText("停止对讲");
			m_isBeginTalk = true;
		}
		else
		{
			stop();
			btnTalk->setText("开始对讲");
			m_isBeginTalk = false;
		}
	}

	void onReadyRead() {
		QByteArray data = inputDevice->readAll();
		pcmBuffer.append(data);

		int bytesPerSample = 2; // S16LE 每样本2字节
		int samplesPerFrame = frame->nb_samples; // AAC LC固定为1024
		int bytesPerFrame = samplesPerFrame * codecCtx->channels * bytesPerSample;

		while (pcmBuffer.size() >= bytesPerFrame) {
			QByteArray oneFrame = pcmBuffer.left(bytesPerFrame);
			pcmBuffer.remove(0, bytesPerFrame);
			encodeAndSend(oneFrame, 1); // flag=1 表示中间帧
		}

		//QByteArray pcm = inputDevice->readAll();
		//encodeAndSend(pcm, 1);

		//// ===== 计算音量幅度 =====
		//const int16_t* samples = reinterpret_cast<const int16_t*>(pcm.constData());
		//int sampleCount = pcm.size() / 2; // 16 bit 每样本2字节
		//double sumSquares = 0;
		//for (int i = 0; i < sampleCount; ++i) {
		//	sumSquares += samples[i] * samples[i];
		//}
		//double rms = sqrt(sumSquares / qMax(sampleCount, 1));
		//double normalized = rms / 32768.0;  // 归一化到 0-1

		//// 转百分比 (0–100)，加入限制，防止跳动过猛
		//int volume = int(normalized * 100);
		//volume = qBound(0, volume, 100);

		//// 更新音量条
		//m_volumeBar->setValue(volume);
	}

private:
	CustomListDialog* customListDlg = NULL;
	QLabel* m_lblClass = NULL;
	QString m_groupName;
	QString m_unique_group_id;
	TAHttpHandler* m_taHttpHandler = NULL;
	ChatDialog* m_chatDlg = NULL;
	TaQTWebSocket* m_pWs = NULL;
	bool m_iGroupOwner = false;
	QAudioInput* audioInput = nullptr;
	QIODevice* inputDevice = nullptr;
	AVCodecContext* codecCtx = nullptr;
	AVFrame* frame = nullptr;
	AVPacket* pkt = nullptr;
	SwrContext* swrCtx = nullptr;
	QString m_userId;
	QString m_userName;
	QPushButton* btnTalk = NULL;
	bool m_isBeginTalk = false;
	QGroupInfo* m_groupInfo;
	TAHttpHandler* m_httpHandler = NULL;
	// 用于记录按下开始时间
	qint64 pressStartMs = 0;
	//QProgressBar* m_volumeBar = nullptr;
	//QFile localRecordFile;
	//bool isLocalRecording = false;
	QByteArray pcmBuffer;        // 缓冲未编码的PCM数据
	QVector<GroupMemberInfo>  m_groupMemberInfo;
	QTableWidget* seatTable = nullptr; // 座位表格
	ArrangeSeatDialog* arrangeSeatDlg = nullptr; // 排座对话框
};
