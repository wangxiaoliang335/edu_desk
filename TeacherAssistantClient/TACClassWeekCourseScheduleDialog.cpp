#pragma execution_character_set("utf-8")
#include "TACClassWeekCourseScheduleDialog.h"
#include "common.h"
TACClassWeekCourseScheduleDialog::TACClassWeekCourseScheduleDialog(QWidget *parent)
	: TABaseDialog(parent)
{
    this->setObjectName("TACClassWeekCourseScheduleDialog");
    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(40);
    this->setFixedSize(QSize(550, 730));
    this->setTitle("教室周课程表");


	TACToolWidget* toolWidget = new TACToolWidget(this);
    toolWidget->setObjectName("toolWidget");
	this->contentLayout->addWidget(toolWidget);

	QWidget* container = new QWidget(this);
    container->setObjectName("container");
	QVBoxLayout* containerLayout = new QVBoxLayout();
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);
    container->setLayout(containerLayout);

    QStringList days = { "周一", "周二", "周三", "周四", "周五" };
    QWidget* labelWidget = new QWidget(this);
    labelWidget->setObjectName("labelWidget");
    QHBoxLayout* labelLayout = new QHBoxLayout(labelWidget);
    labelLayout->setSpacing(0);
    labelLayout->setContentsMargins(100, 20, 20, 20);
    for (int col = 0; col < 5; ++col) {
        QLabel* label = new QLabel(days[col]);
        label->setAlignment(Qt::AlignCenter);
        labelLayout->addWidget(label);
    }
    containerLayout->addWidget(labelWidget);

    QHBoxLayout* hlayout = new QHBoxLayout();
    hlayout->setSpacing(0);
    hlayout->setContentsMargins(0, 0, 0, 0);

	QVBoxLayout* vlayout = new QVBoxLayout();
	QLabel* morningLabel = new QLabel("上<br>午", this);
    morningLabel->setObjectName("morningLabel");
    morningLabel->setAlignment(Qt::AlignCenter);
	vlayout->addWidget(morningLabel);
	QLabel* afternoonLabel = new QLabel("下<br>午", this);
    afternoonLabel->setObjectName("afternoonLabel");
    afternoonLabel->setAlignment(Qt::AlignCenter);
	vlayout->addWidget(afternoonLabel);
    vlayout->setSpacing(0);
    vlayout->setContentsMargins(0, 0, 0, 0);

	hlayout->addLayout(vlayout);

    QWidget* gridWidget = new QWidget(this);
    gridWidget->setObjectName("gridWidget");
    gridLayout = new QGridLayout(gridWidget);
    gridLayout->setSpacing(1);
    gridLayout->getContentsMargins(0, 0, 0, 0);
    hlayout->addWidget(gridWidget);

    containerLayout->addLayout(hlayout);
	this->contentLayout->addWidget(container);
    
    updateClassList();
}

TACClassWeekCourseScheduleDialog::~TACClassWeekCourseScheduleDialog()
{}

void TACClassWeekCourseScheduleDialog::updateClassList()
{
   
    classVecotr.resize(8);
    for (int row = 0; row < 8; ++row) {
        classVecotr[row].resize(5);
        
        for (int col = 0; col < 5; ++col) {
            QPushButton* btn = new QPushButton("课" + QString::number(row + 1));
            btn->setProperty("row", row);
            btn->setProperty("col", col);
            btn->setCheckable(true);
            connect(btn, &QPushButton::clicked, this, &TACClassWeekCourseScheduleDialog::classClick);

            gridLayout->addWidget(btn, row + 1, col + 1);
            classVecotr[row][col] = btn;
        }
    }

}
void TACClassWeekCourseScheduleDialog::init()
{
    

}
void TACClassWeekCourseScheduleDialog::classClick()
{

}
