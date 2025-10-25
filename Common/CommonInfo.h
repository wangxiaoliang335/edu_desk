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
