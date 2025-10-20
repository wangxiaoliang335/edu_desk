#pragma once
#include "TABaseDialog.h"

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
