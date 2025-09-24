#pragma execution_character_set("utf-8")
#include <QScreen>
#include "TACMainDialog.h"

TACMainDialog::TACMainDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	this->setWindowState(Qt::WindowFullScreen);
	this->setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
}

TACMainDialog::~TACMainDialog()
{}
void TACMainDialog::Init()
{
    navBarWidget = new TACNavigationBarWidget(this);
    navBarWidget->visibleCloseButton(false);
    connect(navBarWidget, &TACNavigationBarWidget::navType, this, [=](TACNavigationBarWidgetType type,bool checked) {
        if (type == TACNavigationBarWidgetType::TIMER)
        {
            if (datetimeDialog)
            {
                datetimeDialog->show();
            }
            if (datetimeWidget)
                datetimeWidget->show();
        }
        else if (type == TACNavigationBarWidgetType::FOLDER)
        {
            if (desktopManagerWidget)
            {
                QRect rect = navBarWidget->geometry();
                desktopManagerWidget->move(rect.x(), rect.y() - desktopManagerWidget->height() - 10);
                if (checked)
                    desktopManagerWidget->show();
                else
                    desktopManagerWidget->hide();
            }
        }
        else if (type == TACNavigationBarWidgetType::WALLPAPER)
        {
            if (wallpaperLibraryDialog)
                wallpaperLibraryDialog->show();
        }
        else if (type == TACNavigationBarWidgetType::HOMEWORK)
        {
            if (homeworkDialog)
                homeworkDialog->show();
        }
        else if (type == TACNavigationBarWidgetType::IM)
        {
            if (imDialog)
                imDialog->show();
        }
        else if (type == TACNavigationBarWidgetType::PREPARE_CLASS)
        {
            if (prepareClassDialog)
                prepareClassDialog->show();
        }
        else if (type == TACNavigationBarWidgetType::CALENDAR)
        {
            if (classWeekCourseScheduldDialog)
                classWeekCourseScheduldDialog->show();
        }
    });
    navBarWidget->show();

    prepareClassDialog = new TACPrepareClassDialog(this);
    desktopManagerWidget = new TACDesktopManagerWidget(this);


    imDialog = new TACIMDialog(this);

    homeworkDialog = new TACHomeworkDialog(this);
    homeworkDialog->setContent("语文<br>背诵《荷塘月色》第二段");


    wallpaperLibraryDialog = new TACWallpaperLibraryDialog(this);

    countDownDialog = new TACCountDownDialog(this);
    countDownWidget = new TACCountDownWidget(this);
    connect(countDownWidget, &TACCountDownWidget::doubleClicked, this, [=]() {
        countDownDialog->show();
        });
    countDownWidget->setContent(countDownDialog->content());
    countDownWidget->show();

    datetimeDialog = new TACDateTimeDialog(this);
    connect(datetimeDialog, &TACDateTimeDialog::updateType, this, [=](int type) {
        datetimeWidget->setType(type);
    });

    datetimeWidget = new TACDateTimeWidget(this);
    datetimeWidget->show();

    logoWidget = new TACLogoWidget(this);
    logoWidget->updateLogo("C:\\workspace\\obs\\TeacherAssistant\\TeacherAssistantClient\\res\\img\\qinghua.png");
    logoWidget->show();

    schoolLabelWidget = new TACSchoolLabelWidget(this);
    connect(schoolLabelWidget, &TACSchoolLabelWidget::doubleClicked, this, [=]() {
        logoDialog->show();
    });
    schoolLabelWidget->show();

    classLabelWidget = new TACClassLabelWidget(this);
    connect(classLabelWidget, &TACClassLabelWidget::doubleClicked, this, [=]() {
        logoDialog->show();
    });
    classLabelWidget->show();

    logoDialog = new TACLogoDialog(this);
    connect(logoDialog, &TACLogoDialog::enterClicked, this, [=]() {
        if (schoolLabelWidget && !logoDialog->getSchoolName().isEmpty())
        {
            schoolLabelWidget->setContent(logoDialog->getSchoolName());
        }
        if (classLabelWidget && !logoDialog->getClassName().isEmpty())
        {
            classLabelWidget->setContent(logoDialog->getClassName());
        }
    });
   
    folderDialog = new TACFolderDialog(this);
    folderWidget = new TACFolderWidget(this);
    connect(folderWidget, &TACFolderWidget::clicked, this, [=]() {
        folderDialog->show();
    });
    folderWidget->show();

    courseSchedule = new TACCourseSchedule(this);
    courseSchedule->show();

    QMap<TimeRange, QString> classMap;
    classMap[TimeRange(QTime(8, 0), QTime(8, 45))] = "数学";
    classMap[TimeRange(QTime(8, 45), QTime(9, 30))] = "语文";
    classMap[TimeRange(QTime(9, 30), QTime(10, 15))] = "大课间";
    classMap[TimeRange(QTime(10, 30), QTime(11, 30))] = "英语";
    classMap[TimeRange(QTime(13, 0), QTime(14, 0))] = "午休";
    classMap[TimeRange(QTime(14, 15), QTime(15, 0))] = "历史";

    courseSchedule->updateClass(classMap);

 
    classWeekCourseScheduldDialog = new TACClassWeekCourseScheduleDialog(this);
    //updateBackground("C:/workspace/obs/TeacherAssistant/TeacherAssistantClient/res/bg/5.jpg");

}
void TACMainDialog::updateBackground(const QString & fileName)
{
    m_background = QPixmap(fileName);
    update();
}
void TACMainDialog::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if (m_background.isNull()) {
        return;
    }
    QPixmap scaled = m_background.scaled(
        size(),
        Qt::KeepAspectRatioByExpanding,
        Qt::SmoothTransformation
    );
    QRect r = scaled.rect();
    r.moveCenter(rect().center());
    painter.drawPixmap(r, scaled);
}

void TACMainDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    this->update();
}