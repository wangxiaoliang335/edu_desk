#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QList>
#include "ScheduleDialog.h" // 包含 StudentInfo 定义

class StudentAttributeDialog : public QDialog
{
    Q_OBJECT

signals:
    void attributeUpdated(const QString& studentId, const QString& attributeName, double newValue);

public:
    explicit StudentAttributeDialog(QWidget* parent = nullptr);
    
    // 设置学生信息
    void setStudentInfo(const struct StudentInfo& student);
    
    // 设置可用属性列表
    void setAvailableAttributes(const QList<QString>& attributes);

private slots:
    void onAttributeValueClicked();
    void onAnnotationClicked();
    void onValueEditingFinished();

private:
    void updateAttributeRow(const QString& attributeName, double value, bool isHighlighted = false);
    void clearAllHighlights();
    
    struct StudentInfo m_student;
    QList<QString> m_availableAttributes;
    
    QVBoxLayout* m_mainLayout;
    QMap<QString, QPushButton*> m_attributeValueButtons; // 属性名 -> 值按钮
    QMap<QString, QPushButton*> m_annotationButtons; // 属性名 -> 注释按钮
    QMap<QString, QLineEdit*> m_valueEdits; // 属性名 -> 输入框（隐藏，用于编辑）
    QString m_currentEditingAttribute; // 当前正在编辑的属性
    QString m_selectedAttribute; // 当前选中的属性
};

