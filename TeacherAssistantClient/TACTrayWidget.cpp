#pragma execution_character_set("utf-8")
#include "TACTrayWidget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include "common.h"
#include "../Common/CommonInfo.h"
TACTrayWidget::TACTrayWidget(QWidget *parent)
	: TAFloatingWidget(parent)
{
	this->setObjectName("TACTrayWidget");

	this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
	this->setBorderColor(WIDGET_BORDER_COLOR);
	this->setBorderWidth(WIDGET_BORDER_WIDTH);
	this->setRadius(15);
	this->visibleCloseButton(false);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(10, 10, 10, 10);
	layout->setSpacing(5);

	QLabel* label = new QLabel("", this);
	label->setAlignment(Qt::AlignCenter);
	layout->addWidget(label);

	m_fileManagerButton = new QPushButton(QString::fromUtf8(u8"配置学校信息"), this);
	m_fileManagerButton->setCheckable(true);
	m_fileManagerButton->setEnabled(false); // 默认禁用，等待用户信息加载后更新
	layout->addWidget(m_fileManagerButton);
	
	// ����������Lambda����Ϊ����¼�
	connect(m_fileManagerButton, &QPushButton::toggled, this, [=](bool checked) {
		emit navChoolInfo(checked);
		});

	QPushButton* createFolderButton = new QPushButton("桌面组件", this);
	layout->addWidget(createFolderButton);
	createFolderButton->setCheckable(true);

	// ����������Lambda����Ϊ����¼�
	connect(createFolderButton, &QPushButton::toggled, this, [=](bool checked) {
		emit navType(checked);
	});

	//QPushButton* countdowButton = new QPushButton("�Խ�", this);
	//layout->addWidget(countdowButton);

	//QPushButton* timeButton = new QPushButton("��Ϣ", this);
	//layout->addWidget(timeButton);

	setLayout(layout);
	resize(230, 150);

}

TACTrayWidget::~TACTrayWidget()
{}
void TACTrayWidget::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	initShow();
}

void TACTrayWidget::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	initShow();
}
void TACTrayWidget::initShow()
{
}

void TACTrayWidget::updateAdminButtonState()
{
	if (!m_fileManagerButton) {
		return;
	}
	
	// 根据管理员字段控制按钮启用状态
	UserInfo userInfo = CommonInfo::GetData();
	bool isAdmin = (userInfo.strIsAdministrator == "1" ||
		userInfo.strIsAdministrator.compare(QString::fromUtf8(u8"是"), Qt::CaseInsensitive) == 0 ||
		userInfo.strIsAdministrator.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0);
	m_fileManagerButton->setEnabled(isAdmin);
}