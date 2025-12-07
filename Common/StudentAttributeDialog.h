#pragma once
#include "CommonInfo.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QList>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QEvent>
#include <QPoint>
#include <QCursor>
#include <QRect>
#include <QDesktopWidget>

class StudentAttributeDialog : public QDialog
{
    Q_OBJECT

signals:
    void attributeUpdated(const QString& studentId, const QString& attributeName, double newValue);

public:
    explicit StudentAttributeDialog(QWidget* parent = nullptr);

protected:
    // 鼠标进入窗口时显示关闭按钮
    void enterEvent(QEvent* event) override;
    
    // 鼠标离开窗口时隐藏关闭按钮
    void leaveEvent(QEvent* event) override;
    
    // 窗口大小改变时更新关闭按钮位置
    void resizeEvent(QResizeEvent* event) override;
    
    // 窗口显示时更新关闭按钮位置
    void showEvent(QShowEvent* event) override;
    
    // 事件过滤器，处理关闭按钮的鼠标事件
    bool eventFilter(QObject* obj, QEvent* event) override;
    
    // 重写鼠标事件以实现窗口拖动
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

public:
    // 设置学生信息
    void setStudentInfo(const struct StudentInfo& student);
    
    // 设置可用属性列表
    void setAvailableAttributes(const QList<QString>& attributes);
    
    // 设置标题
    void setTitle(const QString& title);

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
    
    QPushButton* m_btnClose = nullptr; // 关闭按钮
    QPoint m_dragPosition; // 用于窗口拖动
    QLabel* m_titleLabel = nullptr; // 标题标签
};

