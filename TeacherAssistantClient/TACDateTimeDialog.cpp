#pragma execution_character_set("utf-8")
#include <QDateTime>
#include "TACDateTimeDialog.h"
#include "TACTypeButton.h"
#include "common.h"
TACDateTimeDialog::TACDateTimeDialog(QWidget *parent)
	: TADialog(parent)
{
    this->setObjectName("TACDateTimeDialog");
    typeLayout = new QHBoxLayout();
    typeLayout->setContentsMargins(20, 10, 10, 20);
    typeLayout->setSpacing(20);
    TACTypeButton* dateButton = new TACTypeButton(this);
    dateButton->setCheckable(true);
    dateButton->setText("日期");
    dateButton->setCornerIcon(QIcon(":/res/img/type-button-checked.png"));
    TACTypeButton* timeButton = new TACTypeButton(this);
    timeButton->setCheckable(true);
    timeButton->setText("时间");
    timeButton->setCornerIcon(QIcon(":/res/img/type-button-checked.png"));
    typeLayout->addWidget(dateButton);
    typeLayout->addWidget(timeButton);
    this->contentLayout->addLayout(typeLayout);
    connect(dateButton, &QPushButton::toggled, this, [=](bool checked) {
        int type = (static_cast<int>(dateButton->isChecked()) << 1) | static_cast<int>(timeButton->isChecked());
        emit updateType(type);
    });
    connect(timeButton, &QPushButton::toggled, this, [=](bool checked) {
        int type = (static_cast<int>(dateButton->isChecked()) << 1) | static_cast<int>(timeButton->isChecked());
        emit updateType(type);
    });

    contentWidget = new QWidget(this);
    contentWidget->setObjectName("contentWidget");
    upContentLabel = new QLabel(contentWidget);
    upContentLabel->setAlignment(Qt::AlignCenter);
    upContentLabel->setObjectName("upContentLabel");
    downContentLabel = new QLabel(contentWidget);
    downContentLabel->setAlignment(Qt::AlignCenter);
    downContentLabel->setObjectName("downContentLabel");
    QVBoxLayout* vLayout = new QVBoxLayout(contentWidget);
    vLayout->setAlignment(Qt::AlignCenter);
    vLayout->addWidget(upContentLabel);
    vLayout->addWidget(downContentLabel);
    contentWidget->setLayout(vLayout);
    contentWidget->setFixedSize(QSize(460, 140));
    this->contentLayout->addWidget(contentWidget);
    this->contentLayout->addStretch();
    this->contentLayout->setAlignment(Qt::AlignCenter);
    
    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(40);
    this->resize(520, 415);
    this->setTitle("选择时间模式");

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [=]() {
        if (dateButton->isChecked() && timeButton->isChecked())
        {
            upContentLabel->show();
            upContentLabel->setText(QTime::currentTime().toString("hh:mm"));
            downContentLabel->show();
            downContentLabel->setText(QDate::currentDate().toString("MM月dd日"));
        }
        else if (dateButton->isChecked())
        {
            upContentLabel->show();
            upContentLabel->setText(QDate::currentDate().toString("MM月dd日"));
            downContentLabel->hide();
        }
        else
        {
            upContentLabel->show();
            upContentLabel->setText(QTime::currentTime().toString("hh:mm"));
            downContentLabel->hide();
        }
    });
    timer->start(100);
    
    // 连接确定/取消按钮信号
    connect(this, &TADialog::cancelClicked, this, [=]() {
        this->close();
    });
    connect(this, &TADialog::enterClicked, this, [=]() {
        // 确定时关闭对话框，类型已经通过 updateType 信号实时更新了
        this->close();
    });
}

TACDateTimeDialog::~TACDateTimeDialog()
{}

