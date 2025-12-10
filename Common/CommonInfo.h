#pragma once
#include "TABaseDialog.h"

struct Notification {
    int id;
    int sender_id;
    QString sender_name;
    int receiver_id;
    QString unique_group_id;
    QString group_name;
    QString content;
    int content_text;
    int is_read;
    int is_agreed;
    QString remark;
    QString created_at;
    QString updated_at;
};

struct GroupMemberInfo {
    QString member_id;
    QString member_name;
    QString member_role;
    bool is_voice_enabled;  // 是否开启语音
};

// StudentInfo 结构体在 ScheduleDialog.h 中定义，这里使用条件编译避免重复定义
//#ifndef STUDENT_INFO_DEFINED
//#define STUDENT_INFO_DEFINED
struct StudentInfo {
    QString id;      // 学号
    QString name;    // 姓名
    QString groupName; // 小组名称
    double score;    // 成绩（用于排序）
    double groupTotalScore = 0.0; // 小组总分
    int originalIndex; // 原始索引
    QMap<QString, double> attributes; // 多个属性值（如"背诵"、"语文"等）
    QMap<QString, QString> comments; // 字段注释（如"数学" -> "需要加强练习"）
};
//#endif

class CommonInfo
{
public:
	CommonInfo()
	{

	}

    static void InitData(UserInfo userInfo)
    {
        m_userInfo = userInfo;
    }

    static UserInfo GetData()
    {
        return m_userInfo;
    }

private:
    static UserInfo m_userInfo;
};

// 这里必须定义静态成员变量一次，否则会链接错误
//UserInfo CommonInfo::m_userInfo;
