#include "QClassMgr.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QCheckBox>
#include <QIcon>
#include <qmessagebox.h>
#include <algorithm>
#include <QSet>

QClassMgr::QClassMgr(QWidget *parent)
	: QWidget(parent)
{ 
    // 深色主题布局
    QVBoxLayout* contentLayout = new QVBoxLayout(this);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(12);
    
    // 设置透明背景
    setStyleSheet("QWidget { background-color: transparent; }");

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
                    int code = oTmp["code"].toInt();
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
                    else if (strTmp == "批量插入/更新完成" && code == 200)
                    {
                        // 处理 updateClasses 接口返回的班级列表
                        if (oTmp["classes"].isArray())
                        {
                            QJsonArray classes = oTmp["classes"].toArray();
                            
                            // 遍历返回的班级列表
                            for (const QJsonValue& val : classes) {
                                if (!val.isObject()) continue;
                                
                                QJsonObject cls = val.toObject();
                                QString classCode = cls.value("class_code").toString();
                                QString schoolStage = cls.value("school_stage").toString();
                                QString grade = cls.value("grade").toString();
                                QString className = cls.value("class_name").toString();
                                
                                // 在表格中查找匹配的行（学段、年级、班级名称相同，且班级编号为空）
                                for (int row = 0; row < table_->rowCount(); ++row) {
                                    QTableWidgetItem* itemSchoolStage = table_->item(row, 1);  // 学段在第二列
                                    QTableWidgetItem* itemGrade = table_->item(row, 2);  // 年级在第三列
                                    QTableWidgetItem* itemClassName = table_->item(row, 3);  // 班级在第四列
                                    QTableWidgetItem* itemClassCode = table_->item(row, 4);  // 班级编号在第五列
                                    
                                    // 检查是否匹配：学段、年级、班级名称相同，且班级编号为空
                                    if (itemSchoolStage && itemSchoolStage->text() == schoolStage &&
                                        itemGrade && itemGrade->text() == grade &&
                                        itemClassName && itemClassName->text() == className &&
                                        (!itemClassCode || itemClassCode->text().isEmpty())) {
                                        
                                        // 更新班级编号
                                        if (!itemClassCode) {
                                            itemClassCode = new QTableWidgetItem(classCode);
                                            itemClassCode->setFlags(itemClassCode->flags() & ~Qt::ItemIsEditable);
                                            table_->setItem(row, 4, itemClassCode);  // 班级编号在第五列
                                        } else {
                                            itemClassCode->setText(classCode);
                                        }
                                        break; // 找到匹配的行后跳出循环
                                    }
                                }
                            }
                            
                            QMessageBox::information(this, "成功", QString("已更新 %1 个班级的编号").arg(classes.size()));
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

    // 顶部操作栏：添加、删除、生成按钮（带图标）
    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(12);
    
    QPushButton* btnAdd = new QPushButton("添加");
    btnAdd->setIcon(QIcon(":/res/img/com_card_ic_add@3x.png"));
    btnAdd->setIconSize(QSize(22, 22));
    btnAdd->setFixedHeight(40);
    btnAdd->setStyleSheet(
        "QPushButton { "
        "background-color: rgba(255, 255, 255, 0.1); "
        "color: white; "
        "font-size: 14px; "
        "border: 1px solid rgba(255, 255, 255, 0.2); "
        "border-radius: 6px; "
        "padding: 6px 14px; "
        "} "
        "QPushButton:hover { "
        "background-color: rgba(255, 255, 255, 0.15); "
        "}");
    
    QPushButton* btnDelete = new QPushButton("删除");
    btnDelete->setIcon(QIcon(":/res/img/com_card_ic_del@3x.png"));
    btnDelete->setIconSize(QSize(22, 22));
    btnDelete->setFixedHeight(40);
    btnDelete->setStyleSheet(
        "QPushButton { "
        "background-color: rgba(255, 255, 255, 0.1); "
        "color: white; "
        "font-size: 14px; "
        "border: 1px solid rgba(255, 255, 255, 0.2); "
        "border-radius: 6px; "
        "padding: 6px 14px; "
        "} "
        "QPushButton:hover { "
        "background-color: rgba(255, 255, 255, 0.15); "
        "}");
    
    QPushButton* btnGenerate = new QPushButton("生成");
    btnGenerate->setIcon(QIcon(":/res/img/com_card_ic_generate@3x.png"));
    btnGenerate->setIconSize(QSize(22, 22));
    btnGenerate->setFixedHeight(40);
    btnGenerate->setStyleSheet(
        "QPushButton { "
        "background-color: rgba(255, 255, 255, 0.1); "
        "color: white; "
        "font-size: 14px; "
        "border: 1px solid rgba(255, 255, 255, 0.2); "
        "border-radius: 6px; "
        "padding: 6px 14px; "
        "} "
        "QPushButton:hover { "
        "background-color: rgba(255, 255, 255, 0.15); "
        "}");
    
    btnLayout->addWidget(btnAdd);
    btnLayout->addWidget(btnDelete);
    btnLayout->addStretch();
    btnLayout->addWidget(btnGenerate);
    contentLayout->addLayout(btnLayout);

	connect(btnGenerate, &QPushButton::clicked, this, [=] {
        if (m_httpHandler)
        {
            QJsonArray jsonArray;
            for (int row = 0; row < table_->rowCount(); ++row) {
                QTableWidgetItem* itemSchoolStage = table_->item(row, 1);  // 学段在第二列
                QTableWidgetItem* itemGrade = table_->item(row, 2);  // 年级在第三列
                QTableWidgetItem* itemClassName = table_->item(row, 3);  // 班级在第四列
                QTableWidgetItem* itemClassCode = table_->item(row, 4);  // 班级编号在第五列

                // 只上传班级编号为空的行
                if (itemClassCode && !itemClassCode->text().isEmpty()) {
                    continue; // 跳过有班级编号的行
                }

                // 检查必填字段
                if (!itemSchoolStage || itemSchoolStage->text().isEmpty() ||
                    !itemGrade || itemGrade->text().isEmpty() ||
                    !itemClassName || itemClassName->text().isEmpty()) {
                    continue; // 跳过必填字段为空的行
                }

                QJsonObject obj;
                obj["school_stage"] = itemSchoolStage->text();
                obj["grade"] = itemGrade->text();
                obj["class_name"] = itemClassName->text();
                obj["schoolid"] = m_schoolId; // 添加学校ID
                // 不再包含 class_code 字段
                // 备注列已移除，不再包含在表格中
                jsonArray.append(obj);
            }

            if (jsonArray.isEmpty()) {
                QMessageBox::information(this, "提示", "没有数据可上传！");
                return;
            }

            // 检查重复：要上传的班级列表内部是否有重复
            QSet<QString> uploadClassKeys; // 用于检查上传列表内部的重复
            for (int i = 0; i < jsonArray.size(); ++i) {
                QJsonObject obj = jsonArray[i].toObject();
                QString key = obj["school_stage"].toString() + "|" + 
                             obj["grade"].toString() + "|" + 
                             obj["class_name"].toString();
                
                if (uploadClassKeys.contains(key)) {
                    QMessageBox::warning(this, "错误", 
                        QString("要上传的班级列表中存在重复：\n学段：%1\n年级：%2\n班级：%3\n\n请修改后再上传！")
                        .arg(obj["school_stage"].toString())
                        .arg(obj["grade"].toString())
                        .arg(obj["class_name"].toString()));
                    return;
                }
                uploadClassKeys.insert(key);
            }

            // 检查重复：要上传的班级与表格中已有班级编号的行是否有重复
            for (int i = 0; i < jsonArray.size(); ++i) {
                QJsonObject obj = jsonArray[i].toObject();
                QString uploadSchoolStage = obj["school_stage"].toString();
                QString uploadGrade = obj["grade"].toString();
                QString uploadClassName = obj["class_name"].toString();
                
                // 检查表格中所有有班级编号的行
                for (int row = 0; row < table_->rowCount(); ++row) {
                    QTableWidgetItem* itemClassCode = table_->item(row, 4);  // 班级编号在第五列
                    // 只检查有班级编号的行
                    if (!itemClassCode || itemClassCode->text().isEmpty()) {
                        continue;
                    }
                    
                    QTableWidgetItem* itemSchoolStage = table_->item(row, 1);  // 学段在第二列
                    QTableWidgetItem* itemGrade = table_->item(row, 2);  // 年级在第三列
                    QTableWidgetItem* itemClassName = table_->item(row, 3);  // 班级在第四列
                    
                    if (itemSchoolStage && itemSchoolStage->text() == uploadSchoolStage &&
                        itemGrade && itemGrade->text() == uploadGrade &&
                        itemClassName && itemClassName->text() == uploadClassName) {
                        QMessageBox::warning(this, "错误", 
                            QString("要上传的班级与表格中已有的班级重复：\n学段：%1\n年级：%2\n班级：%3\n\n请修改后再上传！")
                            .arg(uploadSchoolStage)
                            .arg(uploadGrade)
                            .arg(uploadClassName));
                        return;
                    }
                }
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
        if (!m_httpHandler) {
            return;
        }

        // 收集勾选复选框且有班级编号的行
        QJsonArray jsonArray;
        QList<int> rowsToDelete;
        
        for (int row = 0; row < table_->rowCount(); ++row) {
            QTableWidgetItem* chkItem = table_->item(row, 0); // 第一列复选框
            QTableWidgetItem* itemClassCode = table_->item(row, 4); // 班级编号在第五列
            
            // 检查复选框是否被勾选
            if (!chkItem || chkItem->checkState() != Qt::Checked) {
                continue; // 跳过未勾选的行
            }
            
            // 只删除有班级编号的班级
            if (!itemClassCode || itemClassCode->text().isEmpty()) {
                QMessageBox::warning(this, "提示", QString("第 %1 行没有班级编号，无法删除！").arg(row + 1));
                continue; // 跳过没有班级编号的行
            }
            
            // 收集要删除的班级数据
            QTableWidgetItem* itemSchoolStage = table_->item(row, 1);  // 学段在第二列
            QTableWidgetItem* itemGrade = table_->item(row, 2);  // 年级在第三列
            QTableWidgetItem* itemClassName = table_->item(row, 3);  // 班级在第四列
            
            QJsonObject obj;
            obj["class_code"] = itemClassCode->text();
            obj["schoolid"] = m_schoolId; // 添加学校ID
            if (itemSchoolStage) {
                obj["school_stage"] = itemSchoolStage->text();
            }
            if (itemGrade) {
                obj["grade"] = itemGrade->text();
            }
            if (itemClassName) {
                obj["class_name"] = itemClassName->text();
            }
            // 备注列已移除，不再包含在表格中
            jsonArray.append(obj);
            rowsToDelete.append(row);
        }
        
        if (jsonArray.isEmpty()) {
            QMessageBox::information(this, "提示", "请先勾选要删除的班级（必须有班级编号）！");
            return;
        }
        
        // 上传到服务器
        QJsonDocument jsonDoc(jsonArray);
        QByteArray jsonData = jsonDoc.toJson();
        
        // 发送删除请求
        QNetworkAccessManager* manager = new QNetworkAccessManager(this);
        QNetworkRequest request(QUrl("http://47.100.126.194:5000/deleteClasses"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        
        QNetworkReply* reply = manager->post(request, jsonData);
        
        connect(reply, &QNetworkReply::finished, this, [this, reply, rowsToDelete]() {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray response_data = reply->readAll();
                QJsonDocument respDoc = QJsonDocument::fromJson(response_data);
                
                if (respDoc.isObject()) {
                    QJsonObject root = respDoc.object();
                    if (!root.contains("data") || !root["data"].isObject()) {
                        QMessageBox::warning(this, "错误", "服务器返回格式错误：缺少data字段");
                        reply->deleteLater();
                        return;
                    }
                    
                    QJsonObject dataObj = root["data"].toObject();
                    int code = dataObj.value("code").toInt();
                    QString message = dataObj.value("message").toString();
                    
                    if (code == 200) {
                        // 获取服务器返回的已删除班级编号列表
                        QJsonArray deletedCodes = dataObj.value("deleted_codes").toArray();
                        QSet<QString> deletedCodeSet;
                        for (const QJsonValue& val : deletedCodes) {
                            deletedCodeSet.insert(val.toString());
                        }
                        
                        // 根据返回的班级编号列表，从表格中删除对应的行
                        QList<int> rowsToRemove;
                        for (int row = 0; row < table_->rowCount(); ++row) {
                            QTableWidgetItem* itemClassCode = table_->item(row, 4);  // 班级编号在第五列
                            if (itemClassCode && deletedCodeSet.contains(itemClassCode->text())) {
                                rowsToRemove.append(row);
                            }
                        }
                        
                        // 从后往前删除，避免索引混乱
                        std::sort(rowsToRemove.begin(), rowsToRemove.end());
                        for (int i = rowsToRemove.size() - 1; i >= 0; --i) {
                            table_->removeRow(rowsToRemove[i]);
                        }
                        
                        int deletedCount = dataObj.value("deleted_count").toInt();
                        QMessageBox::information(this, "成功", QString("删除班级成功！共删除 %1 个班级").arg(deletedCount));
                    } else {
                        QMessageBox::warning(this, "删除失败", message);
                    }
                } else {
                    QMessageBox::warning(this, "错误", "服务器返回格式错误：不是有效的JSON对象");
                }
            } else {
                QMessageBox::critical(this, "网络错误", reply->errorString());
            }
            reply->deleteLater();
        });
    });

    // 表格 - 深色主题样式
    table_ = new QTableWidget(0, 5);
    table_->setHorizontalHeaderLabels({ "", "学段", "年级", "班级", "班级编号" });  // 第一列为复选框列，无标题
    
    // 表格样式
    table_->setStyleSheet(
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
        "padding: 12px; "
        "border: none; "
        "} "
        "QTableWidget::item:selected { "
        "background-color: rgba(37, 99, 235, 0.3); "
        "} "
        "QHeaderView::section { "
        "background-color: rgba(255, 255, 255, 0.1); "
        "color: white; "
        "font-size: 16px; "
        "font-weight: 500; "
        "padding: 12px; "
        "border: none; "
        "border-bottom: 1px solid rgba(255, 255, 255, 0.2); "
        "}");
    
    // 隐藏行号
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setStretchLastSection(false);
    table_->setColumnWidth(0, 40);  // 复选框列固定宽度
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setAlternatingRowColors(false);
    // 列表框整体放大（约 1.5 倍效果：高度/行高/表头）
    table_->setMinimumHeight(450);
    table_->horizontalHeader()->setFixedHeight(48);
    table_->verticalHeader()->setDefaultSectionSize(48);
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

                    // 第一列：复选框
                    QTableWidgetItem* chkItem = new QTableWidgetItem();
                    chkItem->setCheckState(Qt::Unchecked);
                    chkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
                    // 注意：不要同时设置 checkState + icon，否则会出现“复选框 + 图标”错位叠加
                    chkItem->setText("");
                    chkItem->setTextAlignment(Qt::AlignCenter);
                    table_->setItem(row, 0, chkItem);
                    
                    // 第二列：学段
                    QTableWidgetItem* stageItem = new QTableWidgetItem(cls.value("school_stage").toString());
                    stageItem->setFlags(stageItem->flags() | Qt::ItemIsEditable);
                    table_->setItem(row, 1, stageItem);
                    
                    // 第三列：年级
                    QTableWidgetItem* gradeItem = new QTableWidgetItem(cls.value("grade").toString());
                    gradeItem->setFlags(gradeItem->flags() | Qt::ItemIsEditable);
                    table_->setItem(row, 2, gradeItem);
                    
                    // 第四列：班级
                    QTableWidgetItem* classItem = new QTableWidgetItem(cls.value("class_name").toString());
                    classItem->setFlags(classItem->flags() | Qt::ItemIsEditable);
                    table_->setItem(row, 3, classItem);

                    // 第五列：班级编号（只读）
                    QTableWidgetItem* bhItem = new QTableWidgetItem(cls.value("class_code").toString());
                    bhItem->setFlags(bhItem->flags() & ~Qt::ItemIsEditable);
                    table_->setItem(row, 4, bhItem);
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
    // 表格列定义：
    // 0: 复选框（无文本）
    // 1: 学段
    // 2: 年级
    // 3: 班级
    // 4: 班级编号（只读）

    const QString xueduan = "小学";
    const QString nianji = "一年级";
    const QString banji = "1班";

    // 第一列：复选框（只用 checkState，避免与 icon 叠加错位）
    QTableWidgetItem* chkItem = new QTableWidgetItem();
    chkItem->setText("");
    chkItem->setTextAlignment(Qt::AlignCenter);
    chkItem->setCheckState(Qt::Unchecked);
    chkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    table_->setItem(row, 0, chkItem);

    // 第二列：学段
    table_->setItem(row, 1, new QTableWidgetItem(xueduan));
    // 第三列：年级
    table_->setItem(row, 2, new QTableWidgetItem(nianji));
    // 第四列：班级
    table_->setItem(row, 3, new QTableWidgetItem(banji));

    // 第五列：班级编号（只读，为空）
    QTableWidgetItem* codeItem = new QTableWidgetItem("");
    codeItem->setFlags(codeItem->flags() & ~Qt::ItemIsEditable);
    table_->setItem(row, 4, codeItem);
}

QString QClassMgr::generateClassIdAuto(const QString& xueduan, const QString& nianji)
{
    int maxNum = 0;

    // 遍历表格所有行，找到同学段+年级的最大编号
    for (int r = 0; r < table_->rowCount(); ++r) {
        QTableWidgetItem* sdItem = table_->item(r, 1); // 学段
        QTableWidgetItem* njItem = table_->item(r, 2); // 年级
        QTableWidgetItem* bhItem = table_->item(r, 4); // 班级编号

        if (!sdItem || !njItem || !bhItem) continue;

        if (sdItem->text() == xueduan && njItem->text() == nianji) {
            // 班级编号形如：<学校id><后三位流水>，取后三位做自增
            bool ok = false;
            const QString code = bhItem->text().trimmed();
            const int num = (code.size() >= 3) ? code.right(3).toInt(&ok) : 0;
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
