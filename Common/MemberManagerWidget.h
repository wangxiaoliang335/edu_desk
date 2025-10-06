#include <QWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QHeaderView>
#include <QLabel>

class MemberManagerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MemberManagerWidget(QWidget* parent = nullptr) : QWidget(parent)
    {
        // 顶部按钮行
        QHBoxLayout* btnLayout = new QHBoxLayout;
        QString btnStyle = "QPushButton {background-color: green; color: white; font-size:16px; padding:6px 12px;}";
        QPushButton* btnAdd = new QPushButton("添加成员");
        QPushButton* btnImport = new QPushButton("导入");
        QPushButton* btnExport = new QPushButton("导出");
        QPushButton* btnDelete = new QPushButton("删除");
        btnAdd->setStyleSheet(btnStyle);
        btnImport->setStyleSheet(btnStyle);
        btnExport->setStyleSheet(btnStyle);
        btnDelete->setStyleSheet(btnStyle);
        btnLayout->addWidget(btnAdd);
        btnLayout->addWidget(btnImport);
        btnLayout->addWidget(btnExport);
        btnLayout->addWidget(btnDelete);
        btnLayout->addStretch();

        // 表格
        QTableWidget* table = new QTableWidget(5, 14); // 5 行，14 列
        table->setHorizontalHeaderLabels({
            "管理员", "系统唯一编号", "姓名", "电话", "身份证", "性别", "教龄", "学历",
            "毕业院校", "专业", "教师资格证学段", "教师资格证科目", "学段", ""
            });

        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->verticalHeader()->setVisible(false);
        table->setSelectionMode(QAbstractItemView::NoSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // 第一列加复选框
        for (int row = 0; row < table->rowCount(); ++row) {
            QWidget* chkContainer = new QWidget;
            QHBoxLayout* chkLay = new QHBoxLayout(chkContainer);
            QCheckBox* chk = new QCheckBox;
            chkLay->addWidget(chk);
            chkLay->setAlignment(Qt::AlignCenter);
            chkLay->setContentsMargins(0, 0, 0, 0);
            chkContainer->setLayout(chkLay);
            table->setCellWidget(row, 0, chkContainer);

            // 第二列输入一些示例数据
            QLabel* lbl = new QLabel(QString("2348681234")); // 换行
            lbl->setAlignment(Qt::AlignCenter);
            table->setCellWidget(row, 1, lbl);
        }

        // 主布局
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->addLayout(btnLayout);
        mainLayout->addWidget(table);
        setLayout(mainLayout);

        // 设置整体样式（可选）
        setStyleSheet("background-color: rgba(220, 230, 255, 255);"); // 浅蓝背景
    }
};
