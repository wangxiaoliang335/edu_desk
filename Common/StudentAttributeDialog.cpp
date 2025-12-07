#include "StudentAttributeDialog.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QDebug>
#include <QMap>
#include <QApplication>

StudentAttributeDialog::StudentAttributeDialog(QWidget* parent)
    : QDialog(parent)
    , m_selectedAttribute("")
    , m_currentEditingAttribute("")
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setStyleSheet("background-color: #808080;"); // 灰色背景
    resize(400, 500);
    
    // 启用鼠标跟踪以检测鼠标进入/离开
    setMouseTracking(true);

    // 创建关闭按钮
    m_btnClose = new QPushButton("X", this);
    m_btnClose->setFixedSize(30, 30);
    m_btnClose->setStyleSheet(
        "QPushButton { background-color: orange; color: white; font-weight:bold; font-size: 14px; border: 1px solid #555; border-radius: 4px; }"
        "QPushButton:hover { background-color: #cc6600; }"
    );
    m_btnClose->hide(); // 初始隐藏
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::close);
    
    // 为关闭按钮安装事件过滤器，确保鼠标在按钮上时不会隐藏
    m_btnClose->installEventFilter(this);

    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(20, 40, 20, 20); // 增加顶部边距，为关闭按钮留出空间
    outerLayout->setSpacing(15);

    // 标题："学生统计信息"
    m_titleLabel = new QLabel("学生统计信息");
    m_titleLabel->setStyleSheet(
        "background-color: #d0d0d0;"
        "color: black;"
        "font-size: 14px;"
        "padding: 8px;"
        "border-radius: 4px;"
    );
    m_titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    outerLayout->addWidget(m_titleLabel);

    // 主内容区域
    m_mainLayout = new QVBoxLayout;
    m_mainLayout->setSpacing(10);
    outerLayout->addLayout(m_mainLayout);

    outerLayout->addStretch();
}

void StudentAttributeDialog::setStudentInfo(const struct StudentInfo& student)
{
    m_student = student;
    
    // 清空现有内容
    QLayoutItem* item;
    while ((item = m_mainLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    m_attributeValueButtons.clear();
    m_annotationButtons.clear();
    m_valueEdits.clear();
    
    // 如果没有可用属性列表，使用默认属性
    if (m_availableAttributes.isEmpty()) {
        m_availableAttributes = QStringList() << "背诵" << "语文" << "数学" << "英语";
    }
    
    // 为每个属性创建一行
    for (const QString& attrName : m_availableAttributes) {
        // 获取属性值（如果不存在，默认为0）
        double value = m_student.attributes.value(attrName, 0.0);
        if (value == 0.0 && !m_student.attributes.contains(attrName)) {
            // 如果属性不存在，尝试从score获取（用于兼容旧数据）
            if (attrName == "总分" || attrName == "数学") {
                value = m_student.score;
            }
        }
        
        updateAttributeRow(attrName, value, false);
    }
}

void StudentAttributeDialog::setAvailableAttributes(const QList<QString>& attributes)
{
    m_availableAttributes = attributes;
}

void StudentAttributeDialog::updateAttributeRow(const QString& attributeName, double value, bool isHighlighted)
{
    QHBoxLayout* rowLayout = new QHBoxLayout;
    rowLayout->setSpacing(10);
    
    // 左侧绿色小方块（带"-"）
    QPushButton* leftSquare = new QPushButton("-");
    leftSquare->setFixedSize(30, 30);
    leftSquare->setStyleSheet(
        "QPushButton {"
        "background-color: green;"
        "color: white;"
        "border: none;"
        "border-radius: 4px;"
        "font-size: 16px;"
        "font-weight: bold;"
        "}"
    );
    leftSquare->setEnabled(false);
    rowLayout->addWidget(leftSquare);
    
    // 属性标签（浅蓝色按钮）
    QPushButton* attrLabel = new QPushButton(attributeName);
    attrLabel->setStyleSheet(
        "QPushButton {"
        "background-color: #87ceeb;"
        "color: black;"
        "font-size: 14px;"
        "padding: 8px 16px;"
        "border: none;"
        "border-radius: 4px;"
        "text-align: left;"
        "}"
        "QPushButton:hover {"
        "background-color: #6bb6ff;"
        "}"
    );
    attrLabel->setFixedHeight(35);
    connect(attrLabel, &QPushButton::clicked, this, [this, attributeName, attrLabel]() {
        // 点击属性标签时，选中该行
        clearAllHighlights();
        m_selectedAttribute = attributeName;
        // 重新创建该行以显示高亮
        double value = m_student.attributes.value(attributeName, 0.0);
        if (value == 0.0 && !m_student.attributes.contains(attributeName)) {
            if (attributeName == "总分" || attributeName == "数学") {
                value = m_student.score;
            }
        }
        // 找到该行并高亮
        if (m_attributeValueButtons.contains(attributeName)) {
            QPushButton* valueBtn = m_attributeValueButtons[attributeName];
            valueBtn->setStyleSheet(
                "QPushButton {"
                "background-color: green;"
                "color: white;"
                "font-size: 14px;"
                "padding: 8px 16px;"
                "border: 3px solid yellow;"
                "border-radius: 4px;"
                "}"
            );
        }
        if (attrLabel) {
            attrLabel->setStyleSheet(
                "QPushButton {"
                "background-color: #87ceeb;"
                "color: black;"
                "font-size: 14px;"
                "padding: 8px 16px;"
                "border: 3px solid yellow;"
                "border-radius: 4px;"
                "text-align: left;"
                "}"
            );
        }
    });
    rowLayout->addWidget(attrLabel);
    
    // 属性值（绿色方块，可点击编辑）
    QPushButton* valueBtn = new QPushButton(QString::number(value));
    valueBtn->setStyleSheet(
        "QPushButton {"
        "background-color: green;"
        "color: white;"
        "font-size: 14px;"
        "padding: 8px 16px;"
        "border: none;"
        "border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "background-color: #00cc00;"
        "}"
    );
    valueBtn->setFixedHeight(35);
    valueBtn->setProperty("attributeName", attributeName);
    connect(valueBtn, &QPushButton::clicked, this, &StudentAttributeDialog::onAttributeValueClicked);
    m_attributeValueButtons[attributeName] = valueBtn;
    rowLayout->addWidget(valueBtn);
    
    // 注释按钮（绿色方块）
    QPushButton* annotationBtn = new QPushButton("注释");
    annotationBtn->setStyleSheet(
        "QPushButton {"
        "background-color: green;"
        "color: white;"
        "font-size: 14px;"
        "padding: 8px 16px;"
        "border: none;"
        "border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "background-color: #00cc00;"
        "}"
    );
    annotationBtn->setFixedHeight(35);
    annotationBtn->setProperty("attributeName", attributeName);
    connect(annotationBtn, &QPushButton::clicked, this, &StudentAttributeDialog::onAnnotationClicked);
    m_annotationButtons[attributeName] = annotationBtn;
    rowLayout->addWidget(annotationBtn);
    
    m_mainLayout->addLayout(rowLayout);
}

void StudentAttributeDialog::onAttributeValueClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    QString attributeName = btn->property("attributeName").toString();
    if (attributeName.isEmpty()) return;
    
    // 获取当前值
    double currentValue = m_student.attributes.value(attributeName, 0.0);
    if (currentValue == 0.0 && !m_student.attributes.contains(attributeName)) {
        if (attributeName == "总分" || attributeName == "数学") {
            currentValue = m_student.score;
        }
    }
    
    // 弹出输入对话框
    bool ok;
    double newValue = QInputDialog::getDouble(this, "修改属性值", 
        QString("请输入 %1 的新值:").arg(attributeName),
        currentValue, 0, 1000, 2, &ok);
    
    if (ok) {
        // 更新学生属性
        m_student.attributes[attributeName] = newValue;
        
        // 如果是"总分"，也更新score
        if (attributeName == "总分") {
            m_student.score = newValue;
        }
        
        // 更新按钮文本
        btn->setText(QString::number(newValue));
        
        // 发送更新信号
        emit attributeUpdated(m_student.id, attributeName, newValue);
    }
}

void StudentAttributeDialog::onAnnotationClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    QString attributeName = btn->property("attributeName").toString();
    if (attributeName.isEmpty()) return;
    
    // 可以在这里实现注释功能
    // 暂时显示一个消息框
    QMessageBox::information(this, "注释", 
        QString("为属性 \"%1\" 添加注释功能待实现").arg(attributeName));
}

void StudentAttributeDialog::onValueEditingFinished()
{
    // 处理编辑完成
}

void StudentAttributeDialog::clearAllHighlights()
{
    // 清除所有高亮
    for (auto it = m_attributeValueButtons.begin(); it != m_attributeValueButtons.end(); ++it) {
        it.value()->setStyleSheet(
            "QPushButton {"
            "background-color: green;"
            "color: white;"
            "font-size: 14px;"
            "padding: 8px 16px;"
            "border: none;"
            "border-radius: 4px;"
            "}"
            "QPushButton:hover {"
            "background-color: #00cc00;"
            "}"
        );
    }
    m_selectedAttribute = "";
}

// 鼠标进入窗口时显示关闭按钮
void StudentAttributeDialog::enterEvent(QEvent* event)
{
    if (m_btnClose) {
        m_btnClose->show();
    }
    QDialog::enterEvent(event);
}

// 鼠标离开窗口时隐藏关闭按钮
void StudentAttributeDialog::leaveEvent(QEvent* event)
{
    // 检查鼠标是否真的离开了窗口（包括关闭按钮）
    QPoint globalPos = QCursor::pos();
    QRect widgetRect = QRect(mapToGlobal(QPoint(0, 0)), size());
    if (!widgetRect.contains(globalPos) && m_btnClose) {
        // 如果鼠标不在窗口内，检查是否在关闭按钮上
        QRect btnRect = QRect(m_btnClose->mapToGlobal(QPoint(0, 0)), m_btnClose->size());
        if (!btnRect.contains(globalPos)) {
            m_btnClose->hide();
        }
    }
    QDialog::leaveEvent(event);
}

// 窗口大小改变时更新关闭按钮位置
void StudentAttributeDialog::resizeEvent(QResizeEvent* event)
{
    if (m_btnClose) {
        m_btnClose->move(width() - 35, 5);
    }
    QDialog::resizeEvent(event);
}

// 窗口显示时更新关闭按钮位置
void StudentAttributeDialog::showEvent(QShowEvent* event)
{
    if (m_btnClose) {
        m_btnClose->move(width() - 35, 5);
        // 窗口显示时也显示关闭按钮
        m_btnClose->show();
    }
    
    // 确保窗口位置在屏幕可见区域内
    QRect screenGeometry = QApplication::desktop()->availableGeometry();
    QRect windowGeometry = geometry();
    
    // 如果窗口完全在屏幕外，移动到屏幕中央
    if (!screenGeometry.intersects(windowGeometry)) {
        move(screenGeometry.center() - QPoint(windowGeometry.width() / 2, windowGeometry.height() / 2));
    }
    
    // 确保窗口显示在最前面
    raise();
    activateWindow();
    QDialog::showEvent(event);
}

// 事件过滤器，处理关闭按钮的鼠标事件
bool StudentAttributeDialog::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_btnClose) {
        if (event->type() == QEvent::Enter) {
            // 鼠标进入关闭按钮时确保显示
            m_btnClose->show();
        } else if (event->type() == QEvent::Leave) {
            // 鼠标离开关闭按钮时，检查是否还在窗口内
            QPoint globalPos = QCursor::pos();
            QRect widgetRect = QRect(mapToGlobal(QPoint(0, 0)), size());
            if (!widgetRect.contains(globalPos)) {
                m_btnClose->hide();
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}

// 重写鼠标事件以实现窗口拖动
void StudentAttributeDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void StudentAttributeDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton && !m_dragPosition.isNull()) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

void StudentAttributeDialog::setTitle(const QString& title)
{
    if (m_titleLabel) {
        m_titleLabel->setText(title);
    }
}

