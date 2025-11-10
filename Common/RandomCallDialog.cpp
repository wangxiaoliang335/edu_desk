#include "RandomCallDialog.h"
#include "StudentAttributeDialog.h"
#include <QInputDialog>
#include <QTimer>
#include <QGraphicsEffect>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QApplication>
#include <random>
#include <algorithm>

RandomCallDialog::RandomCallDialog(QWidget* parent)
    : QDialog(parent)
    , m_seatTable(nullptr)
    , animationTimer(nullptr)
    , animationStep(0)
    , isAnimating(false)
    , m_dragging(false)
{
    // 设置无边框窗口
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setStyleSheet("background-color: #e0f0ff;");
    resize(500, 350); // 还原为原始大小

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 50, 30, 30);

    // 顶部栏：关闭按钮和数字按钮
    QHBoxLayout* topLayout = new QHBoxLayout;
    topLayout->addStretch();
    
    // 黄色圆圈带红色数字2
    QPushButton* btnNumber = new QPushButton("2");
    btnNumber->setFixedSize(30, 30);
    btnNumber->setStyleSheet(
        "QPushButton {"
        "background-color: yellow;"
        "color: red;"
        "border-radius: 15px;"
        "font-weight: bold;"
        "font-size: 16px;"
        "border: none;"
        "}"
    );
    topLayout->addWidget(btnNumber);
    
    // 绿色X关闭按钮
    QPushButton* btnClose = new QPushButton("✕");
    btnClose->setFixedSize(30, 30);
    btnClose->setStyleSheet(
        "QPushButton {"
        "background-color: green;"
        "color: white;"
        "border-radius: 15px;"
        "font-weight: bold;"
        "font-size: 18px;"
        "border: none;"
        "}"
        "QPushButton:hover {"
        "background-color: #00cc00;"
        "}"
    );
    connect(btnClose, &QPushButton::clicked, this, &QDialog::reject);
    topLayout->addWidget(btnClose);
    
    mainLayout->addLayout(topLayout);

    // 标题框："指定参与者"
    QLabel* titleLabel = new QLabel("指定参与者");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "background-color: black;"
        "color: white;"
        "font-size: 18px;"
        "font-weight: bold;"
        "padding: 10px;"
        "border: none;"
    );
    titleLabel->setFixedHeight(50);
    mainLayout->addWidget(titleLabel);

    // 表格和属性选择（绿色按钮）
    QHBoxLayout* selectLayout = new QHBoxLayout;
    selectLayout->setSpacing(15);
    
    // 表格选择按钮
    QPushButton* btnTable = new QPushButton("期中成绩表");
    btnTable->setStyleSheet(
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
    btnTable->setFixedHeight(35);
    connect(btnTable, &QPushButton::clicked, this, [this]() {
        // 可以在这里添加下拉菜单或对话框来选择表格
        // 暂时保持简单，使用ComboBox
    });
    selectLayout->addWidget(btnTable);
    
    // 属性选择按钮
    QPushButton* btnAttr = new QPushButton("数学");
    btnAttr->setStyleSheet(
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
    btnAttr->setFixedHeight(35);
    connect(btnAttr, &QPushButton::clicked, this, [this]() {
        // 可以在这里添加下拉菜单或对话框来选择属性
    });
    selectLayout->addWidget(btnAttr);
    
    selectLayout->addStretch();
    mainLayout->addLayout(selectLayout);

    // 隐藏的ComboBox（用于实际功能）
    tableComboBox = new QComboBox(this);
    tableComboBox->addItem("期中成绩表");
    tableComboBox->addItem("学生体质统计表");
    tableComboBox->hide();
    connect(tableComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, btnTable](int index) {
        btnTable->setText(tableComboBox->itemText(index));
        updateParticipants();
    });
    connect(btnTable, &QPushButton::clicked, this, [this, btnTable]() {
        // 显示下拉菜单
        // 将按钮的顶部位置转换为相对于对话框的坐标，X坐标对齐按钮左边，向上偏移一些让下拉菜单更靠近按钮
        QPoint globalPos = btnTable->mapToGlobal(QPoint(0, 0));
        QPoint localPos = this->mapFromGlobal(globalPos);
        // 设置下拉菜单的宽度与按钮相同，左边对齐
        tableComboBox->setFixedWidth(btnTable->width());
        tableComboBox->move(localPos);
        tableComboBox->showPopup();
    });

    attributeComboBox = new QComboBox(this);
    attributeComboBox->addItem("总分");
    attributeComboBox->addItem("语文");
    attributeComboBox->addItem("数学");
    attributeComboBox->addItem("英语");
    attributeComboBox->setCurrentText("数学");
    attributeComboBox->hide();
    connect(attributeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, btnAttr](int index) {
        btnAttr->setText(attributeComboBox->itemText(index));
        currentAttribute = attributeComboBox->currentText();
        updateParticipants();
    });
    connect(btnAttr, &QPushButton::clicked, this, [this, btnAttr]() {
        // 显示下拉菜单
        // 将按钮的顶部位置转换为相对于对话框的坐标，X坐标对齐按钮左边，向上偏移一些让下拉菜单更靠近按钮
        QPoint globalPos = btnAttr->mapToGlobal(QPoint(0, 0));
        QPoint localPos = this->mapFromGlobal(globalPos);
        // 设置下拉菜单的宽度与按钮相同，左边对齐
        attributeComboBox->setFixedWidth(btnAttr->width());
        attributeComboBox->move(localPos);
        attributeComboBox->showPopup();
    });

    // 属性值范围（灰色输入框）
    QHBoxLayout* rangeLayout = new QHBoxLayout;
    rangeLayout->setSpacing(10);
    
    minValueEdit = new QLineEdit("0");
    minValueEdit->setStyleSheet(
        "QLineEdit {"
        "background-color: #d0d0d0;"
        "border: 1px solid #999;"
        "padding: 8px;"
        "font-size: 14px;"
        "border-radius: 4px;"
        "}"
    );
    minValueEdit->setFixedSize(80, 35);
    minValueEdit->setAlignment(Qt::AlignCenter);
    
    QLabel* dashLabel = new QLabel("-");
    dashLabel->setStyleSheet("font-size: 16px; color: black;");
    dashLabel->setAlignment(Qt::AlignCenter);
    
    maxValueEdit = new QLineEdit("100");
    maxValueEdit->setStyleSheet(
        "QLineEdit {"
        "background-color: #d0d0d0;"
        "border: 1px solid #999;"
        "padding: 8px;"
        "font-size: 14px;"
        "border-radius: 4px;"
        "}"
    );
    maxValueEdit->setFixedSize(80, 35);
    maxValueEdit->setAlignment(Qt::AlignCenter);
    
    rangeLayout->addStretch();
    rangeLayout->addWidget(minValueEdit);
    rangeLayout->addWidget(dashLabel);
    rangeLayout->addWidget(maxValueEdit);
    rangeLayout->addStretch();
    mainLayout->addLayout(rangeLayout);

    // 信息标签（隐藏，因为图片中没有显示）
    lblInfo = new QLabel("参与者: 0 人");
    lblInfo->setStyleSheet("color: blue; font-weight: bold; font-size: 12px;");
    lblInfo->hide(); // 隐藏，因为图片中没有
    mainLayout->addWidget(lblInfo);

    mainLayout->addStretch();

    // 确定按钮（底部右侧）
    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    
    btnConfirm = new QPushButton("确定");
    btnConfirm->setStyleSheet(
        "QPushButton {"
        "background-color: green;"
        "color: white;"
        "font-size: 14px;"
        "padding: 8px 20px;"
        "border: none;"
        "border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "background-color: #00cc00;"
        "}"
    );
    btnConfirm->setFixedHeight(35);
    btnLayout->addWidget(btnConfirm);
    mainLayout->addLayout(btnLayout);

    connect(minValueEdit, &QLineEdit::textChanged, this, &RandomCallDialog::onRangeChanged);
    connect(maxValueEdit, &QLineEdit::textChanged, this, &RandomCallDialog::onRangeChanged);
    connect(btnConfirm, &QPushButton::clicked, this, &RandomCallDialog::onConfirm);

    // 初始化当前属性
    currentAttribute = "数学";

    // 初始化动画定时器
    animationTimer = new QTimer(this);
    animationTimer->setInterval(100); // 每100ms切换一次
    connect(animationTimer, &QTimer::timeout, this, &RandomCallDialog::onAnimationFinished);
}

void RandomCallDialog::setStudentData(const QList<struct StudentInfo>& students)
{
    m_students = students;
    updateParticipants();
}

void RandomCallDialog::setSeatTable(QTableWidget* seatTable)
{
    m_seatTable = seatTable;
    
    // 保存原始样式并连接所有座位按钮的点击事件
    if (m_seatTable) {
        for (int row = 0; row < 6; ++row) {
            for (int col = 0; col < 10; ++col) {
                QPushButton* btn = qobject_cast<QPushButton*>(m_seatTable->cellWidget(row, col));
                if (btn && btn->property("isSeat").toBool()) {
                    // 保存原始样式
                    if (!originalStyles.contains(btn)) {
                        originalStyles[btn] = btn->styleSheet();
                    }
                    // 断开之前的连接（如果有）
                    btn->disconnect(this);
                    // 重新连接点击事件
                    connect(btn, &QPushButton::clicked, this, &RandomCallDialog::onSeatClicked, Qt::UniqueConnection);
                }
            }
        }
    }
}

QList<struct StudentInfo> RandomCallDialog::getParticipants() const
{
    return m_participants;
}

void RandomCallDialog::onTableChanged(int index)
{
    Q_UNUSED(index);
    updateParticipants();
}

void RandomCallDialog::onAttributeChanged(int index)
{
    Q_UNUSED(index);
    currentAttribute = attributeComboBox->currentText();
    updateParticipants();
}

void RandomCallDialog::onRangeChanged()
{
    updateParticipants();
}

void RandomCallDialog::onConfirm()
{
    if (m_participants.isEmpty()) {
        QMessageBox::warning(this, "提示", "没有符合条件的参与者！");
        return;
    }

    // 停止之前的动画
    if (animationTimer->isActive()) {
        animationTimer->stop();
    }
    clearAllHighlights();

    // 开始随机动画
    startRandomAnimation();
}

void RandomCallDialog::onAnimationFinished()
{
    animationStep++;
    
    // 3秒 = 30步（每100ms一步）
    if (animationStep >= 30) {
        animationTimer->stop();
        isAnimating = false;
        qDebug() << "动画结束，步数:" << animationStep;
        
        // 随机选择一个参与者
        if (!m_participants.isEmpty()) {
            std::random_device rd;
            std::mt19937 g(rd());
            std::uniform_int_distribution<> dis(0, m_participants.size() - 1);
            int randomIndex = dis(g);
            selectedStudentId = m_participants[randomIndex].id;
            
            qDebug() << "最终选中的学生ID:" << selectedStudentId << "，姓名:" << m_participants[randomIndex].name;
            
            // 高亮选中的学生
            highlightStudent(selectedStudentId, true);
            
            QMessageBox::information(this, "随机点名", 
                QString("选中学生: %1 (学号: %2)").arg(m_participants[randomIndex].name).arg(selectedStudentId));
        }
        return;
    }
    
    // 随机高亮一个参与者（动画过程中）
    if (!m_participants.isEmpty()) {
        clearAllHighlights();
        // 强制刷新界面，确保清除效果可见
        if (m_seatTable) {
            m_seatTable->update();
            QApplication::processEvents(); // 处理事件，确保界面更新
        }
        
        std::random_device rd;
        std::mt19937 g(rd());
        std::uniform_int_distribution<> dis(0, m_participants.size() - 1);
        int randomIndex = dis(g);
        QString studentId = m_participants[randomIndex].id;
        qDebug() << "动画步骤" << animationStep << "，高亮学生ID:" << studentId;
        highlightStudent(studentId, true);
        
        // 再次强制刷新界面
        QApplication::processEvents();
    }
}

void RandomCallDialog::onSeatClicked()
{
    if (isAnimating) return; // 动画进行中不允许点击
    
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    QString studentId = btn->property("studentId").toString();
    if (studentId.isEmpty()) return;
    
    // 检查是否是选中的学生
    if (studentId != selectedStudentId) {
        QMessageBox::information(this, "提示", "请点击被选中的学生！");
        return;
    }
    
    // 查找学生信息
    struct StudentInfo* student = nullptr;
    for (auto& s : m_students) {
        if (s.id == studentId) {
            student = &s;
            break;
        }
    }
    
    if (!student) return;
    
    // 打开属性编辑对话框
    StudentAttributeDialog* attrDlg = new StudentAttributeDialog(this);
    
    // 设置可用属性列表（根据当前选择的表格和属性）
    QList<QString> attributes;
    if (tableComboBox->currentText() == "期中成绩表") {
        attributes = QStringList() << "背诵" << "语文" << "数学" << "英语" << "总分";
    } else {
        attributes = QStringList() << "身高" << "体重" << "肺活量" << "50米跑";
    }
    attrDlg->setAvailableAttributes(attributes);
    
    // 设置学生信息
    attrDlg->setStudentInfo(*student);
    
    // 连接属性更新信号
    connect(attrDlg, &StudentAttributeDialog::attributeUpdated, this, 
        [this, studentId](const QString& id, const QString& attrName, double newValue) {
            // 更新学生数据
            for (auto& s : m_students) {
                if (s.id == id) {
                    s.attributes[attrName] = newValue;
                    // 如果是当前选择的属性，也更新score
                    if (attrName == currentAttribute || attrName == "总分") {
                        s.score = newValue;
                    }
                    break;
                }
            }
            // 更新参与者列表
            for (auto& p : m_participants) {
                if (p.id == id) {
                    p.attributes[attrName] = newValue;
                    if (attrName == currentAttribute || attrName == "总分") {
                        p.score = newValue;
                    }
                    break;
                }
            }
            // 发送信号通知父窗口
            emit studentScoreUpdated(id, newValue);
            // 重新计算参与者
            updateParticipants();
        });
    
    attrDlg->exec();
    attrDlg->deleteLater();
}

void RandomCallDialog::updateParticipants()
{
    m_participants.clear();
    
    if (m_students.isEmpty()) {
        lblInfo->setText("参与者: 0 人");
        return;
    }
    
    // 获取范围值
    bool ok1, ok2;
    double minValue = minValueEdit->text().toDouble(&ok1);
    double maxValue = maxValueEdit->text().toDouble(&ok2);
    
    if (!ok1 || !ok2) {
        lblInfo->setText("参与者: 0 人（范围值无效）");
        return;
    }
    
    // 根据当前选择的属性筛选参与者
    QString selectedAttr = currentAttribute;
    for (const auto& student : m_students) {
        double value = student.score; // 默认使用score
        
        // 如果学生有该属性的值，使用属性值
        if (student.attributes.contains(selectedAttr)) {
            value = student.attributes[selectedAttr];
        } else if (selectedAttr == "总分" && student.attributes.contains("总分")) {
            value = student.attributes["总分"];
        } else if (selectedAttr == "数学" && student.attributes.contains("数学")) {
            value = student.attributes["数学"];
        } else if (selectedAttr == "语文" && student.attributes.contains("语文")) {
            value = student.attributes["语文"];
        } else if (selectedAttr == "英语" && student.attributes.contains("英语")) {
            value = student.attributes["英语"];
        }
        
        if (value >= minValue && value <= maxValue) {
            m_participants.append(student);
        }
    }
    
    lblInfo->setText(QString("参与者: %1 人").arg(m_participants.size()));
}

void RandomCallDialog::startRandomAnimation()
{
    if (m_participants.isEmpty()) {
        qDebug() << "startRandomAnimation: 参与者列表为空";
        return;
    }
    
    qDebug() << "开始随机动画，参与者数量:" << m_participants.size() << "，座位表格:" << (m_seatTable ? "存在" : "不存在");
    isAnimating = true;
    animationStep = 0;
    animationTimer->start();
    qDebug() << "动画定时器已启动，间隔:" << animationTimer->interval() << "ms";
}

void RandomCallDialog::highlightStudent(const QString& studentId, bool highlight)
{
    QPushButton* btn = findSeatButtonByStudentId(studentId);
    if (!btn) {
        qDebug() << "highlightStudent: 未找到学生ID为" << studentId << "的座位按钮";
        return;
    }
    
    if (highlight) {
        // 保存原始样式（如果还没有保存）
        if (!originalStyles.contains(btn)) {
            originalStyles[btn] = btn->styleSheet();
        }
        
        // 添加高亮效果：明显的蓝色边框和背景
        QString highlightStyle = 
            "QPushButton { "
            "border: 5px solid #0066FF !important; "
            "background-color: #3399FF !important; "
            "color: #FFFFFF !important; "
            "font-weight: bold !important; "
            "font-size: 14px !important; "
            "border-radius: 4px !important; "
            "}";
        btn->setStyleSheet(highlightStyle);
        
        // 添加明显的蓝色阴影效果
        QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect;
        shadow->setBlurRadius(25);
        shadow->setColor(QColor(0, 102, 255, 255)); // 蓝色阴影
        shadow->setOffset(0, 0);
        shadow->setXOffset(0);
        shadow->setYOffset(0);
        btn->setGraphicsEffect(shadow);
        
        // 确保按钮可见（提升到最前）
        btn->raise();
        btn->repaint(); // 强制重绘
        btn->update(); // 更新显示
        
        // 如果按钮在表格中，也需要更新表格
        if (m_seatTable) {
            m_seatTable->update();
            m_seatTable->repaint();
        }
        
        qDebug() << "高亮学生ID:" << studentId << "，按钮位置:" << btn->pos() << "，按钮文本:" << btn->text();
    } else {
        // 移除高亮效果
        btn->setGraphicsEffect(nullptr);
        // 恢复原始样式
        if (originalStyles.contains(btn)) {
            btn->setStyleSheet(originalStyles[btn]);
        }
    }
}

void RandomCallDialog::clearAllHighlights()
{
    if (!m_seatTable) return;
    
    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < 10; ++col) {
            QPushButton* btn = qobject_cast<QPushButton*>(m_seatTable->cellWidget(row, col));
            if (btn && btn->property("isSeat").toBool()) {
                btn->setGraphicsEffect(nullptr);
                // 恢复原始样式
                if (originalStyles.contains(btn)) {
                    btn->setStyleSheet(originalStyles[btn]);
                }
            }
        }
    }
}

QPushButton* RandomCallDialog::findSeatButtonByStudentId(const QString& studentId)
{
    if (!m_seatTable) return nullptr;
    
    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < 10; ++col) {
            QPushButton* btn = qobject_cast<QPushButton*>(m_seatTable->cellWidget(row, col));
            if (btn && btn->property("isSeat").toBool()) {
                if (btn->property("studentId").toString() == studentId) {
                    return btn;
                }
            }
        }
    }
    return nullptr;
}

void RandomCallDialog::updateStudentScore(const QString& studentId, double newScore)
{
    // 更新学生数据中的成绩
    for (auto& student : m_students) {
        if (student.id == studentId) {
            student.score = newScore;
            break;
        }
    }
    
    // 更新参与者列表中的成绩
    for (auto& participant : m_participants) {
        if (participant.id == studentId) {
            participant.score = newScore;
            break;
        }
    }
    
    // 通知父窗口更新学生数据
    emit studentScoreUpdated(studentId, newScore);
}

double RandomCallDialog::getStudentScore(const QString& studentId)
{
    for (const auto& student : m_students) {
        if (student.id == studentId) {
            return student.score;
        }
    }
    return 0.0;
}

QString RandomCallDialog::getStudentName(const QString& studentId)
{
    for (const auto& student : m_students) {
        if (student.id == studentId) {
            return student.name;
        }
    }
    return "";
}

void RandomCallDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
    QDialog::mousePressEvent(event);
}

void RandomCallDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
    QDialog::mouseMoveEvent(event);
}

void RandomCallDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
    QDialog::mouseReleaseEvent(event);
}

