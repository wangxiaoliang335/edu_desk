#include "CommonInfo.h"

// 这里必须定义静态成员变量一次，否则会链接错误
UserInfo CommonInfo::m_userInfo;
QMap<QString, QString> CommonInfo::m_teacherIdNumberMap;  // teacher_unique_id -> id_number 映射
