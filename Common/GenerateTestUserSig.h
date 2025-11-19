#pragma once

/*
 * Module: GenerateTestUserSig
 *
 * Function: Used to generate UserSig for testing. UserSig is a security signature designed by Tencent Cloud for its cloud services.
 *           It is calculated based on SDKAppID, UserID, and EXPIRETIME using the HMAC-SHA256 encryption algorithm.
 *
 * Attention: Do not use the code below in your commercial application. This is because:
 *
 *            The code may be able to calculate UserSig correctly, but it is only for quick testing of the SDK's basic features, not for commercial applications.
 *            SECRETKEY in client code can be easily decompiled and reversed, especially on web.
 *            Once your key is disclosed, attackers will be able to steal your Tencent Cloud traffic.
 *
 *            The correct method is to deploy the UserSig calculation code and encryption key on your project server so that your application can request from your server a UserSig that is calculated whenever one is needed.
 *            Given that it is more difficult to hack a server than a client application, server-end calculation can better protect your key.
 *
 * Reference: https://intl.cloud.tencent.com/document/product/1047/34385
 */
#include <string>
#include <stdint.h>
#include <QObject>
#include <QEventLoop>
#include <QTimer>
#include <QString>

class TAHttpHandler;

class GenerateTestUserSig : public QObject
{
    Q_OBJECT

private:
    GenerateTestUserSig();
    ~GenerateTestUserSig();

   /**
    * Tencent Cloud SDKAppID. Set it to the SDKAppID of your account.
    * <p>
    * You can view your SDKAppID after creating an application in the [Tencent Cloud IM console](https://console.intl.cloud.tencent.com/im).
    * SDKAppID uniquely identifies a Tencent Cloud account.
    */
	const uint32_t SDKAPPID = 1600111046;

   /**
    * Signature validity period, which should not be set too short
    * 
    * Time unit: second
    * Default value: 604800 (7 days)
    */
    const uint32_t EXPIRETIME = 604800;

   /**
     * Follow the steps below to obtain the key required for UserSig calculation.
     * 
     * Step 1. Log in to the [Tencent Cloud IM console](https://console.intl.cloud.tencent.com/im), and create an application if you don't have one.
     * Step 2. Click Application Configuration to go to the basic configuration page and locate Account System Integration.
     * Step 3. Click View Key to view the encrypted key used for UserSig calculation. Then copy and paste the key to the variable below.
     * 
     * Note: this method is for testing only. Before commercial launch, please migrate the UserSig calculation code and key to your backend server to prevent key disclosure and traffic stealing.
     * Reference: https://intl.cloud.tencent.com/document/product/1047/34385
     */
    const char* SECRETKEY = "6b0aad55725e4b04b1292c224cc2c16b92210a62650492884def1122b70794d0";

public:
    static GenerateTestUserSig& instance();

    uint32_t getSDKAppID() const;

    /**
     * 获取REST API管理员账号UserID
     * 从CommonInfo中获取当前登录用户的teacher_unique_id作为管理员账号
     * 注意：需要确保CommonInfo::InitData()已经调用并设置了用户信息
     * @return 管理员账号UserID，如果未设置则返回空字符串
     */
    std::string getAdminUserId() const;

    /**
     * Calculate UserSig
     * 
     * Now fetches UserSig from server instead of calculating locally.
     * 
     * @param userId The user ID for which to get UserSig
     * @return The UserSig string, or empty string if request fails
     */
    std::string genTestUserSig(const std::string& userId);

private slots:
    void onHttpSuccess(const QString& response);
    void onHttpFailed(const QString& error);

private:
    TAHttpHandler* m_httpHandler;
    QEventLoop* m_eventLoop;
    QString m_receivedUserSig;
    bool m_requestCompleted;
    bool m_requestSuccess;
};
