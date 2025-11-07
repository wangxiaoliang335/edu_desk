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

// 前向声明
class ScheduleDialog;

// StudentInfo 结构体在 ScheduleDialog.h 中定义，这里使用条件编译避免重复定义
#ifndef STUDENT_INFO_DEFINED
#define STUDENT_INFO_DEFINED
struct StudentInfo {
    QString id;      // 学号
    QString name;    // 姓名
    double score;    // 成绩（用于排序）
    int originalIndex; // 原始索引
    QMap<QString, double> attributes; // 多个属性值（如"背诵"、"语文"等）
};
#endif

class MidtermGradeDialog : public QDialog
{
    Q_OBJECT

public:
    MidtermGradeDialog(QString classid, QWidget* parent = nullptr);
    
    // 导入Excel数据
    void importData(const QStringList& headers, const QList<QStringList>& dataRows);

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
    QTableWidget* table;
    QTextEdit* textDescription;
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
};
