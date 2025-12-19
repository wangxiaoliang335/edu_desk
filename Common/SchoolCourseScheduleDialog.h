#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <QDate>
#include <QMap>
#include <QSet>
#include <QRegExp>
#include <QSignalBlocker>
#include <QMessageBox>

#include "CommonInfo.h"

// 学校课程表总览：按年级查看该年级所有班级的课程表（数据源：/getClassesByPrefix + /course-schedule）
class SchoolCourseScheduleDialog : public QDialog
{
public:
    explicit SchoolCourseScheduleDialog(QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(QString::fromUtf8(u8"年级课程表"));
        resize(1280, 800);
        setStyleSheet(
            "QDialog{background-color:#2f2f2f;color:#eaeaea;}"
            "QLabel{color:#eaeaea;}"
            "QComboBox{background-color:#3b3b3b;color:#eaeaea;border:1px solid #555;padding:6px 10px;border-radius:6px;}"
            "QPushButton{background-color:#3b3b3b;color:#eaeaea;border:1px solid #555;padding:6px 10px;border-radius:6px;}"
            "QPushButton:hover{background-color:#454545;}"
            "QTableWidget{background-color:#2b2b2b;color:#eaeaea;gridline-color:#444;}"
            "QHeaderView::section{background-color:#3a3a3a;color:#eaeaea;border:1px solid #444;padding:6px;}"
        );

        m_manager = new QNetworkAccessManager(this);

        QVBoxLayout* root = new QVBoxLayout(this);
        root->setContentsMargins(12, 12, 12, 12);
        root->setSpacing(10);

        // 顶部栏：标题 + 年级下拉 + 刷新 + 关闭
        QHBoxLayout* top = new QHBoxLayout;
        top->setSpacing(10);
        QLabel* title = new QLabel(QString::fromUtf8(u8"课程表"), this);
        title->setStyleSheet("font-size:16px;font-weight:700;");
        top->addWidget(title);
        top->addStretch();

        m_gradeCombo = new QComboBox(this);
        m_gradeCombo->setMinimumWidth(180);
        top->addWidget(m_gradeCombo);

        // 预留：上传/下载（暂不实现）
        QPushButton* downloadBtn = new QPushButton(QString::fromUtf8(u8"下载"), this);
        QPushButton* uploadBtn = new QPushButton(QString::fromUtf8(u8"上传"), this);
        top->addWidget(downloadBtn);
        top->addWidget(uploadBtn);

        m_refreshBtn = new QPushButton(QString::fromUtf8(u8"刷新"), this);
        top->addWidget(m_refreshBtn);

        QPushButton* closeBtn = new QPushButton(QString::fromUtf8(u8"关闭"), this);
        top->addWidget(closeBtn);
        root->addLayout(top);

        // 主体：表格
        m_table = new QTableWidget(this);
        m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
        m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        m_table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        m_table->setWordWrap(true);
        root->addWidget(m_table, 1);

        // 状态栏
        m_status = new QLabel(QString::fromUtf8(u8"就绪"), this);
        m_status->setStyleSheet("color:#cfcfcf;");
        root->addWidget(m_status);

        connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
        connect(m_refreshBtn, &QPushButton::clicked, this, [this]() { refresh(); });
        connect(downloadBtn, &QPushButton::clicked, this, [this]() {
            QMessageBox::information(this, QString::fromUtf8(u8"提示"), QString::fromUtf8(u8"下载功能暂未实现"));
        });
        connect(uploadBtn, &QPushButton::clicked, this, [this]() {
            QMessageBox::information(this, QString::fromUtf8(u8"提示"), QString::fromUtf8(u8"上传功能暂未实现"));
        });
        connect(m_gradeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            const QString grade = m_gradeCombo->currentData().toString();
            if (!grade.isEmpty()) {
                fetchSchedulesForGrade(grade);
            }
        });
    }

    void refresh()
    {
        m_status->setText(QString::fromUtf8(u8"正在获取班级列表..."));
        fetchClassesForSchool();
    }

private:
    struct ClassInfo {
        QString classId;     // class_code
        QString stage;       // school_stage
        QString grade;       // grade
        QString className;   // class_name
        QString displayName; // 用于表头展示（比如：3班/七(3)班等）
    };

    struct ScheduleData {
        QStringList days;
        QStringList times;
        // row -> col -> course
        QMap<int, QMap<int, QString>> cellText;
        QSet<QPair<int,int>> highlightCells;
        bool filteredFirstColumn = false;
    };

    static QString currentTermString()
    {
        // 与 CourseDialog / ScheduleDialog 逻辑对齐
        const QDate currentDate = QDate::currentDate();
        const int year = currentDate.year();
        const int month = currentDate.month();

        int startYear = year;
        int endYear = year + 1;
        int termIndex = 1;

        if (month >= 9) {
            startYear = year;
            endYear = year + 1;
            termIndex = 1;
        } else if (month >= 3 && month <= 8) {
            startYear = year - 1;
            endYear = year;
            termIndex = 2;
        } else {
            // 1-2月属于上一学年第一学期
            startYear = year - 1;
            endYear = year;
            termIndex = 1;
        }

        return QString("%1-%2-%3").arg(startYear).arg(endYear).arg(termIndex);
    }

    static bool looksLikeTimeText(const QString& text)
    {
        static QRegExp preciseTimePattern(QStringLiteral("^\\d{1,2}:\\d{2}(?::\\d{2}(?:\\.\\d{1,3})?)?$"));
        if (preciseTimePattern.exactMatch(text)) return true;
        QRegExp rangePattern(QStringLiteral("^(\\d{1,2}:\\d{2})\\s*-\\s*(\\d{1,2}:\\d{2})$"));
        if (rangePattern.exactMatch(text)) return true;
        return false;
    }

    void fetchClassesForSchool()
    {
        const QString schoolId = CommonInfo::GetData().schoolId;
        if (schoolId.trimmed().isEmpty()) {
            m_status->setText(QString::fromUtf8(u8"schoolId 为空，无法获取班级列表"));
            return;
        }
        QString prefix = schoolId.trimmed();
        if (prefix.length() != 6 || !prefix.toInt()) {
            m_status->setText(QString::fromUtf8(u8"schoolId 格式错误，应为6位数字"));
            return;
        }

        QJsonObject req;
        req["prefix"] = prefix;
        const QByteArray body = QJsonDocument(req).toJson(QJsonDocument::Compact);

        QNetworkRequest request(QUrl(QStringLiteral("http://47.100.126.194:5000/getClassesByPrefix")));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QNetworkReply* reply = m_manager->post(request, body);

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> guard(reply);
            if (reply->error() != QNetworkReply::NoError) {
                m_status->setText(QString::fromUtf8(u8"获取班级列表失败：") + reply->errorString());
                return;
            }
            const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (!doc.isObject()) {
                m_status->setText(QString::fromUtf8(u8"班级列表响应解析失败"));
                return;
            }
            const QJsonObject root = doc.object();
            const QJsonObject dataObj = root.value("data").toObject();
            const int code = dataObj.value("code").toInt(-1);
            if (code != 200) {
                m_status->setText(QString::fromUtf8(u8"获取班级列表失败：") + dataObj.value("message").toString());
                return;
            }

            m_classesByGrade.clear();
            const QJsonArray classes = dataObj.value("classes").toArray();
            for (const QJsonValue& v : classes) {
                if (!v.isObject()) continue;
                const QJsonObject cls = v.toObject();
                ClassInfo info;
                info.stage = cls.value("school_stage").toString().trimmed();
                info.grade = cls.value("grade").toString().trimmed();
                info.className = cls.value("class_name").toString().trimmed();
                info.classId = cls.value("class_code").toString().trimmed();
                if (info.classId.isEmpty()) continue;

                // displayName 尽量只显示“X班”；为空则回退 classId
                info.displayName = info.className;
                if (info.displayName.isEmpty()) info.displayName = info.classId;

                const QString gradeKey = !info.grade.isEmpty() ? info.grade : info.stage;
                if (gradeKey.isEmpty()) continue;
                m_classesByGrade[gradeKey].append(info);
            }

            updateGradeCombo();
        });
    }

    void updateGradeCombo()
    {
        m_gradeCombo->blockSignals(true);
        m_gradeCombo->clear();
        QStringList grades = m_classesByGrade.keys();
        grades.sort();
        for (const QString& g : grades) {
            m_gradeCombo->addItem(g, g);
        }
        m_gradeCombo->blockSignals(false);

        if (m_gradeCombo->count() == 0) {
            m_status->setText(QString::fromUtf8(u8"未获取到任何班级"));
            return;
        }

        const QString grade = m_gradeCombo->currentData().toString();
        m_status->setText(QString::fromUtf8(u8"班级列表已更新，选择年级后将拉取课程表"));
        fetchSchedulesForGrade(grade);
    }

    void fetchSchedulesForGrade(const QString& grade)
    {
        if (!m_classesByGrade.contains(grade)) {
            m_status->setText(QString::fromUtf8(u8"年级不存在：") + grade);
            return;
        }

        const QString term = currentTermString();
        m_status->setText(QString::fromUtf8(u8"正在获取课程表：") + grade + QString::fromUtf8(u8"（") + term + QString::fromUtf8(u8"）"));

        const QList<ClassInfo> classes = m_classesByGrade.value(grade);
        if (classes.isEmpty()) {
            m_status->setText(QString::fromUtf8(u8"该年级无班级"));
            return;
        }

        m_pendingGrade = grade;
        m_pendingTerm = term;
        m_pendingSchedules.clear();

        int* pending = new int(classes.size());
        for (const ClassInfo& cls : classes) {
            QUrl url(QStringLiteral("http://47.100.126.194:5000/course-schedule"));
            QUrlQuery q;
            q.addQueryItem(QStringLiteral("class_id"), cls.classId);
            q.addQueryItem(QStringLiteral("term"), term);
            url.setQuery(q);

            QNetworkRequest req(url);
            QNetworkReply* reply = m_manager->get(req);

            connect(reply, &QNetworkReply::finished, this, [this, reply, cls, pending]() {
                const QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> guard(reply);
                if (reply->error() == QNetworkReply::NoError) {
                    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                    if (doc.isObject()) {
                        const QJsonObject root = doc.object();
                        if (root.value("code").toInt(-1) == 200) {
                            const QJsonObject dataObj = root.value("data").toObject();
                            const QJsonObject scheduleObj = dataObj.value(QStringLiteral("schedule")).toObject();
                            ScheduleData sd;
                            sd.days = jsonArrayToStringList(scheduleObj.value(QStringLiteral("days")).toArray());
                            sd.times = jsonArrayToStringList(scheduleObj.value(QStringLiteral("times")).toArray());
                            const QJsonArray cells = dataObj.value(QStringLiteral("cells")).toArray();
                            parseCellsInto(sd, cells);
                            m_pendingSchedules[cls.classId] = sd;
                        }
                    }
                }

                (*pending)--;
                if (*pending <= 0) {
                    delete pending;
                    buildGradeTable(m_pendingGrade, m_pendingTerm);
                }
            });
        }
    }

    static QStringList jsonArrayToStringList(const QJsonArray& arr)
    {
        QStringList list;
        for (const QJsonValue& v : arr) list << v.toString();
        return list;
    }

    void parseCellsInto(ScheduleData& sd, const QJsonArray& cells)
    {
        // 检测是否存在第一列时间文本
        sd.filteredFirstColumn = false;
        for (const QJsonValue& v : cells) {
            if (!v.isObject()) continue;
            const QJsonObject o = v.toObject();
            const int colIndex = o.value(QStringLiteral("col_index")).toInt(-1);
            const QString courseName = o.value(QStringLiteral("course_name")).toString();
            if (colIndex == 0 && looksLikeTimeText(courseName)) {
                sd.filteredFirstColumn = true;
                break;
            }
        }

        for (const QJsonValue& v : cells) {
            if (!v.isObject()) continue;
            const QJsonObject o = v.toObject();
            const int rowIndex = o.value(QStringLiteral("row_index")).toInt(-1);
            const int colIndex = o.value(QStringLiteral("col_index")).toInt(-1);
            const QString courseName = o.value(QStringLiteral("course_name")).toString();
            const bool isHighlight = o.value(QStringLiteral("is_highlight")).toInt(0) != 0;
            if (rowIndex < 0 || colIndex < 0) continue;
            if (courseName.trimmed().isEmpty()) continue;
            if (colIndex == 0 && looksLikeTimeText(courseName)) continue;

            int targetCol = colIndex;
            if (sd.filteredFirstColumn) {
                targetCol = colIndex - 1;
                if (targetCol < 0) continue;
            }
            sd.cellText[rowIndex][targetCol] = courseName;
            if (isHighlight) sd.highlightCells.insert(qMakePair(rowIndex, targetCol));
        }
    }

    void buildGradeTable(const QString& grade, const QString& term)
    {
        const QList<ClassInfo> classes = m_classesByGrade.value(grade);
        if (classes.isEmpty()) {
            m_status->setText(QString::fromUtf8(u8"该年级无班级"));
            return;
        }

        // 以第一个成功返回的 schedule 作为基准 days/times
        QStringList baseDays;
        QStringList baseTimes;
        for (const ClassInfo& cls : classes) {
            if (m_pendingSchedules.contains(cls.classId)) {
                baseDays = m_pendingSchedules[cls.classId].days;
                baseTimes = m_pendingSchedules[cls.classId].times;
                break;
            }
        }
        if (baseDays.isEmpty()) {
            // Qt 5.15 下直接用 initializer_list 赋值可能触发 operator= 歧义（MSVC C2593）
            baseDays = QStringList()
                << QString::fromUtf8(u8"周一")
                << QString::fromUtf8(u8"周二")
                << QString::fromUtf8(u8"周三")
                << QString::fromUtf8(u8"周四")
                << QString::fromUtf8(u8"周五");
        }
        const int dayCount = baseDays.size();
        const int classCount = classes.size();
        const int rows = qMax(baseTimes.size(), 15);
        const int cols = dayCount * classCount;

        m_table->clear();
        m_table->setRowCount(rows);
        m_table->setColumnCount(cols);

        // 表头：day + class
        QStringList headers;
        headers.reserve(cols);
        for (int d = 0; d < dayCount; ++d) {
            for (int ci = 0; ci < classCount; ++ci) {
                headers << QString("%1\n%2").arg(baseDays[d]).arg(classes[ci].displayName);
            }
        }
        m_table->setHorizontalHeaderLabels(headers);

        // 行头：times（不足填空）
        QStringList vheaders = baseTimes;
        while (vheaders.size() < rows) vheaders.append("");
        m_table->setVerticalHeaderLabels(vheaders);

        // 清空单元格
        {
            const QSignalBlocker blocker(m_table);
            for (int r = 0; r < rows; ++r) {
                for (int c = 0; c < cols; ++c) {
                    QTableWidgetItem* item = new QTableWidgetItem("");
                    item->setTextAlignment(Qt::AlignCenter);
                    m_table->setItem(r, c, item);
                }
            }
        }

        // 填充：按 class、day、row
        const QSignalBlocker blocker(m_table);
        for (int ci = 0; ci < classCount; ++ci) {
            const QString classId = classes[ci].classId;
            if (!m_pendingSchedules.contains(classId)) continue;
            const ScheduleData& sd = m_pendingSchedules[classId];

            for (auto rowIt = sd.cellText.begin(); rowIt != sd.cellText.end(); ++rowIt) {
                const int r = rowIt.key();
                if (r < 0 || r >= rows) continue;
                for (auto colIt = rowIt.value().begin(); colIt != rowIt.value().end(); ++colIt) {
                    const int dayIdx = colIt.key(); // 0..dayCount-1
                    if (dayIdx < 0 || dayIdx >= dayCount) continue;
                    const int tableCol = dayIdx * classCount + ci;
                    QTableWidgetItem* item = m_table->item(r, tableCol);
                    if (!item) {
                        item = new QTableWidgetItem("");
                        item->setTextAlignment(Qt::AlignCenter);
                        m_table->setItem(r, tableCol, item);
                    }
                    item->setText(colIt.value());
                    if (sd.highlightCells.contains(qMakePair(r, dayIdx))) {
                        item->setBackground(QBrush(QColor("#3d4f2b")));
                    }
                }
            }
        }

        m_status->setText(QString::fromUtf8(u8"已加载：") + grade + QString::fromUtf8(u8"（") + term + QString::fromUtf8(u8"）")
                          + QString::fromUtf8(u8" 班级数=") + QString::number(classCount));
    }

private:
    QNetworkAccessManager* m_manager = nullptr;
    QComboBox* m_gradeCombo = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QTableWidget* m_table = nullptr;
    QLabel* m_status = nullptr;

    QMap<QString, QList<ClassInfo>> m_classesByGrade; // grade -> classes

    // 当前请求中的临时结果
    QString m_pendingGrade;
    QString m_pendingTerm;
    QMap<QString, ScheduleData> m_pendingSchedules; // classId -> schedule
};


