#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsEffect>
#include <QGraphicsDropShadowEffect>
#include <QDebug>
#include <QList>
#include "ScheduleDialog.h" // 包含 StudentInfo 定义

class RandomCallDialog : public QDialog
{
    Q_OBJECT

signals:
    void studentScoreUpdated(const QString& studentId, double newScore); // 学生成绩更新信号（兼容旧接口）
    void studentAttributeUpdated(const QString& studentId, const QString& attributeName, double newValue); // 学生属性更新信号

public:
    explicit RandomCallDialog(QWidget* parent = nullptr);
    
    // 设置学生数据
    void setStudentData(const QList<struct StudentInfo>& students);
    
    // 设置座位表格（用于高亮显示）
    void setSeatTable(QTableWidget* seatTable);
    
    // 获取选中的参与者
    QList<struct StudentInfo> getParticipants() const;

private slots:
    void onTableChanged(int index);
    void onAttributeChanged(int index);
    void onRangeChanged();
    void onConfirm();
    void onAnimationFinished();
    void onSeatClicked();

private:
    void updateParticipants();
    void startRandomAnimation();
    void highlightStudent(const QString& studentId, bool highlight);
    void clearAllHighlights();
    QPushButton* findSeatButtonByStudentId(const QString& studentId);
    void updateStudentScore(const QString& studentId, double newScore);
    double getStudentScore(const QString& studentId);
    QString getStudentName(const QString& studentId);
    
    QComboBox* tableComboBox; // 隐藏的ComboBox，用于实际功能
    QComboBox* attributeComboBox; // 隐藏的ComboBox，用于实际功能
    QLineEdit* minValueEdit;
    QLineEdit* maxValueEdit;
    QPushButton* btnConfirm;
    QLabel* lblInfo;
    
    QList<struct StudentInfo> m_students;
    QList<struct StudentInfo> m_participants;
    QTableWidget* m_seatTable;
    
    QTimer* animationTimer;
    int animationStep;
    QString selectedStudentId;
    QString currentAttribute; // 当前选择的属性名称
    bool isAnimating;
    
    QMap<QPushButton*, QString> originalStyles; // 保存原始按钮样式
};

