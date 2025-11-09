#pragma once
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <functional>

/**
 * @brief 腾讯云IM REST API封装类
 * 
 * 用于替代腾讯IM SDK的客户端调用，通过REST API在服务端进行操作
 * 注意：REST API需要使用管理员账号的UserSig进行鉴权
 */
class TIMRestAPI : public QObject
{
    Q_OBJECT

public:
    // 回调函数类型定义
    typedef std::function<void(int errorCode, const QString& errorDesc, const QJsonObject& result)> RestAPICallback;

    explicit TIMRestAPI(QObject* parent = nullptr);
    ~TIMRestAPI();

    /**
     * @brief 设置管理员账号信息
     * @param adminUserId 管理员账号ID
     * @param adminUserSig 管理员账号的UserSig
     */
    void setAdminInfo(const QString& adminUserId, const QString& adminUserSig);

    /**
     * @brief 创建群组
     * @param groupName 群组名称
     * @param groupType 群组类型：Private(私有群), Public(公开群), ChatRoom(聊天室), AVChatRoom(直播群)
     * @param memberList 初始成员列表（包含群主）
     * @param callback 回调函数
     */
    void createGroup(const QString& groupName, const QString& groupType, 
                     const QJsonArray& memberList, RestAPICallback callback);

    /**
     * @brief 邀请成员加入群组
     * @param groupId 群组ID
     * @param memberList 要邀请的成员ID列表
     * @param callback 回调函数
     */
    void inviteGroupMember(const QString& groupId, const QJsonArray& memberList, RestAPICallback callback);

    /**
     * @brief 删除群组成员（踢出成员）
     * @param groupId 群组ID
     * @param memberList 要删除的成员ID列表
     * @param reason 删除原因（可选）
     * @param callback 回调函数
     */
    void deleteGroupMember(const QString& groupId, const QJsonArray& memberList, 
                          const QString& reason, RestAPICallback callback);

    /**
     * @brief 解散群组
     * @param groupId 群组ID
     * @param callback 回调函数
     */
    void destroyGroup(const QString& groupId, RestAPICallback callback);

    /**
     * @brief 退出群组（删除指定成员）
     * @param groupId 群组ID
     * @param userId 要退出的用户ID
     * @param callback 回调函数
     */
    void quitGroup(const QString& groupId, const QString& userId, RestAPICallback callback);

    /**
     * @brief 获取群组信息
     * @param groupIdList 群组ID列表
     * @param callback 回调函数
     */
    void getGroupInfo(const QStringList& groupIdList, RestAPICallback callback);

    /**
     * @brief 获取群成员列表
     * @param groupId 群组ID
     * @param limit 分页拉取的群成员数量，最大100
     * @param offset 分页拉取的偏移量
     * @param callback 回调函数
     */
    void getGroupMemberList(const QString& groupId, int limit = 100, int offset = 0, RestAPICallback callback = nullptr);

    /**
     * @brief 修改群组基础信息
     * @param groupId 群组ID
     * @param groupName 群组名称（可选）
     * @param introduction 群组简介（可选）
     * @param notification 群组公告（可选）
     * @param faceUrl 群组头像URL（可选）
     * @param maxMemberCount 最大成员数（可选）
     * @param applyJoinOption 加群选项：FreeAccess(自由加入), NeedPermission(需要验证), DisableApply(禁止加群)（可选）
     * @param callback 回调函数
     */
    void modifyGroupInfo(const QString& groupId, 
                        const QString& groupName = QString(),
                        const QString& introduction = QString(),
                        const QString& notification = QString(),
                        const QString& faceUrl = QString(),
                        int maxMemberCount = 0,
                        const QString& applyJoinOption = QString(),
                        RestAPICallback callback = nullptr);

    /**
     * @brief 修改群成员信息（设置/取消管理员、禁言等）
     * @param groupId 群组ID
     * @param memberAccount 成员账号
     * @param role 成员角色：Admin(管理员), Member(普通成员)（可选）
     * @param shutUpTime 禁言时间，单位秒，0表示取消禁言（可选）
     * @param nameCard 群名片（可选）
     * @param callback 回调函数
     */
    void modifyGroupMemberInfo(const QString& groupId, 
                              const QString& memberAccount,
                              const QString& role = QString(),
                              int shutUpTime = -1,
                              const QString& nameCard = QString(),
                              RestAPICallback callback = nullptr);

private:
    /**
     * @brief 发送REST API请求
     * @param apiName API名称（如：group_open_http_svc/create_group）
     * @param requestBody 请求体JSON对象
     * @param callback 回调函数
     */
    void sendRestAPIRequest(const QString& apiName, const QJsonObject& requestBody, RestAPICallback callback);

    /**
     * @brief 获取REST API基础URL
     */
    QString getBaseURL() const;

    /**
     * @brief 解析REST API响应
     */
    void parseResponse(QNetworkReply* reply, RestAPICallback callback);

private:
    QString m_adminUserId;      // 管理员账号ID
    QString m_adminUserSig;      // 管理员账号UserSig
    uint32_t m_sdkAppId;         // SDKAppID
    QNetworkAccessManager* m_networkManager; // 网络管理器
};

