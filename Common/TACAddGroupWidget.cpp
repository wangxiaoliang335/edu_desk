#pragma execution_character_set("utf-8")
#include "TACAddGroupWidget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
//#include "common.h"

#define WIDGET_BACKGROUND_COLOR_C QColor(50,50,50,200)
#define WIDGET_BORDER_COLOR_C QColor(255,255,255,25)
#define WIDGET_BORDER_WIDTH_C 1

TACAddGroupWidget::TACAddGroupWidget(QWidget *parent, TaQTWebSocket* pWs)
	: TAFloatingWidget(parent)
{
	m_searchDlg = new SearchDialog(this);
	// 连接SearchDialog的加入群组成功信号，转发给FriendGroupDialog
	connect(m_searchDlg, &SearchDialog::groupJoined, this, &TACAddGroupWidget::groupJoined);
	m_classTeacherDlg = new ClassTeacherDialog(this, pWs);
	this->setObjectName("TACDesktopManagerWidget");

	this->setBackgroundColor(WIDGET_BACKGROUND_COLOR_C);
	this->setBorderColor(WIDGET_BORDER_COLOR_C);
	this->setBorderWidth(WIDGET_BORDER_WIDTH_C);
	this->setRadius(15);
	this->visibleCloseButton(false);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(10, 10, 10, 10);
	layout->setSpacing(5);

	QLabel* label = new QLabel("", this);
	label->setAlignment(Qt::AlignCenter);
	layout->addWidget(label);

	QPushButton* fileManagerButton = new QPushButton("创建班级群", this);
	layout->addWidget(fileManagerButton);
	connect(fileManagerButton, &QPushButton::clicked, this, [=]() {
		if (m_classTeacherDlg && m_classTeacherDlg->isHidden())
		{
			m_classTeacherDlg->show();
		}
		});

	QPushButton* createFolderButton = new QPushButton("创建普通群", this);
	layout->addWidget(createFolderButton);

	QPushButton* countdowButton = new QPushButton("加好友/群", this);
	layout->addWidget(countdowButton);

	connect(countdowButton, &QPushButton::clicked, this, [=]() {
		if (m_searchDlg && m_searchDlg->isHidden())
		{
			m_searchDlg->show();
		}
	});

	/*QPushButton* timeButton = new QPushButton("时间对话框", this);
	layout->addWidget(timeButton);

	QPushButton* classButton = new QPushButton("学校/班级", this);
	layout->addWidget(classButton);

	QPushButton* wallpaperButton = new QPushButton("壁纸", this);
	layout->addWidget(wallpaperButton);

	QPushButton* courseScheduleButton = new QPushButton("教师课程表", this);
	layout->addWidget(courseScheduleButton);

	QPushButton* tabelButton = new QPushButton("表格对话框", this);
	layout->addWidget(tabelButton);

	QPushButton* textButton = new QPushButton("文本对话框", this);
	layout->addWidget(textButton);

	QPushButton* imageButton = new QPushButton("图片对话框", this);
	layout->addWidget(imageButton);*/

	setLayout(layout);
	resize(230, 380);

}

TACAddGroupWidget::~TACAddGroupWidget()
{}

void TACAddGroupWidget::InitWebSocket()
{
	if (m_classTeacherDlg)
	{
		m_classTeacherDlg->InitWebSocket();
	}
}

void TACAddGroupWidget::InitData()
{
	if (m_classTeacherDlg)
	{
		m_classTeacherDlg->InitData();
	}
}

QVector<QString> TACAddGroupWidget::getNoticeMsg()
{
	if (m_classTeacherDlg)
	{
		return m_classTeacherDlg->getNoticeMsg();
	}
	return QVector<QString>();
}

void TACAddGroupWidget::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	initShow();
}

void TACAddGroupWidget::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	initShow();
}
void TACAddGroupWidget::initShow()
{
}