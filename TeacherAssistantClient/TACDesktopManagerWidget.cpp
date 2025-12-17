#pragma execution_character_set("utf-8")
#include "TACDesktopManagerWidget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include "common.h"
#include "TACalendarDialog.h"
#include "TACTeacherCourseScheduleWindow.h"
TACDesktopManagerWidget::TACDesktopManagerWidget(QWidget *parent)
	: TAFloatingWidget(parent)
{
	this->setObjectName("TACDesktopManagerWidget");

	this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
	this->setBorderColor(WIDGET_BORDER_COLOR);
	this->setBorderWidth(WIDGET_BORDER_WIDTH);
	this->setRadius(15);
	this->visibleCloseButton(false);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(10, 10, 10, 10);
	layout->setSpacing(5);

	QLabel* label = new QLabel("桌面管理", this);
	label->setAlignment(Qt::AlignCenter);
	layout->addWidget(label);

	QPushButton* fileManagerButton = new QPushButton("文件管理", this);
	layout->addWidget(fileManagerButton);

	QPushButton* createFolderButton = new QPushButton("创建文件夹", this);
	layout->addWidget(createFolderButton);

	QPushButton* countdowButton = new QPushButton("倒计时对话框", this);
	layout->addWidget(countdowButton);

	QPushButton* timeButton = new QPushButton("时间对话框", this);
	layout->addWidget(timeButton);

	QPushButton* classButton = new QPushButton("学校/班级", this);
	layout->addWidget(classButton);

	QPushButton* wallpaperButton = new QPushButton("壁纸", this);
	layout->addWidget(wallpaperButton);

	QPushButton* courseScheduleButton = new QPushButton("教师课程表", this);
	layout->addWidget(courseScheduleButton);
	connect(courseScheduleButton, &QPushButton::clicked, this, [this]() {
		if (!m_teacherCourseScheduleWindow) {
			// 用独立顶层窗口，方便悬浮显示
			m_teacherCourseScheduleWindow = new TACTeacherCourseScheduleWindow(nullptr);
			m_teacherCourseScheduleWindow->setObjectName(QStringLiteral("TACTeacherCourseScheduleWindow"));
			m_teacherCourseScheduleWindow->setWindowTitle(QString::fromUtf8(u8"我的课表"));
		}
		// 尽量在桌面管理面板右侧弹出
		const QPoint anchor = this->mapToGlobal(QPoint(this->width() + 10, 0));
		m_teacherCourseScheduleWindow->move(anchor);
		m_teacherCourseScheduleWindow->show();
		m_teacherCourseScheduleWindow->raise();
		m_teacherCourseScheduleWindow->activateWindow();
	});

	QPushButton* tabelButton = new QPushButton("表格对话框", this);
	layout->addWidget(tabelButton);

	QPushButton* textButton = new QPushButton("文本对话框", this);
	layout->addWidget(textButton);

	QPushButton* imageButton = new QPushButton("图片对话框", this);
	layout->addWidget(imageButton);

	QPushButton* schoolCalendarButton = new QPushButton(QString::fromUtf8(u8"校历"), this);
	layout->addWidget(schoolCalendarButton);
	connect(schoolCalendarButton, &QPushButton::clicked, this, [this]() {
		if (!m_schoolCalendarWidget) {
			m_schoolCalendarWidget = new TASchoolCalendarWidget(nullptr);
			m_schoolCalendarWidget->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
			m_schoolCalendarWidget->setBorderColor(WIDGET_BORDER_COLOR);
			m_schoolCalendarWidget->setBorderWidth(WIDGET_BORDER_WIDTH);
			m_schoolCalendarWidget->setRadius(15);
			m_schoolCalendarWidget->resize(1100, 820);
		}

		// 尽量在桌面管理面板右侧弹出
		const QPoint anchor = this->mapToGlobal(QPoint(this->width() + 10, 0));
		m_schoolCalendarWidget->move(anchor);
		m_schoolCalendarWidget->show();
		m_schoolCalendarWidget->raise();
		m_schoolCalendarWidget->activateWindow();
		});

	setLayout(layout);
	resize(230, 780);

}

TACDesktopManagerWidget::~TACDesktopManagerWidget()
{}
void TACDesktopManagerWidget::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	initShow();
}

void TACDesktopManagerWidget::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	initShow();
}
void TACDesktopManagerWidget::initShow()
{
}