#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QButtonGroup>
#include <QRadioButton>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QList>
#include <QMap>
#include <QVector>
#include <QRandomGenerator>
#include <algorithm>
#include <QCloseEvent>
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QPoint>
#include <QDebug>
#include <random>

// 学生信息结构
struct StudentInfo {
    QString id;      // 学号
    QString name;    // 姓名
    double score;    // 成绩（用于排序）
    int originalIndex; // 原始索引
};

// 座位位置
struct SeatPosition {
    int row;    // 行（从0开始）
    int col;    // 列（从0开始）
    bool isSideSeat; // 是否是讲台两边的座位
    
    bool operator<(const SeatPosition& other) const {
        if (row != other.row) return row < other.row;
        return col < other.col;
    }
    
    bool operator==(const SeatPosition& other) const {
        return row == other.row && col == other.col;
    }
};

class SeatWidget : public QLabel
{
    Q_OBJECT
public:
    SeatWidget(const QString& text, QWidget* parent = nullptr) 
        : QLabel(text, parent), m_isOccupied(false), m_studentIndex(-1)
    {
        setAlignment(Qt::AlignCenter);
        setStyleSheet("border: 2px solid #333; background-color: #f0f0f0; padding: 5px; min-width: 80px; min-height: 40px;");
        setAcceptDrops(true);
    }
    
    void setOccupied(bool occupied, int studentIndex = -1, const QString& studentId = "") {
        m_isOccupied = occupied;
        m_studentIndex = studentIndex;
        m_studentId = studentId;
        if (occupied) {
            setStyleSheet("border: 2px solid #0066cc; background-color: #e6f3ff; padding: 5px; min-width: 80px; min-height: 40px; font-weight: bold;");
            // 设置学号属性
            setProperty("studentId", studentId);
        } else {
            setStyleSheet("border: 2px solid #333; background-color: #f0f0f0; padding: 5px; min-width: 80px; min-height: 40px;");
            setProperty("studentId", "");
        }
    }
    
    bool isOccupied() const { return m_isOccupied; }
    int getStudentIndex() const { return m_studentIndex; }
    void setStudentIndex(int index) { m_studentIndex = index; }
    QString getStudentId() const { return m_studentId; }
    
protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && m_isOccupied) {
            m_dragStartPos = event->pos();
        }
        QLabel::mousePressEvent(event);
    }
    
    void mouseMoveEvent(QMouseEvent* event) override {
        if (event->buttons() & Qt::LeftButton && m_isOccupied) {
            int distance = (event->pos() - m_dragStartPos).manhattanLength();
            if (distance > QApplication::startDragDistance()) {
                QDrag* drag = new QDrag(this);
                QMimeData* mimeData = new QMimeData;
                mimeData->setText(QString::number(m_studentIndex));
                drag->setMimeData(mimeData);
                drag->exec(Qt::MoveAction);
            }
        }
        QLabel::mouseMoveEvent(event);
    }
    
    void dragEnterEvent(QDragEnterEvent* event) override {
        if (event->mimeData()->hasText()) {
            event->acceptProposedAction();
        }
    }
    
    void dropEvent(QDropEvent* event) override {
        if (event->mimeData()->hasText()) {
            int draggedStudentIndex = event->mimeData()->text().toInt();
            emit seatSwapped(m_studentIndex, draggedStudentIndex);
            event->acceptProposedAction();
        }
    }
    
signals:
    void seatSwapped(int fromIndex, int toIndex);
    
private:
    bool m_isOccupied;
    int m_studentIndex;
    QString m_studentId; // 学号
    QPoint m_dragStartPos;
};

class SeatingArrangementDialog : public QDialog
{
    Q_OBJECT
    
public:
    SeatingArrangementDialog(const QList<StudentInfo>& students, QWidget* parent = nullptr);
    void arrangeSeats(); // 改为public，允许外部调用
    
private slots:
    void onArrangeClicked();
    void onCloseClicked();
    void onSeatSwapped(int fromIndex, int toIndex);
    
private:
    void setupUI();
    void createSeatLayout();
    void arrangeRandom();
    void arrangeAscending();
    void arrangeDescending();
    void arrangeGroup2();
    void arrangeGroup4();
    void arrangeGroup6();
    
    // 辅助函数
    QList<StudentInfo> getStudentsSortedByScore(bool ascending);
    void clearSeats();
    void updateSeatDisplay();
    int calculateRows() const;
    int calculateCols() const;
    
    // 成员变量
    QList<StudentInfo> m_students;
    QList<StudentInfo> m_arrangedStudents; // 已排座的学生列表
    QMap<int, SeatPosition> m_seatMap; // 学生索引 -> 座位位置
    QMap<SeatPosition, SeatWidget*> m_seatWidgets; // 座位位置 -> 座位控件
    
    // UI控件
    QComboBox* m_groupComboBox;
    QComboBox* m_methodComboBox;
    QRadioButton* m_radioAscending;
    QRadioButton* m_radioDescending;
    QPushButton* m_btnArrange;
    QPushButton* m_btnClose;
    QWidget* m_seatContainer;
    QGridLayout* m_seatLayout;
    QLabel* m_lblPlatform; // 讲台标签
};

// 实现 SeatingArrangementDialog
inline SeatingArrangementDialog::SeatingArrangementDialog(const QList<StudentInfo>& students, QWidget* parent)
    : QDialog(parent), m_students(students)
{
    setWindowTitle("排座");
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    resize(1200, 800);
    setStyleSheet("background-color: #f5f5dc; font-size:14px;");
    
    setupUI();
    createSeatLayout();
}

inline void SeatingArrangementDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // 顶部控制区域
    QHBoxLayout* controlLayout = new QHBoxLayout;
    
    // 分组选择
    QLabel* lblGroup = new QLabel("分组:");
    m_groupComboBox = new QComboBox;
    m_groupComboBox->addItem("不分组");
    m_groupComboBox->addItem("2人组");
    m_groupComboBox->addItem("4人组");
    m_groupComboBox->addItem("6人组");
    
    // 方法选择
    QLabel* lblMethod = new QLabel("排座方式:");
    m_methodComboBox = new QComboBox;
    m_methodComboBox->addItem("随机排座");
    m_methodComboBox->addItem("正序");
    m_methodComboBox->addItem("倒序");
    
    // 排序方向
    QLabel* lblOrder = new QLabel("排序方向:");
    m_radioAscending = new QRadioButton("正序");
    m_radioDescending = new QRadioButton("倒序");
    m_radioDescending->setChecked(true);
    QButtonGroup* orderGroup = new QButtonGroup(this);
    orderGroup->addButton(m_radioAscending);
    orderGroup->addButton(m_radioDescending);
    
    // 按钮
    m_btnArrange = new QPushButton("排座");
    m_btnArrange->setStyleSheet("background-color: green; color: white; padding: 6px 12px; border-radius: 4px; font-size: 14px;");
    m_btnClose = new QPushButton("×");
    m_btnClose->setStyleSheet("background-color: red; color: white; padding: 6px 12px; border-radius: 4px; font-size: 16px; font-weight: bold;");
    m_btnClose->setFixedSize(30, 30);
    
    controlLayout->addWidget(lblGroup);
    controlLayout->addWidget(m_groupComboBox);
    controlLayout->addWidget(lblMethod);
    controlLayout->addWidget(m_methodComboBox);
    controlLayout->addWidget(lblOrder);
    controlLayout->addWidget(m_radioAscending);
    controlLayout->addWidget(m_radioDescending);
    controlLayout->addStretch();
    controlLayout->addWidget(m_btnArrange);
    controlLayout->addWidget(m_btnClose);
    
    mainLayout->addLayout(controlLayout);
    
    // 座位容器
    m_seatContainer = new QWidget;
    m_seatLayout = new QGridLayout(m_seatContainer);
    m_seatLayout->setSpacing(5);
    mainLayout->addWidget(m_seatContainer);
    
    // 连接信号
    connect(m_btnArrange, &QPushButton::clicked, this, &SeatingArrangementDialog::onArrangeClicked);
    connect(m_btnClose, &QPushButton::clicked, this, &SeatingArrangementDialog::onCloseClicked);
}

inline void SeatingArrangementDialog::createSeatLayout()
{
    // 计算座位布局：假设每行8个座位，讲台在中间
    int rows = calculateRows();
    int cols = calculateCols();
    
    // 清空现有座位
    QLayoutItem* item;
    while ((item = m_seatLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    m_seatWidgets.clear();
    
    // 创建讲台标签（在中间行）
    int platformRow = rows / 2;
    m_lblPlatform = new QLabel("讲台");
    m_lblPlatform->setAlignment(Qt::AlignCenter);
    m_lblPlatform->setStyleSheet("background-color: #8B4513; color: white; font-size: 16px; font-weight: bold; padding: 10px; border-radius: 5px;");
    m_lblPlatform->setFixedHeight(60);
    m_seatLayout->addWidget(m_lblPlatform, platformRow, cols / 2 - 1, 1, 2);
    
    // 创建座位（讲台两边各2个座位）
    int seatIndex = 0;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            // 跳过讲台位置
            if (row == platformRow && (col == cols / 2 - 1 || col == cols / 2)) {
                continue;
            }
            
            SeatPosition pos;
            pos.row = row;
            pos.col = col;
            pos.isSideSeat = (row == platformRow && (col == cols / 2 - 2 || col == cols / 2 + 1));
            
            SeatWidget* seat = new SeatWidget("");
            connect(seat, &SeatWidget::seatSwapped, this, &SeatingArrangementDialog::onSeatSwapped);
            m_seatLayout->addWidget(seat, row, col);
            m_seatWidgets[pos] = seat;
            seatIndex++;
        }
    }
}

inline int SeatingArrangementDialog::calculateRows() const
{
    // 根据学生数量计算行数，每行8个座位（不包括讲台）
    int totalSeats = m_students.size() + 4; // 学生数 + 讲台两边4个座位
    int cols = 8;
    return (totalSeats + cols - 1) / cols;
}

inline int SeatingArrangementDialog::calculateCols() const
{
    return 8; // 固定8列
}

inline void SeatingArrangementDialog::onArrangeClicked()
{
    arrangeSeats();
}

inline void SeatingArrangementDialog::onCloseClicked()
{
    close();
}

inline void SeatingArrangementDialog::onSeatSwapped(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || toIndex < 0 || fromIndex >= m_arrangedStudents.size() || toIndex >= m_arrangedStudents.size()) {
        return;
    }
    
    // 交换学生
    StudentInfo temp = m_arrangedStudents[fromIndex];
    m_arrangedStudents[fromIndex] = m_arrangedStudents[toIndex];
    m_arrangedStudents[toIndex] = temp;
    
    // 更新显示
    updateSeatDisplay();
}

inline void SeatingArrangementDialog::arrangeSeats()
{
    clearSeats();
    
    QString groupMode = m_groupComboBox->currentText();
    QString method = m_methodComboBox->currentText();
    
    if (groupMode == "不分组") {
        if (method == "随机排座") {
            arrangeRandom();
        } else if (method == "正序") {
            arrangeAscending();
        } else if (method == "倒序") {
            arrangeDescending();
        }
    } else if (groupMode == "2人组") {
        arrangeGroup2();
    } else if (groupMode == "4人组") {
        arrangeGroup4();
    } else if (groupMode == "6人组") {
        arrangeGroup6();
    }
    
    updateSeatDisplay();
}

inline void SeatingArrangementDialog::clearSeats()
{
    m_arrangedStudents.clear();
    m_seatMap.clear();
    
    for (auto it = m_seatWidgets.begin(); it != m_seatWidgets.end(); ++it) {
        it.value()->setOccupied(false);
        it.value()->setText("");
    }
}

inline void SeatingArrangementDialog::updateSeatDisplay()
{
    // 清空所有座位
    for (auto it = m_seatWidgets.begin(); it != m_seatWidgets.end(); ++it) {
        it.value()->setOccupied(false);
        it.value()->setText("");
        it.value()->setStudentIndex(-1);
    }
    
    // 填充座位（从左到右，从上到下）
    QList<SeatPosition> sortedPositions = m_seatWidgets.keys();
    std::sort(sortedPositions.begin(), sortedPositions.end(), [](const SeatPosition& a, const SeatPosition& b) {
        if (a.row != b.row) return a.row < b.row;
        return a.col < b.col;
    });
    
    int studentIndex = 0;
    for (const SeatPosition& pos : sortedPositions) {
        if (studentIndex >= m_arrangedStudents.size()) break;
        
        const StudentInfo& student = m_arrangedStudents[studentIndex];
        SeatWidget* seat = m_seatWidgets[pos];
        seat->setText(student.name); // 显示学生姓名
        seat->setOccupied(true, studentIndex, student.id); // 设置学号属性
        m_seatMap[studentIndex] = pos;
        studentIndex++;
    }
}

inline QList<StudentInfo> SeatingArrangementDialog::getStudentsSortedByScore(bool ascending)
{
    QList<StudentInfo> sorted = m_students;
    std::sort(sorted.begin(), sorted.end(), [ascending](const StudentInfo& a, const StudentInfo& b) {
        if (ascending) {
            return a.score < b.score;
        } else {
            return a.score > b.score;
        }
    });
    return sorted;
}

inline void SeatingArrangementDialog::arrangeRandom()
{
    m_arrangedStudents = m_students;
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(m_arrangedStudents.begin(), m_arrangedStudents.end(), g);
}

inline void SeatingArrangementDialog::arrangeAscending()
{
    m_arrangedStudents = getStudentsSortedByScore(true);
}

inline void SeatingArrangementDialog::arrangeDescending()
{
    m_arrangedStudents = getStudentsSortedByScore(false);
}

inline void SeatingArrangementDialog::arrangeGroup2()
{
    // 2人组：同桌帮扶
    // 将学生按成绩分成两半：前一半和后一半
    QList<StudentInfo> sorted = getStudentsSortedByScore(false); // 降序
    
    int total = sorted.size();
    int half = total / 2;
    
    m_arrangedStudents.clear();
    
    // 前一半和后一半配对
    for (int i = 0; i < half; ++i) {
        m_arrangedStudents.append(sorted[i]);
        if (i + half < total) {
            m_arrangedStudents.append(sorted[i + half]);
        }
    }
    
    // 如果总数是奇数，最后一个单独
    if (total % 2 == 1) {
        m_arrangedStudents.append(sorted[half]);
    }
}

inline void SeatingArrangementDialog::arrangeGroup4()
{
    // 4人组：按用户描述的方法
    QList<StudentInfo> sorted = getStudentsSortedByScore(false); // 降序
    
    int total = sorted.size();
    int groupSize = 4;
    int fullGroups = total / groupSize;
    int remainder = total % groupSize;
    
    // 将前 fullGroups * groupSize 个学生分成 groupSize 等份
    int studentsPerLevel = fullGroups;
    QList<QList<StudentInfo>> levels;
    levels.reserve(groupSize);
    for (int i = 0; i < groupSize; ++i) {
        levels.append(QList<StudentInfo>());
    }
    
    for (int i = 0; i < fullGroups * groupSize; ++i) {
        int level = i % groupSize;
        levels[level].append(sorted[i]);
    }
    
    // 随机打乱每个等份
    std::random_device rd;
    std::mt19937 g(rd());
    for (int i = 0; i < groupSize; ++i) {
        std::shuffle(levels[i].begin(), levels[i].end(), g);
    }
    
    // 从每个等份中依次选取组成小组
    m_arrangedStudents.clear();
    for (int group = 0; group < fullGroups; ++group) {
        for (int level = 0; level < groupSize; ++level) {
            if (!levels[level].isEmpty()) {
                m_arrangedStudents.append(levels[level].takeFirst());
            }
        }
    }
    
    // 剩余学生
    for (int i = fullGroups * groupSize; i < total; ++i) {
        m_arrangedStudents.append(sorted[i]);
    }
}

inline void SeatingArrangementDialog::arrangeGroup6()
{
    // 6人组：与4人组相同的方法
    QList<StudentInfo> sorted = getStudentsSortedByScore(false); // 降序
    
    int total = sorted.size();
    int groupSize = 6;
    int fullGroups = total / groupSize;
    int remainder = total % groupSize;
    
    // 将前 fullGroups * groupSize 个学生分成 groupSize 等份
    int studentsPerLevel = fullGroups;
    QList<QList<StudentInfo>> levels;
    levels.reserve(groupSize);
    for (int i = 0; i < groupSize; ++i) {
        levels.append(QList<StudentInfo>());
    }
    
    for (int i = 0; i < fullGroups * groupSize; ++i) {
        int level = i % groupSize;
        levels[level].append(sorted[i]);
    }
    
    // 随机打乱每个等份
    std::random_device rd;
    std::mt19937 g(rd());
    for (int i = 0; i < groupSize; ++i) {
        std::shuffle(levels[i].begin(), levels[i].end(), g);
    }
    
    // 从每个等份中依次选取组成小组
    m_arrangedStudents.clear();
    for (int group = 0; group < fullGroups; ++group) {
        for (int level = 0; level < groupSize; ++level) {
            if (!levels[level].isEmpty()) {
                m_arrangedStudents.append(levels[level].takeFirst());
            }
        }
    }
    
    // 剩余学生
    for (int i = fullGroups * groupSize; i < total; ++i) {
        m_arrangedStudents.append(sorted[i]);
    }
}

