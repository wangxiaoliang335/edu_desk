#pragma once
#define TAC_VERSION 1.0

#define TAC_ICON_TEXT "TeacherAssistantClient"
#define TAC_APP_NAME "TeacherAssistant"
#define TAC_APP_LOG ":/res/img/logo.png"
#define TAC_CONFIG_FILE "tac-config.json"
#define TAC_APP_ICON QIcon(":/res/img/logo.ico")

#define TAC_SERVER_URL "http://47.100.126.194"
#define HJ_URL_UPDATE HJ_SERVER_URL"/service/v10/updater"
#define TAC_AES_KEY "evGel4wHMqzOVxJj6k8gKwODBrzSAvKo"



#define CENTER_ON_SCREEN \
    do { \
        QRect screenGeometry = QGuiApplication::primaryScreen()->geometry(); \
        int x = (screenGeometry.width() - width()) / 2; \
        int y = (screenGeometry.height() - height()) / 2; \
        move(x, y); \
    } while (0)


#define WIDGET_BACKGROUND_COLOR QColor(50,50,50,200)
#define WIDGET_BORDER_COLOR QColor(255,255,255,25)
#define WIDGET_BORDER_WIDTH 1