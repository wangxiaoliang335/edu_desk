#include <QWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QHeaderView>
#include <QLabel>
#include <qdebug.h>
#include <qfiledialog.h>
#include <qjsonobject.h>
#include <qjsondocument.h>
#include <QNetworkRequest>
#include <qnetworkreply.h>
#include <qjsonarray.h>
#include <qregularexpression.h>
#include <QIcon>
#include "TAIconCheckDelegate.h"
#include "TABaseDialog.h"
#include "CustomMessageDialog.h"

class MemberManagerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MemberManagerWidget(QWidget* parent = nullptr) : QWidget(parent)
    {
        // 顶部按钮行（深色主题 + 图标）
        QHBoxLayout* btnLayout = new QHBoxLayout;
        btnLayout->setContentsMargins(0, 0, 0, 0);
        btnLayout->setSpacing(12);

        const QString btnStyle =
            "QPushButton { "
            "background-color: rgba(255, 255, 255, 0.1); "
            "color: white; "
            "font-size: 14px; "
            "border: 1px solid rgba(255, 255, 255, 0.2); "
            "border-radius: 6px; "
            "padding: 6px 16px; "
            "} "
            "QPushButton:hover { "
            "background-color: rgba(255, 255, 255, 0.15); "
            "}";

        btnAdd = new QPushButton("添加成员");
        btnAdd->setIcon(QIcon(":/res/img/com_card_ic_member@3x.png"));
        btnAdd->setIconSize(QSize(20, 20));
        btnAdd->setFixedHeight(40);
        btnAdd->setStyleSheet(btnStyle);

        btnImport = new QPushButton("导入");
        btnImport->setIcon(QIcon(":/res/img/com_card_ic_import@3x.png"));
        btnImport->setIconSize(QSize(20, 20));
        btnImport->setFixedHeight(40);
        btnImport->setStyleSheet(btnStyle);

        btnExport = new QPushButton("导出");
        btnExport->setIcon(QIcon(":/res/img/com_card_ic_export@3x.png"));
        btnExport->setIconSize(QSize(20, 20));
        btnExport->setFixedHeight(40);
        btnExport->setStyleSheet(btnStyle);

        btnDelete = new QPushButton("删除");
        btnDelete->setIcon(QIcon(":/res/img/com_card_ic_del@3x.png"));
        btnDelete->setIconSize(QSize(20, 20));
        btnDelete->setFixedHeight(40);
        btnDelete->setStyleSheet(btnStyle);

        // 生成按钮保留逻辑但不展示（导入后会自动调用 onGenerate）
        btnGenerate = new QPushButton("生成");
        btnGenerate->setIcon(QIcon(":/res/img/com_card_ic_generate@3x.png"));
        btnGenerate->setIconSize(QSize(20, 20));
        btnGenerate->setFixedHeight(40);
        btnGenerate->setStyleSheet(btnStyle);

        btnLayout->addWidget(btnAdd);
        btnLayout->addWidget(btnImport);
        btnLayout->addWidget(btnExport);
        btnLayout->addWidget(btnDelete);
        btnLayout->addStretch();
        btnLayout->addWidget(btnGenerate);

        manager_ = new QNetworkAccessManager(this); // 初始化网络对象

        // 表格
        table = new QTableWidget(0, 14); // 5 行，14 列
        table->setHorizontalHeaderLabels({
            "管理员", "系统唯一编号", "姓名", "电话", "身份证", "性别", "教龄", "学历",
            "毕业院校", "专业", "教师资格证学段", "教师资格证科目", "学段", ""
            });

        table->verticalHeader()->setVisible(false);
        //table->setSelectionMode(QAbstractItemView::NoSelection);
        //table->setEditTriggers(QAbstractItemView::NoEditTriggers);

        table->setSelectionBehavior(QAbstractItemView::SelectRows);   // 按行选择
        table->setSelectionMode(QAbstractItemView::ExtendedSelection); // 支持多行

        //table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive); // 手动调整
        // 复选框列使用图片自绘（checked 时叠加蓝色对勾）
        table->setItemDelegateForColumn(0, new TAIconCheckDelegate(table));

        // 第一列加复选框
        for (int row = 0; row < table->rowCount(); ++row) {
            //QWidget* chkContainer = new QWidget;
            //QHBoxLayout* chkLay = new QHBoxLayout(chkContainer);
            //QCheckBox* chk = new QCheckBox;
            //chkLay->addWidget(chk);
            //chkLay->setAlignment(Qt::AlignCenter);
            //chkLay->setContentsMargins(0, 0, 0, 0);
            //chkContainer->setLayout(chkLay);
            //table->setCellWidget(row, 0, chkContainer);

            QTableWidgetItem* chkItem = new QTableWidgetItem();
            chkItem->setCheckState(Qt::Unchecked);
            chkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
            table->setItem(row, 0, chkItem);

            //// 第二列输入一些示例数据
            //QLabel* lbl = new QLabel(QString("2348681234")); // 换行
            //lbl->setAlignment(Qt::AlignCenter);
            //table->setCellWidget(row, 1, lbl);
        }

        int tWidth = table->columnWidth(0);
        for (int index = 0; index < table->columnCount(); index++)
        {
            if (1 == index || 10 == index || 11 == index)
            {
                table->setColumnWidth(index, tWidth * 4/5);
            }
            else
            {
                table->setColumnWidth(index, tWidth/2);
            }
        }

        // 表格深色主题
        table->setStyleSheet(
            "QTableWidget { "
            "background-color: transparent; "
            "color: white; "
            "border: 1px solid rgba(255, 255, 255, 0.2); "
            "border-radius: 6px; "
            "gridline-color: rgba(255, 255, 255, 0.1); "
            "} "
            "QTableWidget::item { "
            "background-color: rgba(255, 255, 255, 0.05); "
            "color: white; "
            "padding: 10px; "
            "border: none; "
            "} "
            "QTableWidget::item:selected { "
            "background-color: rgba(37, 99, 235, 0.3); "
            "} "
            "QHeaderView::section { "
            "background-color: rgba(255, 255, 255, 0.1); "
            "color: white; "
            "font-size: 14px; "
            "font-weight: 500; "
            "padding: 10px; "
            "border: none; "
            "border-bottom: 1px solid rgba(255, 255, 255, 0.2); "
            "}"
        );
        table->horizontalHeader()->setFixedHeight(42);
        table->verticalHeader()->setDefaultSectionSize(42);

        // 主布局
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(12);
        mainLayout->addLayout(btnLayout);
        mainLayout->addWidget(table);
        setLayout(mainLayout);

        // 透明背景，让父窗口的深色背景显示
        setStyleSheet("QWidget { background-color: transparent; }");

        // 连接按钮事件
        connect(btnAdd, &QPushButton::clicked, this, &MemberManagerWidget::onAddMember);
        connect(btnImport, &QPushButton::clicked, this, &MemberManagerWidget::onImport);
        connect(btnExport, &QPushButton::clicked, this, &MemberManagerWidget::onExport);
        connect(btnDelete, &QPushButton::clicked, this, &MemberManagerWidget::onDeleteMember);
        connect(btnGenerate, &QPushButton::clicked, this, &MemberManagerWidget::onGenerate);
    }

private slots:
    void showTip(const QString& title, const QString& message) {
        CustomMessageDialog::showMessage(this, title, message);
    }

    // 添加成员
    void onAddMember() {
        int newRow = table->rowCount();
        table->insertRow(newRow);

        // 第一列加复选框
        QTableWidgetItem* chkItem = new QTableWidgetItem();
        chkItem->setCheckState(Qt::Unchecked);
        chkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        table->setItem(newRow, 0, chkItem);

        QTableWidgetItem* bhItem = new QTableWidgetItem();
        bhItem->setFlags(bhItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(newRow, 1, bhItem);

        // 其他列空白，用户自行填写
        for (int col = 2; col < table->columnCount(); ++col) {
            table->setItem(newRow, col, new QTableWidgetItem(""));
        }

        //QTableWidgetItem* bhItem = new QTableWidgetItem();
        //bhItem->setFlags(bhItem->flags() & ~Qt::ItemIsEditable);
        //QPushButton* pImageBtn = new QPushButton();
        //table->setCellWidget(newRow, table->columnCount() - 1, pImageBtn);
        qDebug() << "添加新成员行:" << newRow;
    }

    // 导出（包含列头）
    void onExport() {
        QString filePath = QFileDialog::getSaveFileName(this, "导出成员数据", "", "CSV文件 (*.csv)");
        if (filePath.isEmpty()) return;

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            showTip("错误", "无法保存文件");
            return;
        }

        QTextStream out(&file);

        // 1. 写入列头
        QStringList headerFields;
        for (int col = 0; col < table->columnCount(); ++col) {
            headerFields << table->horizontalHeaderItem(col)->text();
        }
        out << headerFields.join(",") << "\n";

        // 2. 写入数据
        for (int row = 0; row < table->rowCount(); ++row) {
            QStringList fields;
            for (int col = 0; col < table->columnCount(); ++col) {
                QTableWidgetItem* item = table->item(row, col);
                if (0 == col)
                {
                    if (Qt::Checked == item->checkState())
                    {
                        fields << "1";
                    }
                    else
                    {
                        fields << "";
                    }
                }
                else
                {
                    fields << (item ? item->text() : "");
                }
            }
            out << fields.join(",") << "\n";
        }

        file.close();
        showTip("导出完成", "已导出表头和数据到文件");
    }

    // 导入（包含表头）
    void onImport() {
        QString filePath = QFileDialog::getOpenFileName(this, "导入成员数据", "", "CSV文件 (*.csv)");
        if (filePath.isEmpty()) return;

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            showTip("错误", "无法打开文件");
            return;
        }

        QTextStream in(&file);
        table->setRowCount(0);

        bool isFirstLine = true;
        int rowIndex = 0;

        int invalidIdCount = 0;
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList fields = line.split(",");

            if (isFirstLine) {
                // 第一行是列头
                table->setColumnCount(fields.size());
                table->setHorizontalHeaderLabels(fields);
                isFirstLine = false;
            }
            else {
                if (!isIdCardFormatValid(fields[4]))   //身份证号码格式检查
                {
                    invalidIdCount++;
                    continue;
                }

                // 数据行
                table->insertRow(rowIndex);

                // 第一列加复选框
                QTableWidgetItem* chkItem = new QTableWidgetItem();
                chkItem->setCheckState(fields[0] == "1" ? Qt::Checked : Qt::Unchecked);
                chkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
                table->setItem(rowIndex, 0, chkItem);

                QTableWidgetItem* bhItem = new QTableWidgetItem();
                bhItem->setFlags(bhItem->flags() & ~Qt::ItemIsEditable);
                table->setItem(rowIndex, 1, bhItem);

                for (int col = 2; col < fields.size(); col++) {
                    table->setItem(rowIndex, col, new QTableWidgetItem(fields[col]));
                }
                rowIndex++;
            }
        }

        file.close();
        onGenerate();
        if (invalidIdCount > 0) {
            showTip("提示", QString("导入时有 %1 条记录身份证号码格式不正确，已跳过。").arg(invalidIdCount));
        }
        showTip("导入完成", "已导入表头和数据");
    }

    // 删除选中的行（支持多行选择）
    void onDeleteMember() {
        QList<QTableWidgetSelectionRange> selectedRanges = table->selectedRanges();
        if (selectedRanges.isEmpty()) {
            showTip("删除成员", "请先选择要删除的行");
            return;
        }

        QNetworkAccessManager* manager = new QNetworkAccessManager(this);

        int delCount = 0;
        // 外层选区倒序遍历
        for (int rangeIndex = selectedRanges.size() - 1; rangeIndex >= 0; --rangeIndex) {
            QTableWidgetSelectionRange range = selectedRanges[rangeIndex];
            for (int row = range.bottomRow(); row >= range.topRow(); --row) {
                QTableWidgetItem* pItem = table->item(row, 1);
                QString teacherId = table->item(row, 1)->text(); // 假设第2列是 teacher_unique_id
                if (teacherId.isEmpty())
                {
                    continue;
                }

                // 构造删除请求
                QUrl url("http://47.100.126.194:5000/delete_teacher");
                QNetworkRequest request(url);
                request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

                // JSON body
                QJsonObject json;
                json["teacher_unique_id"] = teacherId;
                QJsonDocument doc(json);

                QNetworkReply* reply = manager->post(request, doc.toJson());
                connect(reply, &QNetworkReply::finished, [=]() {
                    if (reply->error() == QNetworkReply::NoError) {
                        QByteArray response = reply->readAll();
                        qDebug() << "删除教师成功:" << response;
                    }
                    else {
                        qDebug() << "删除教师失败:" << reply->errorString();
                    }
                    reply->deleteLater();
                    });

                table->removeRow(row);
                delCount++;
            }
        }

        // CustomMessageDialog::showMessage(this, "删除成员",
        //    QString("已删除 %1 条记录（已通知服务器）").arg(delCount));
    }

    // 校验身份证格式（不含校验码运算）
    bool isIdCardFormatValid(const QString& idCard) {
        // 18位：前17位数字 + 最后一位数字或X
        QRegularExpression re18("^\\d{17}[\\dXx]$");

        // 15位：全是数字
        QRegularExpression re15("^\\d{15}$");

        return re18.match(idCard).hasMatch() || re15.match(idCard).hasMatch();
    }

    void onGenerate() {
        if (m_schoolId.isEmpty())
        {
            showTip("提示", "需要先生成学校组织代码！");
            return;
        }

        QStringList invalidRows;
        for (int row = 0; row < table->rowCount(); ++row) {
            QString teacherUniqueId = table->item(row, 1) ? table->item(row, 1)->text().trimmed() : "";

            // 如果系统唯一编号不为空，跳过
            if (!teacherUniqueId.isEmpty()) {
                continue;
            }

            if (!isIdCardFormatValid(table->item(row, 4) ? table->item(row, 4)->text() : ""))
            {
                invalidRows << QString::number(row + 1);
                continue;
            }

            // 组织 JSON 数据
            QJsonObject teacherData;
            teacherData["is_Administarator"] = table->item(row, 0) ? (Qt::Checked == table->item(row, 0)->checkState() ? "1" : "0") : "";
            teacherData["name"] = table->item(row, 2) ? table->item(row, 2)->text() : "";
            teacherData["phone"] = table->item(row, 3) ? table->item(row, 3)->text() : "";
            teacherData["id_card"] = table->item(row, 4) ? table->item(row, 4)->text() : "";
            teacherData["sex"] = table->item(row, 5) ? table->item(row, 5)->text() : "";
            teacherData["teaching_tenure"] = table->item(row, 6) ? table->item(row, 6)->text() : "";
            teacherData["education"] = table->item(row, 7) ? table->item(row, 7)->text() : "";
            teacherData["graduation_institution"] = table->item(row, 8) ? table->item(row, 8)->text() : "";
            teacherData["major"] = table->item(row, 9) ? table->item(row, 9)->text() : "";
            teacherData["teacher_certification_level"] = table->item(row, 10) ? table->item(row, 10)->text() : "";
            teacherData["subjects_of_teacher_qualification_examination"] = table->item(row, 11) ? table->item(row, 11)->text() : "";
            teacherData["educational_stage"] = table->item(row, 12) ? table->item(row, 12)->text() : "";

            // ⚠ schoolId 你需要从代码或其他输入获取，这里先假设写死一个
            teacherData["schoolId"] = m_schoolId;
            //teacherData["gradeId"] = 12;

            QJsonDocument doc(teacherData);
            QByteArray jsonData = doc.toJson();

            // 发送 POST 请求
            QNetworkRequest request(QUrl("http://47.100.126.194:5000/add_teacher"));
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

            QNetworkReply* reply = manager_->post(request, jsonData);
            connect(reply, &QNetworkReply::finished, this, [=]() {
                if (reply->error() == QNetworkReply::NoError) {
                    QByteArray response_data = reply->readAll();
                    qDebug() << "Row" << row << "Response:" << response_data;

                    // 你可以在这里解析返回的 JSON，并更新表格的“系统唯一编号”列
                    QJsonDocument respDoc = QJsonDocument::fromJson(response_data);
                    if (respDoc.isObject()) {
                        QJsonObject respObj = respDoc.object();
                        QJsonObject dataObj = respObj.value("data").toObject();
                        QJsonObject teacherObj = dataObj.value("teacher").toObject();
                        //QString newUniqueId = QString::number(teacherObj.value("teacher_unique_id").toInt());
                        QString newUniqueId = teacherObj.value("teacher_unique_id").toString();
                        table->setItem(row, 1, new QTableWidgetItem(newUniqueId));
                    }

                }
                else {
                    QString qErr = reply->errorString();
                    qDebug() << "Row" << row << "Error:" << qErr;
                }
                reply->deleteLater();
                });
        }

        if (!invalidRows.isEmpty()) {
            showTip("提示", QString("以下行身份证号码格式不正确，已跳过生成：%1").arg(invalidRows.join("、")));
        }
    }

 private:
    void fetchTeachers(const QString& schoolId) {
        QString url = QString("http://47.100.126.194:5000/get_list_teachers?schoolId=%1")
            .arg(schoolId);
        QNetworkRequest request_get;
        request_get.setUrl(QUrl(url));

        QNetworkReply* reply = manager_->get(request_get);
        connect(reply, &QNetworkReply::finished, this, [=]() {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray responseData = reply->readAll();
                qDebug() << "Response:" << responseData;

                QJsonDocument doc = QJsonDocument::fromJson(responseData);
                if (!doc.isObject()) {
                    qDebug() << "Invalid JSON format";
                    reply->deleteLater();
                    return;
                }

                QJsonObject obj = doc.object();
                QJsonObject dataObj = obj.value("data").toObject();
                QJsonArray teachersArray = dataObj.value("teachers").toArray();

                // 设置表格行数为返回数组大小
                table->setRowCount(teachersArray.size());
                table->setColumnCount(14); // 按你的表格列数

                table->setHorizontalHeaderLabels({
                    "管理员", "系统唯一编号", "姓名", "电话", "身份证", "性别", "教龄", "学历",
                    "毕业院校", "专业", "教师资格证学段", "教师资格证科目", "学段", ""
                    });

                // 填充表格
                for (int i = 0; i < teachersArray.size(); ++i) {
                    QJsonObject teacherObj = teachersArray.at(i).toObject();

                    // 第一列加复选框
                    QTableWidgetItem* chkItem = new QTableWidgetItem();
                    chkItem->setCheckState(teacherObj.value("is_Administarator").toInt() == 0 ? Qt::Unchecked:Qt::Checked);
                    chkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
                    //table->setItem(i, 0, chkItem);

                    //QTableWidgetItem* bhItem = new QTableWidgetItem(QString::number(teacherObj.value("teacher_unique_id").toInt()));
                    QTableWidgetItem* bhItem = new QTableWidgetItem(teacherObj.value("teacher_unique_id").toString());
                    bhItem->setFlags(bhItem->flags() & ~Qt::ItemIsEditable);
                    //table->setItem(i, 1, bhItem);

                    table->setItem(i, 0, chkItem);
                    table->setItem(i, 1, bhItem);
                    table->setItem(i, 2, new QTableWidgetItem(teacherObj.value("name").toString()));
                    table->setItem(i, 3, new QTableWidgetItem(teacherObj.value("phone").toString()));
                    table->setItem(i, 4, new QTableWidgetItem(teacherObj.value("id_card").toString()));
                    table->setItem(i, 5, new QTableWidgetItem(teacherObj.value("sex").toString()));
                    table->setItem(i, 6, new QTableWidgetItem(teacherObj.value("teaching_tenure").toString()));
                    table->setItem(i, 7, new QTableWidgetItem(teacherObj.value("education").toString()));
                    table->setItem(i, 8, new QTableWidgetItem(teacherObj.value("graduation_institution").toString()));
                    table->setItem(i, 9, new QTableWidgetItem(teacherObj.value("major").toString()));
                    table->setItem(i, 10, new QTableWidgetItem(teacherObj.value("teacher_certification_level").toString()));
                    table->setItem(i, 11, new QTableWidgetItem(teacherObj.value("subjects_of_teacher_qualification_examination").toString()));
                    table->setItem(i, 12, new QTableWidgetItem(teacherObj.value("educational_stage").toString()));
                    table->setItem(i, 13, new QTableWidgetItem(""));  // 空列
                }

                table->resizeColumnsToContents(); // 自动适应列宽
                table->horizontalHeader()->setStretchLastSection(true);
            }
            else {
                qDebug() << "Network error:" << reply->errorString();
            }
            reply->deleteLater();
            });
    }

public:
    void InitData(UserInfo userInfo)
    {
        m_userInfo = userInfo;

        //if (m_httpHandler)
        //{
        //    QString qUrl("http://47.100.126.194:5000/schools?name=");
        //    qUrl += m_userInfo.strSchoolName;
        //    m_httpHandler->get(qUrl);
        //}
    }

    void setSchoolId(QString schoolId)
    {
        m_schoolId = schoolId;
        fetchTeachers(m_schoolId);
    }

private:
    QNetworkAccessManager* manager_; // 必须声明并初始化
    QPushButton* btnAdd = NULL;
    QPushButton* btnImport = NULL;
    QPushButton* btnExport = NULL;
    QPushButton* btnDelete = NULL;
    QPushButton* btnGenerate = NULL;
    QTableWidget* table;
    UserInfo m_userInfo;
    QString m_schoolId;
};
