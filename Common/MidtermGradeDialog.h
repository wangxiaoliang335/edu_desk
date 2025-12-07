#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QColorDialog>
#include <QMessageBox>
#include <QToolTip>
#include <QHeaderView>
#include <QMenu>
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QSet>
#include <QTextStream>
#include <QFile>
#include <QIODevice>
#include <QLineEdit>
#include <QCursor>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>
#include <QTimer>
#include <QApplication>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QPoint>
#include <QCursor>
#include <QRect>
#include <QDesktopWidget>
#include "MidtermGradeTableWidget.h"

// 前向声明
//class ScheduleDialog;
class MidtermGradeDialog : public QDialog
{
    Q_OBJECT

public:
    MidtermGradeDialog(QString classid, QWidget* parent = nullptr);
    
    // 导入Excel数据
    void importData(const QStringList& headers, const QList<QStringList>& dataRows, const QString& excelFilePath = QString());

protected:
    // 重写鼠标事件以实现窗口拖动
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    
    // 鼠标进入窗口时显示关闭按钮
    void enterEvent(QEvent *event) override;
    
    // 鼠标离开窗口时隐藏关闭按钮
    void leaveEvent(QEvent *event) override;
    
    // 事件过滤器，处理关闭按钮的鼠标事件
    bool eventFilter(QObject *obj, QEvent *event) override;
    
    // 窗口大小改变时更新关闭按钮位置
    void resizeEvent(QResizeEvent *event) override;
    
    // 窗口显示时更新关闭按钮位置
    void showEvent(QShowEvent *event) override;

private slots:
    void onAddRow();
    void onDeleteColumn();
    void onAddColumn();
    void onFontColor();
    void onBgColor();
    void onDescOrder();
    void onAscOrder();
    void onExport();
    void onUpload();
    void onCellClicked(int row, int column);
    void onCellEntered(int row, int column);
    void onTableContextMenu(const QPoint& pos);

private:
    void sortTable(bool ascending);
    void openSeatingArrangementDialog();
    void showCellComment(int row, int column);

private:
    MidtermGradeTableWidget* table;
    QTextEdit* textDescription;
    QLabel* m_lblTitle; // 标题标签
    QPushButton* btnAddRow;
    QPushButton* btnDeleteColumn;
    QPushButton* btnAddColumn;
    QPushButton* btnFontColor;
    QPushButton* btnBgColor;
    QPushButton* btnDescOrder;
    QPushButton* btnAscOrder;
    QPushButton* btnExport;
    QPushButton* btnUpload;
    QNetworkAccessManager* networkManager;
    
    QSet<int> fixedColumns; // 固定列索引集合（不能删除的列）
    int nameColumnIndex; // 姓名列索引
    QString m_classid;
    QPushButton* m_btnClose = nullptr; // 关闭按钮
    QPoint m_dragPosition; // 用于窗口拖动
    QString m_excelFilePath; // Excel文件路径
    QString m_excelFileName; // Excel文件名
};
