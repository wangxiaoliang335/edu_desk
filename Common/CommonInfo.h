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

// ������붨�徲̬��Ա����һ�Σ���������Ӵ���
//UserInfo CommonInfo::m_userInfo;
