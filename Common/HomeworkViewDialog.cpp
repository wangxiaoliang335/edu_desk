#include "HomeworkViewDialog.h"
#include <QDateTime>
#include <QDebug>
#include <QDate>
#include <QLocale>

HomeworkViewDialog::HomeworkViewDialog(QWidget* parent)
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
    connect(btnCloseTop, &QPushButton::clicked, this, &HomeworkViewDialog::onCloseClicked);
    topLayout->addWidget(btnCloseTop);
    mainLayout->addLayout(topLayout);

    // 日期标题
    dateLabel = new QLabel;
    dateLabel->setAlignment(Qt::AlignCenter);
    dateLabel->setStyleSheet(
        "font-size: 18px;"
        "font-weight: bold;"
        "color: white;"
        "padding: 10px;"
    );
    mainLayout->addWidget(dateLabel);

    // 科目作业显示区域
    QVBoxLayout* contentLayout = new QVBoxLayout;
    contentLayout->setSpacing(15);

    // 语文
    QLabel* labelChinese = new QLabel("语文:");
    labelChinese->setStyleSheet("font-size: 14px; color: white; font-weight: bold;");
    contentLayout->addWidget(labelChinese);
    
    QLabel* contentChinese = new QLabel;
    contentChinese->setWordWrap(true);
    contentChinese->setStyleSheet(
        "QLabel {"
        "background-color: #3b3b3b;"
        "color: white;"
        "border: 1px solid #555;"
        "border-radius: 4px;"
        "padding: 10px;"
        "font-size: 14px;"
        "min-height: 60px;"
        "}"
    );
    subjectLabels["语文"] = contentChinese;
    contentLayout->addWidget(contentChinese);

    // 数学
    QLabel* labelMath = new QLabel("数学:");
    labelMath->setStyleSheet("font-size: 14px; color: white; font-weight: bold;");
    contentLayout->addWidget(labelMath);
    
    QLabel* contentMath = new QLabel;
    contentMath->setWordWrap(true);
    contentMath->setStyleSheet(
        "QLabel {"
        "background-color: #3b3b3b;"
        "color: white;"
        "border: 1px solid #555;"
        "border-radius: 4px;"
        "padding: 10px;"
        "font-size: 14px;"
        "min-height: 60px;"
        "}"
    );
    subjectLabels["数学"] = contentMath;
    contentLayout->addWidget(contentMath);

    // 英语
    QLabel* labelEnglish = new QLabel("英语:");
    labelEnglish->setStyleSheet("font-size: 14px; color: white; font-weight: bold;");
    contentLayout->addWidget(labelEnglish);
    
    QLabel* contentEnglish = new QLabel;
    contentEnglish->setWordWrap(true);
    contentEnglish->setStyleSheet(
        "QLabel {"
        "background-color: #3b3b3b;"
        "color: white;"
        "border: 1px solid #555;"
        "border-radius: 4px;"
        "padding: 10px;"
        "font-size: 14px;"
        "min-height: 60px;"
        "}"
    );
    subjectLabels["英语"] = contentEnglish;
    contentLayout->addWidget(contentEnglish);

    mainLayout->addLayout(contentLayout);
    mainLayout->addStretch();
}

void HomeworkViewDialog::setDate(const QDate& date)
{
    if (dateLabel) {
        QLocale locale(QLocale::Chinese);
        QString weekDay = locale.toString(date, "dddd");
        QString dateStr = QString("%1 %2 家庭作业").arg(date.toString("yyyy年MM月dd日")).arg(weekDay);
        dateLabel->setText(dateStr);
    }
}

void HomeworkViewDialog::setHomeworkContent(const QMap<QString, QString>& content)
{
    for (auto it = content.begin(); it != content.end(); ++it) {
        if (subjectLabels.contains(it.key())) {
            subjectLabels[it.key()]->setText(it.value());
        }
    }
    
    // 如果某个科目没有作业，显示提示
    QStringList subjects = QStringList() << "语文" << "数学" << "英语";
    for (const QString& subject : subjects) {
        if (subjectLabels.contains(subject)) {
            if (!content.contains(subject) || content[subject].isEmpty()) {
                subjectLabels[subject]->setText("（暂无作业）");
                subjectLabels[subject]->setStyleSheet(
                    "QLabel {"
                    "background-color: #3b3b3b;"
                    "color: #888;"
                    "border: 1px solid #555;"
                    "border-radius: 4px;"
                    "padding: 10px;"
                    "font-size: 14px;"
                    "min-height: 60px;"
                    "font-style: italic;"
                    "}"
                );
            }
        }
    }
}

void HomeworkViewDialog::onCloseClicked()
{
    reject();
}

