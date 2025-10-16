#include "QClassMgr.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QCheckBox>
#include <qmessagebox.h>

QClassMgr::QClassMgr(QWidget *parent)
	: QWidget(parent)
{ // 右侧内容布局
    QVBoxLayout* contentLayout = new QVBoxLayout(this);

    // 上方按钮组合 (学段、年级、班级数)
    //QGridLayout* topGrid = new QGridLayout;
    /*QLabel* lblStage = new QLabel("学段");
    QLabel* lblGrade = new QLabel("年级");
    QLabel* lblClassCount = new QLabel("班级数");

    QPushButton* btnStage1 = new QPushButton("小学");
    QPushButton* btnStage2 = new QPushButton("初中");
    btnStage1->setStyleSheet("background-color:blue; color:white;");
    btnStage2->setStyleSheet("background-color:blue; color:white;");

    QPushButton* btnGrade1 = new QPushButton("1");
    QPushButton* btnGrade2 = new QPushButton("2");
    QPushButton* btnAddGrade = new QPushButton("+");
    btnGrade1->setStyleSheet("background-color:blue; color:white;");
    btnGrade2->setStyleSheet("background-color:blue; color:white;");
    btnAddGrade->setStyleSheet("background-color:green; color:white;");*/

    //QPushButton* btnClassNum = new QPushButton("5");
    //btnClassNum->setStyleSheet("background-color:blue; color:white;");

    //// 布局放置
    //topGrid->addWidget(lblStage, 0, 0);
    //topGrid->addWidget(btnStage1, 0, 1);
    //topGrid->addWidget(btnStage2, 0, 2);

    //topGrid->addWidget(lblGrade, 1, 0);
    //topGrid->addWidget(btnGrade1, 1, 1);
    //topGrid->addWidget(btnGrade2, 1, 2);
    //topGrid->addWidget(btnAddGrade, 1, 3);

    //topGrid->addWidget(lblClassCount, 2, 0);
    //topGrid->addWidget(btnClassNum, 2, 1);

    m_httpHandler = new TAHttpHandler(this);
    if (m_httpHandler)
    {
        connect(m_httpHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
            //成功消息就不发送了
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseString.toUtf8());
            if (jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                if (obj["data"].isObject())
                {
                    QJsonObject oTmp = obj["data"].toObject();
                    QString strTmp = oTmp["message"].toString();
                    //QString strUserId = oTmp["user_id"].toString();
                    qDebug() << "status:" << oTmp["code"].toString();
                    qDebug() << "msg:" << oTmp["message"].toString(); // 如果 msg 是中文，也能正常输出
                    if (strTmp == "获取学校列表成功")
                    {
                        if (oTmp["schools"].isArray())
                        {
                            QJsonArray schoolArr = oTmp["schools"].toArray();
                            if (schoolArr.size() > 0)
                            {
                                if (schoolArr[0].isObject())
                                {
                                    QJsonObject schoolSub = schoolArr[0].toObject();
                                }
                            }
                        }
                    }
                    //errLabel->setText(strTmp);
                }
                else
                {
                }
            }
            else
            {
            }
        });

        connect(m_httpHandler, &TAHttpHandler::failed, this, [=](const QString& errResponseString) {
            //if (errLabel)
            {
                QJsonDocument jsonDoc = QJsonDocument::fromJson(errResponseString.toUtf8());
                if (jsonDoc.isObject()) {
                    QJsonObject obj = jsonDoc.object();
                    if (obj["data"].isObject())
                    {
                        QJsonObject oTmp = obj["data"].toObject();
                        QString strTmp = oTmp["message"].toString();
                        qDebug() << "status:" << oTmp["code"].toString();
                        qDebug() << "msg:" << oTmp["message"].toString(); // 如果 msg 是中文，也能正常输出
                        //errLabel->setText(strTmp);
                        //errLabel->show();
                    }
                }
                else
                {
                    //errLabel->setText("网络错误");
                    //errLabel->show();
                }
            }
            });
    }

    QPushButton* btnGenerate = new QPushButton("生成");
    btnGenerate->setStyleSheet("background-color:blue; color:white; font-size:16px;");
    //topGrid->addWidget(btnGenerate, 0, 4, 3, 1);

    //contentLayout->addLayout(topGrid);
    //contentLayout->addWidget(btnGenerate);

    // 添加、删除按钮
    QHBoxLayout* btnLayout = new QHBoxLayout;
    QPushButton* btnAdd = new QPushButton("添加");
    QPushButton* btnDelete = new QPushButton("删除");
    btnAdd->setStyleSheet("background-color:green; color:white; font-size:16px;");
    btnDelete->setStyleSheet("background-color:green; color:white; font-size:16px;");
    btnLayout->addStretch();
    btnLayout->addWidget(btnAdd);
    btnLayout->addStretch();
    btnLayout->addWidget(btnDelete);
    btnLayout->addStretch();
    btnLayout->addWidget(btnGenerate);
    btnLayout->addStretch();
    contentLayout->addLayout(btnLayout);

	connect(btnGenerate, &QPushButton::clicked, this, [=] {
        if (m_httpHandler)
        {
            QJsonArray jsonArray;
            for (int row = 0; row < table_->rowCount(); ++row) {
                QTableWidgetItem* itemSchoolStage = table_->item(row, 0);
                QTableWidgetItem* itemGrade = table_->item(row, 1);
                QTableWidgetItem* itemClassName = table_->item(row, 2);
                QTableWidgetItem* itemClassCode = table_->item(row, 3);
                QTableWidgetItem* itemRemark = table_->item(row, 4);

                if (!itemClassCode || itemClassCode->text().isEmpty()) {
                    // 班级编号是主键，必须有，跳过没编号的行
                    continue;
                }

                QJsonObject obj;
                obj["school_stage"] = itemSchoolStage ? itemSchoolStage->text() : "";
                obj["grade"] = itemGrade ? itemGrade->text() : "";
                obj["class_name"] = itemClassName ? itemClassName->text() : "";
                obj["class_code"] = itemClassCode->text();
                obj["remark"] = itemRemark ? itemRemark->text() : "";
                jsonArray.append(obj);
            }

            if (jsonArray.isEmpty()) {
                qDebug() << "没有数据可上传";
                return;
            }

            QJsonDocument jsonDoc(jsonArray);
            QByteArray jsonData = jsonDoc.toJson();
            m_httpHandler->post(QString("http://47.100.126.194:5000/updateClasses"), jsonData);
        }
	});

    connect(btnAdd, &QPushButton::clicked, this, [=] {
        if (m_schoolId.isEmpty())
        {
            QMessageBox::information(this, "提示", "需要先获取组织代码！");
            return;
        }

        int insertPos = -1;
        // 获取选中的行（只取第一个选中）
        QList<QTableWidgetSelectionRange> ranges = table_->selectedRanges();
        if (!ranges.isEmpty()) {
            insertPos = ranges.first().bottomRow(); // 获取选中块的最后一行
        }

        if (insertPos >= 0) {
            // 在选中行的下一行插入
            table_->insertRow(insertPos + 1);
            fillRow(insertPos + 1);
        }
        else {
            // 没有选中任何行 -> 在末尾追加
            int lastRow = table_->rowCount();
            table_->insertRow(lastRow);
            fillRow(lastRow);
        }
    });

    connect(btnDelete, &QPushButton::clicked, this, [=] {
        // 从最后一行往前删，避免索引混乱
        for (int row = table_->rowCount() - 1; row >= 0; --row)
        {
            QTableWidgetItem* item = table_->item(row, 0); // 第一列复选框项
            if (!item) continue;
            if (item->checkState() == Qt::Checked)
            {
                table_->removeRow(row);
            }
        }
    });

    // 表格
    table_ = new QTableWidget(0, 5);
    table_->setHorizontalHeaderLabels({ "学段","年级","班级","班级编号","备注" });

    // 第一列加复选框
    for (int row = 0; row < table_->rowCount(); ++row) {
        //QCheckBox* chk = new QCheckBox;
        //table->setCellWidget(row, 0, chk);

        QTableWidgetItem* item = new QTableWidgetItem();
        item->setCheckState(Qt::Unchecked);                  // 初始为未勾选
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable); // 允许勾选
        table_->setItem(row, 0, item);
    }
    // 隐藏行号
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    contentLayout->addWidget(table_);

    // 将左菜单和右内容加到主布局
    //mainLayout->addLayout(menuLayout);
    //mainLayout->addLayout(contentLayout);
}

QClassMgr::~QClassMgr()
{

}

void QClassMgr::initData()
{
    if (m_schoolId.isEmpty())
    {
        return;
    }

    QString prefix = m_schoolId;
    if (prefix.length() != 6 || !prefix.toInt()) {
        QMessageBox::warning(this, "错误", "请输入6位数字前缀");
        return;
    }

    // 构造请求 JSON
    QJsonObject jsonObj;
    jsonObj["prefix"] = prefix;
    QJsonDocument doc(jsonObj);
    QByteArray reqData = doc.toJson(QJsonDocument::Compact);

    // 网络请求
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl("http://47.100.126.194:5000/getClassesByPrefix")); // 改成你的接口地址
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = manager->post(request, reqData);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response_data = reply->readAll();
            QJsonDocument respDoc = QJsonDocument::fromJson(response_data);

            if (respDoc.isObject()) {
                QJsonObject root = respDoc.object();
                QJsonObject dataObj = root.value("data").toObject();
                int code = dataObj.value("code").toInt();
                QString message = dataObj.value("message").toString();

                if (code != 200) {
                    QMessageBox::warning(this, "查询失败", message);
                    return;
                }

                QJsonArray classes = dataObj.value("classes").toArray();
                table_->setRowCount(0); // 清空旧数据

                for (const QJsonValue& val : classes) {
                    QJsonObject cls = val.toObject();
                    int row = table_->rowCount();
                    table_->insertRow(row);

                    QTableWidgetItem* chkItem = new QTableWidgetItem(cls.value("school_stage").toString());
                    chkItem->setCheckState(Qt::Unchecked);
                    chkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
                    table_->setItem(row, 0, chkItem);
                    table_->setItem(row, 1, new QTableWidgetItem(cls.value("grade").toString()));
                    table_->setItem(row, 2, new QTableWidgetItem(cls.value("class_name").toString()));

                    QTableWidgetItem* bhItem = new QTableWidgetItem(cls.value("class_code").toString());
                    bhItem->setFlags(bhItem->flags() & ~Qt::ItemIsEditable);
                    table_->setItem(row, 3, bhItem);
                    table_->setItem(row, 4, new QTableWidgetItem(cls.value("remark").toString()));
                    //table_->setItem(row, 5, new QTableWidgetItem(cls.value("created_at").toString()));
                }
                //QMessageBox::information(this, "查询成功",
                //    QString("共查询到 %1 条班级数据").arg(classes.size()));
            }
        }
        else {
            QMessageBox::critical(this, "网络错误", reply->errorString());
        }
        reply->deleteLater();
     });
}

void QClassMgr::setSchoolId(QString schoolId)
{
    m_schoolId = schoolId;
}

void QClassMgr::fillRow(int row)
{
    QString xueduan = "小学";
    QString nianji = "一年级";
    QString banji = "3班";

    // 自动生成班级编号：学段+年级+班级去掉汉字，转数字等
    QString banjiBianhao = generateClassIdAuto(xueduan, nianji);

    // 第一列复选框
    QTableWidgetItem* chkItem = new QTableWidgetItem(xueduan);
    chkItem->setCheckState(Qt::Unchecked);
    chkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    table_->setItem(row, 0, chkItem);

    // 填入各列
    table_->setItem(row, 1, new QTableWidgetItem(nianji));
    table_->setItem(row, 2, new QTableWidgetItem(banji));

    QTableWidgetItem* bhItem = new QTableWidgetItem(banjiBianhao);
    bhItem->setFlags(bhItem->flags() & ~Qt::ItemIsEditable);
    table_->setItem(row, 3, bhItem);
    table_->setItem(row, 4, new QTableWidgetItem());
}

QString QClassMgr::generateClassIdAuto(const QString& xueduan, const QString& nianji)
{
    int maxNum = 0;

    // 遍历表格所有行，找到同学段+年级的最大编号
    for (int r = 0; r < table_->rowCount(); ++r) {
        QTableWidgetItem* sdItem = table_->item(r, 0); // 学段
        QTableWidgetItem* njItem = table_->item(r, 1); // 年级
        QTableWidgetItem* bhItem = table_->item(r, 3); // 班级编号

        if (!sdItem || !njItem || !bhItem) continue;

        if (sdItem->text() == xueduan && njItem->text() == nianji) {
            bool ok = false;
            int num = bhItem->text().toInt(&ok);
            if (ok && num > maxNum) {
                maxNum = num;
            }
        }
    }

    // 新编号 = 最大编号 + 1
    int newNum = maxNum + 1;

    int lastThree = newNum % 1000; // 取最后三位

    // 如果需要固定6位（例如 000025）
    return m_schoolId + QString("%1").arg(lastThree, 3, 10, QChar('0'));
}
