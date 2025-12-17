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
    this->contentLayout = new QVBoxLayout;
    this->contentLayout->setSpacing(15);
    mainLayout->addLayout(this->contentLayout);
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
    // 先更新/创建本次内容涉及的科目
    for (auto it = content.begin(); it != content.end(); ++it) {
        QLabel* lbl = ensureSubjectLabel(it.key());
        if (lbl) lbl->setText(it.value());
    }

    // 已存在但本次没传的科目置为空提示（避免显示旧内容）
    for (auto it = subjectLabels.begin(); it != subjectLabels.end(); ++it) {
        const QString subject = it.key();
        QLabel* lbl = it.value();
        if (!content.contains(subject) || content.value(subject).trimmed().isEmpty()) {
            lbl->setText(QString::fromUtf8(u8"（暂无作业）"));
            lbl->setStyleSheet(
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

void HomeworkViewDialog::onCloseClicked()
{
    reject();
}

void HomeworkViewDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QDialog::mousePressEvent(event);
}

void HomeworkViewDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
        event->accept();
        return;
    }
    QDialog::mouseMoveEvent(event);
}

void HomeworkViewDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
        return;
    }
    QDialog::mouseReleaseEvent(event);
}

QLabel* HomeworkViewDialog::ensureSubjectLabel(const QString& subject)
{
    const QString s = subject.trimmed();
    if (s.isEmpty()) return nullptr;
    if (subjectLabels.contains(s)) return subjectLabels[s];
    if (!contentLayout) return nullptr;

    QLabel* labelTitle = new QLabel(QString("%1:").arg(s), this);
    labelTitle->setStyleSheet("font-size: 14px; color: white; font-weight: bold;");
    contentLayout->addWidget(labelTitle);

    QLabel* contentLbl = new QLabel(this);
    contentLbl->setWordWrap(true);
    contentLbl->setStyleSheet(
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
    contentLbl->setText(QString::fromUtf8(u8"（暂无作业）"));

    subjectLabels[s] = contentLbl;
    contentLayout->addWidget(contentLbl);
    return contentLbl;
}

