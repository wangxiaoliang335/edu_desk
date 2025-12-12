#include "HomeworkEditDialog.h"
#include <QDateTime>
#include <QMessageBox>
#include <QDebug>
#include <QDate>

HomeworkEditDialog::HomeworkEditDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setStyleSheet("background-color: #2b2b2b; color: white;");
    resize(500, 600);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 顶部栏：关闭按钮
    QHBoxLayout* topLayout = new QHBoxLayout;
    topLayout->addStretch();
    
    QPushButton* btnCloseTop = new QPushButton("✕");
    btnCloseTop->setFixedSize(30, 30);
    btnCloseTop->setStyleSheet(
        "QPushButton {"
        "background-color: #666666;"
        "color: white;"
        "font-size: 18px;"
        "font-weight: bold;"
        "border: none;"
        "border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "background-color: #777777;"
        "}"
    );
    connect(btnCloseTop, &QPushButton::clicked, this, &HomeworkEditDialog::onCloseClicked);
    topLayout->addWidget(btnCloseTop);
    mainLayout->addLayout(topLayout);

    // 标题："家庭作业"
    QLabel* titleLabel = new QLabel("家庭作业");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 20px;"
        "font-weight: bold;"
        "color: white;"
        "padding: 10px;"
    );
    mainLayout->addWidget(titleLabel);

    // 日期显示
    dateEdit = new QDateEdit(QDate::currentDate());
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    dateEdit->setStyleSheet(
        "QDateEdit {"
        "background-color: #3b3b3b;"
        "color: white;"
        "border: 1px solid #555;"
        "padding: 8px;"
        "border-radius: 4px;"
        "font-size: 14px;"
        "}"
        "QDateEdit::drop-down {"
        "border: none;"
        "}"
    );
    mainLayout->addWidget(dateEdit);

    // 科目作业输入区域
    QVBoxLayout* contentLayout = new QVBoxLayout;
    contentLayout->setSpacing(10);

    // 语文
    QLabel* labelChinese = new QLabel("语文:");
    labelChinese->setStyleSheet("font-size: 14px; color: white;");
    contentLayout->addWidget(labelChinese);
    
    QTextEdit* editChinese = new QTextEdit;
    editChinese->setPlaceholderText("请输入语文作业内容...");
    editChinese->setStyleSheet(
        "QTextEdit {"
        "background-color: #3b3b3b;"
        "color: white;"
        "border: 1px solid #555;"
        "border-radius: 4px;"
        "padding: 8px;"
        "font-size: 14px;"
        "min-height: 80px;"
        "}"
    );
    subjectEdits["语文"] = editChinese;
    contentLayout->addWidget(editChinese);

    // 数学
    QLabel* labelMath = new QLabel("数学:");
    labelMath->setStyleSheet("font-size: 14px; color: white;");
    contentLayout->addWidget(labelMath);
    
    QTextEdit* editMath = new QTextEdit;
    editMath->setPlaceholderText("请输入数学作业内容...");
    editMath->setStyleSheet(
        "QTextEdit {"
        "background-color: #3b3b3b;"
        "color: white;"
        "border: 1px solid #555;"
        "border-radius: 4px;"
        "padding: 8px;"
        "font-size: 14px;"
        "min-height: 80px;"
        "}"
    );
    subjectEdits["数学"] = editMath;
    contentLayout->addWidget(editMath);

    // 英语
    QLabel* labelEnglish = new QLabel("英语:");
    labelEnglish->setStyleSheet("font-size: 14px; color: white;");
    contentLayout->addWidget(labelEnglish);
    
    QTextEdit* editEnglish = new QTextEdit;
    editEnglish->setPlaceholderText("请输入英语作业内容...");
    editEnglish->setStyleSheet(
        "QTextEdit {"
        "background-color: #3b3b3b;"
        "color: white;"
        "border: 1px solid #555;"
        "border-radius: 4px;"
        "padding: 8px;"
        "font-size: 14px;"
        "min-height: 80px;"
        "}"
    );
    subjectEdits["英语"] = editEnglish;
    contentLayout->addWidget(editEnglish);

    mainLayout->addLayout(contentLayout);
    mainLayout->addStretch();

    // 发布按钮
    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    
    btnPublish = new QPushButton("发布");
    btnPublish->setStyleSheet(
        "QPushButton {"
        "background-color: #0066cc;"
        "color: white;"
        "font-size: 16px;"
        "padding: 10px 30px;"
        "border: none;"
        "border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "background-color: #0052a3;"
        "}"
    );
    btnPublish->setFixedHeight(40);
    connect(btnPublish, &QPushButton::clicked, this, &HomeworkEditDialog::onPublishClicked);
    btnLayout->addWidget(btnPublish);
    mainLayout->addLayout(btnLayout);
}

void HomeworkEditDialog::setDate(const QDate& date)
{
    if (dateEdit) {
        dateEdit->setDate(date);
    }
}

void HomeworkEditDialog::setHomeworkContent(const QMap<QString, QString>& content)
{
    for (auto it = content.begin(); it != content.end(); ++it) {
        if (subjectEdits.contains(it.key())) {
            subjectEdits[it.key()]->setPlainText(it.value());
        }
    }
}

void HomeworkEditDialog::onPublishClicked()
{
    QMap<QString, QString> homeworkContent;
    for (auto it = subjectEdits.begin(); it != subjectEdits.end(); ++it) {
        QString text = it.value()->toPlainText().trimmed();
        if (!text.isEmpty()) {
            homeworkContent[it.key()] = text;
        }
    }
    
    if (homeworkContent.isEmpty()) {
        QMessageBox::warning(this, "提示", "请至少输入一个科目的作业内容！");
        return;
    }
    
    emit homeworkPublished(dateEdit->date(), homeworkContent);
    accept();
}

void HomeworkEditDialog::onCloseClicked()
{
    reject();
}

