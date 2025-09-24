#pragma execution_character_set("utf-8")
#include "TACIMDialog.h"
#include "common.h"
#include <QToolButton>
#include <QLineEdit>
#include <QPixmap>

TACIMDialog::TACIMDialog(QWidget *parent)
	: TABaseDialog(parent)
{
    this->setObjectName("TACIMDialog");
    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(40);
    this->setFixedSize(QSize(820, 580));
    this->setTitle("对讲");
    this->titleLabel->setAlignment(Qt::AlignCenter);

    QScrollArea* usersScrollArea = new QScrollArea(this);
    usersScrollArea->setFixedHeight(100);
    usersScrollArea->setObjectName("usersScrollArea");
    usersScrollArea->setWidgetResizable(true);
    usersScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    usersScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QWidget* teacherListContainer = new QWidget();
    teacherListLayout = new QHBoxLayout(teacherListContainer);
    teacherListLayout->setSpacing(10);
    teacherListLayout->setContentsMargins(10, 10, 10, 10);
    this->contentLayout->addWidget(usersScrollArea);


    QScrollArea* messageScrollArea = new QScrollArea(this);
    messageScrollArea->setObjectName("messageScrollArea");
    messageScrollArea->setWidgetResizable(true);
    messageScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    messageScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QWidget* messageContainer = new QWidget();
    messageLayout = new QVBoxLayout(messageContainer);
    messageLayout->setSpacing(10);
    messageLayout->setContentsMargins(10, 10, 10, 10);

    messageScrollArea->setWidget(messageContainer);
    this->contentLayout->addWidget(messageScrollArea);

    QHBoxLayout* toolsLayout = new QHBoxLayout();
    toolsLayout->setSpacing(10);
    toolsLayout->setContentsMargins(10, 10, 10, 10);
    QPushButton* voiceButton = new QPushButton("按住对讲", this);
    QLineEdit* messageEdit = new QLineEdit(this);
    messageEdit->setVisible(false);

    QPushButton* keyButton = new QPushButton(this);
    keyButton->setObjectName("keyButton");
    keyButton->setCheckable(true);
    //keyButton->setIcon(QIcon());
    keyButton->setFixedSize(60, 60);
    connect(keyButton, &QPushButton::toggled, this, [=](bool checked) {
        messageEdit->setVisible(checked);
        voiceButton->setVisible(!checked);
    });

    toolsLayout->addWidget(voiceButton);
    toolsLayout->addWidget(messageEdit);
    toolsLayout->addWidget(keyButton);
    this->contentLayout->addLayout(toolsLayout);

}

TACIMDialog::~TACIMDialog()
{}

void TACIMDialog::updateMessage(const QList<TATeacher*> &teacherList,QList<TAIMMessage*> &messageList)
{
    this->clearLayout(this->teacherListLayout);
    this->clearLayout(this->messageLayout);
    for (auto& it : teacherList)
    {
        QToolButton* toolButton = new QToolButton(this);
        toolButton->setText(it->name());
        toolButton->setIcon(QIcon(it->icon()));
        toolButton->setIconSize(QSize(25,25));
        toolButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        connect(toolButton, &QToolButton::clicked, this, [=]() {});
        this->teacherListLayout->addWidget(toolButton);
    }
    for (auto& it : messageList)
    {
        TACIMMessageItemWidget* item = new TACIMMessageItemWidget(it, this);
        this->messageLayout->addWidget(item);
    }
}

TACIMMessageItemWidget::TACIMMessageItemWidget(TAIMMessage* message, QWidget* parent):QWidget(parent)
{
    QVBoxLayout* container = new QVBoxLayout();
    QHBoxLayout* teacherLayout = new QHBoxLayout();
    QLabel* iconLabel = new QLabel(this);
    iconLabel->setFixedSize(QSize(30,30));
    QPixmap pixmap(message->teacher()->icon());
    iconLabel->setPixmap(pixmap.scaled(iconLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QLabel* nameLabel = new QLabel(message->teacher()->name(),this);

    teacherLayout->addWidget(iconLabel);
    teacherLayout->addWidget(nameLabel);
    container->addLayout(teacherLayout);

    QLineEdit* messageLineEdit = new QLineEdit(this);
    messageLineEdit->setText(message->content());

    container->addWidget(messageLineEdit);
    this->setLayout(container);
}

TACIMMessageItemWidget::~TACIMMessageItemWidget()
{
}
