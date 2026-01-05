#pragma execution_character_set("utf-8")
#include <QApplication>
#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <qpainterpath.h>
#include <qpainter.h>
#include <QMouseEvent>
#include <QRegion>
#include <QStackedWidget>
#include "QSchoolInfoWidget.h"
#include "QClassMgr.h"
#include "MemberManagerWidget.h"

class SchoolInfoDialog : public QDialog
{
    Q_OBJECT
public:
    SchoolInfoDialog(QWidget *parent = nullptr) : QDialog(parent),
        m_backgroundColor(QColor(43, 43, 43)),  // 深色主题背景
        m_dragging(false),
        m_borderColor(QColor(120, 120, 120)),
        m_borderWidth(1), m_radius(20)  // 增大圆角半径
    {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setWindowTitle("配置学校信息");
        resize(900, 550);  // 增大窗口尺寸，让右侧内容区域更大

        // 设置背景图片
        //setStyleSheet("QDialog {"
        //              "background-image: url(:/images/bg.png);"
        //              "background-repeat: no-repeat;"
        //              "background-position: center;"
        //              "}");

        // 左侧菜单按钮 - 深色主题风格，靠近左边缘
        menuLayout = new QVBoxLayout;
        menuLayout->setContentsMargins(0, 0, 0, 0);  // 无左边距，让菜单靠近窗口左边缘
        menuLayout->setSpacing(2);
        
        // 菜单按钮样式：选中为蓝色，未选中为深灰色
        QString selectedStyle = "QPushButton { "
            "background-color: #2563eb; "  // 蓝色选中
            "color: white; "
            "font-size: 18px; "  // 增大字体
            "font-weight: 500; "
            "border: none; "
            "padding: 14px 16px; "  // 增大内边距
            "text-align: left; "
            "}";
        
        QString unselectedStyle = "QPushButton { "
            "background-color: transparent; "
            "color: rgba(255, 255, 255, 0.7); "
            "font-size: 18px; "  // 增大字体
            "font-weight: 400; "
            "border: none; "
            "padding: 14px 16px; "  // 增大内边距
            "text-align: left; "
            "} "
            "QPushButton:hover { "
            "background-color: rgba(255, 255, 255, 0.1); "
            "color: rgba(255, 255, 255, 0.9); "
            "}";
        
        btn1 = new QPushButton("学校信息");
        btn1->setStyleSheet(selectedStyle);
        btn2 = new QPushButton("班级管理");
        btn2->setStyleSheet(unselectedStyle);
        btn3 = new QPushButton("通讯录");
        btn3->setStyleSheet(unselectedStyle);
        btn4 = new QPushButton("健康管理");
        btn4->setStyleSheet(unselectedStyle);
        btn5 = new QPushButton("成长管理");
        btn5->setStyleSheet(unselectedStyle);

        // 创建左侧菜单容器，设置固定宽度和背景，靠近左边缘
        QWidget* menuWidget = new QWidget;
        menuWidget->setFixedWidth(180);  // 减小宽度，让菜单更紧凑
        menuWidget->setStyleSheet("QWidget { background-color: rgba(0, 0, 0, 0.2); }");
        QVBoxLayout* menuContainerLayout = new QVBoxLayout(menuWidget);
        menuContainerLayout->setContentsMargins(5, 80, 5, 0);  // 减少左右边距，让菜单靠近边缘
        menuContainerLayout->setSpacing(2);
        menuContainerLayout->addWidget(btn1);
        menuContainerLayout->addWidget(btn2);
        menuContainerLayout->addWidget(btn3);
        menuContainerLayout->addWidget(btn4);
        menuContainerLayout->addWidget(btn5);
        menuContainerLayout->addStretch();
        
        menuLayout->addWidget(menuWidget);

        // 关闭按钮 - 右上角，使用绝对定位，不参与布局，避免影响标题位置
        closeButton = new QPushButton("×", this);
        closeButton->setFixedSize(QSize(30, 30));
        closeButton->setStyleSheet(
            "QPushButton { "
            "background: transparent; "
            "color: rgba(255, 255, 255, 0.8); "
            "font-size: 20px; "
            "font-weight: bold; "
            "border: none; "
            "} "
            "QPushButton:hover { "
            "background-color: rgba(255, 0, 0, 0.3); "
            "color: white; "
            "}");
        closeButton->move(this->width() - 40, 10);  // 右上角位置
        closeButton->hide();  // 默认隐藏
        closeButton->raise();  // 确保在最上层
        connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);

        connect(btn1, &QPushButton::clicked, this, [=]() {
            if (m_stack && schoolInfoWidget) {
                m_stack->setCurrentWidget(schoolInfoWidget);
                curWidget = schoolInfoWidget;
            }

            // 学校信息页面：显示确定和取消按钮
            if (cancelButton) cancelButton->show();
            if (okButton) okButton->show();

            // 更新按钮样式
            QString selectedStyle = "QPushButton { background-color: #2563eb; color: white; font-size: 18px; font-weight: 500; border: none; padding: 14px 16px; text-align: left; }";
            QString unselectedStyle = "QPushButton { background-color: transparent; color: rgba(255, 255, 255, 0.7); font-size: 18px; font-weight: 400; border: none; padding: 14px 16px; text-align: left; } QPushButton:hover { background-color: rgba(255, 255, 255, 0.1); color: rgba(255, 255, 255, 0.9); }";
            btn1->setStyleSheet(selectedStyle);
            btn2->setStyleSheet(unselectedStyle);
            btn3->setStyleSheet(unselectedStyle);
            btn4->setStyleSheet(unselectedStyle);
            btn5->setStyleSheet(unselectedStyle);
        });

        connect(btn2, &QPushButton::clicked, this, [=]() {
            if (schoolInfoWidget && classMgr) {
                QString schoolId = schoolInfoWidget->getSchoolId();
                classMgr->setSchoolId(schoolId);
                classMgr->initData();
            }
            if (m_stack && classMgr) {
                m_stack->setCurrentWidget(classMgr);
                curWidget = classMgr;
            }

            // 班级管理页面：隐藏确定和取消按钮
            if (cancelButton) cancelButton->hide();
            if (okButton) okButton->hide();

            // 更新按钮样式
            QString selectedStyle = "QPushButton { background-color: #2563eb; color: white; font-size: 18px; font-weight: 500; border: none; padding: 14px 16px; text-align: left; }";
            QString unselectedStyle = "QPushButton { background-color: transparent; color: rgba(255, 255, 255, 0.7); font-size: 18px; font-weight: 400; border: none; padding: 14px 16px; text-align: left; } QPushButton:hover { background-color: rgba(255, 255, 255, 0.1); color: rgba(255, 255, 255, 0.9); }";
            btn2->setStyleSheet(selectedStyle);
            btn1->setStyleSheet(unselectedStyle);
            btn3->setStyleSheet(unselectedStyle);
            btn4->setStyleSheet(unselectedStyle);
            btn5->setStyleSheet(unselectedStyle);
        });

        connect(btn3, &QPushButton::clicked, this, [=]() {
            if (schoolInfoWidget && memberMgrWidget) {
                QString schoolId = schoolInfoWidget->getSchoolId();
                memberMgrWidget->setSchoolId(schoolId);
            }
            if (m_stack && memberMgrWidget) {
                m_stack->setCurrentWidget(memberMgrWidget);
                curWidget = memberMgrWidget;
            }

            // 通讯录页面：显示确定和取消按钮
            if (cancelButton) cancelButton->show();
            if (okButton) okButton->show();

            // 更新按钮样式
            QString selectedStyle = "QPushButton { background-color: #2563eb; color: white; font-size: 18px; font-weight: 500; border: none; padding: 14px 16px; text-align: left; }";
            QString unselectedStyle = "QPushButton { background-color: transparent; color: rgba(255, 255, 255, 0.7); font-size: 18px; font-weight: 400; border: none; padding: 14px 16px; text-align: left; } QPushButton:hover { background-color: rgba(255, 255, 255, 0.1); color: rgba(255, 255, 255, 0.9); }";
            btn3->setStyleSheet(selectedStyle);
            btn1->setStyleSheet(unselectedStyle);
            btn2->setStyleSheet(unselectedStyle);
            btn4->setStyleSheet(unselectedStyle);
            btn5->setStyleSheet(unselectedStyle);
            });

        // 顶部标题栏 - 标题居中，关闭按钮固定位置不影响标题
        QHBoxLayout* titleLayout = new QHBoxLayout;
        titleLayout->setContentsMargins(50, 15, 15, 15);
        titleLayout->addStretch();  // 左侧弹性空间
        pLabel = new QLabel("配置学校信息");
        pLabel->setStyleSheet("color: white; font-size: 20px; font-weight: 600;");  // 增大标题字体
        titleLayout->addWidget(pLabel);
        titleLayout->addStretch();  // 右侧弹性空间，使标题居中
        
        // 关闭按钮使用固定宽度的占位符，确保标题始终居中
        QWidget* closeButtonPlaceholder = new QWidget;
        closeButtonPlaceholder->setFixedWidth(40);  // 与关闭按钮区域同宽（30px按钮 + 10px边距）
        titleLayout->addWidget(closeButtonPlaceholder);
        
        // 右侧内容区域布局，增大尺寸
        m_vBoxLayout = new QVBoxLayout;
        m_vBoxLayout->setContentsMargins(30, 0, 30, 0);  // 增大左右边距，让内容区域更大
        m_vBoxLayout->setSpacing(0);

        schoolInfoWidget = new QSchoolInfoWidget(NULL);
        classMgr = new QClassMgr(NULL);
        memberMgrWidget = new MemberManagerWidget;

        m_vBoxLayout->addLayout(titleLayout);
        // 右侧内容使用堆叠容器，切换页面不触发布局抖动（左侧菜单位置固定）
        m_stack = new QStackedWidget;
        m_stack->setContentsMargins(0, 0, 0, 0);
        m_stack->addWidget(schoolInfoWidget);
        m_stack->addWidget(classMgr);
        m_stack->addWidget(memberMgrWidget);
        m_stack->setCurrentWidget(schoolInfoWidget);
        m_vBoxLayout->addWidget(m_stack, 1);
        
        // 底部按钮区域 - 按钮居中，固定在窗口底部
        buttonLayout = new QHBoxLayout;
        buttonLayout->setContentsMargins(0, 20, 0, 30);  // 增大底部边距到30px，让按钮更靠下
        buttonLayout->addStretch();  // 左侧弹性空间
        cancelButton = new QPushButton("取消");
        cancelButton->setFixedSize(100, 36);
        cancelButton->setStyleSheet(
            "QPushButton { "
            "background-color: rgba(255, 255, 255, 0.1); "
            "color: white; "
            "border: 1px solid rgba(255, 255, 255, 0.2); "
            "border-radius: 6px; "
            "font-size: 14px; "
            "} "
            "QPushButton:hover { "
            "background-color: rgba(255, 255, 255, 0.15); "
            "}");
        okButton = new QPushButton("确定");
        okButton->setFixedSize(100, 36);
        okButton->setStyleSheet(
            "QPushButton { "
            "background-color: #2563eb; "
            "color: white; "
            "border: none; "
            "border-radius: 6px; "
            "font-size: 14px; "
            "font-weight: 500; "
            "} "
            "QPushButton:hover { "
            "background-color: #1d4ed8; "
            "}");
        buttonLayout->addWidget(cancelButton);
        buttonLayout->addSpacing(12);  // 按钮之间的间距
        buttonLayout->addWidget(okButton);
        buttonLayout->addStretch();  // 右侧弹性空间，使按钮居中
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
        
        m_vBoxLayout->addLayout(buttonLayout);

        // 总布局：左菜单固定贴左，右侧自适应占满（切换页左侧不移动）
        mainLayout = new QHBoxLayout;
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);
        mainLayout->addLayout(menuLayout);
        mainLayout->addSpacing(30);
        mainLayout->addLayout(m_vBoxLayout, 1);

        curWidget = schoolInfoWidget;
        setLayout(mainLayout);
    }

    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QRect rect(0, 0, width(), height());
        QPainterPath path;
        path.addRoundedRect(rect, m_radius, m_radius);

        p.fillPath(path, QBrush(m_backgroundColor));
        QPen pen(m_borderColor, m_borderWidth);
        p.strokePath(path, pen);

        // 设置窗口遮罩，使圆角之外透明
        QRegion region(path.toFillPolygon().toPolygon());
        setMask(region);

        // 标题已在布局中显示，这里不再绘制
    }

    void mousePressEvent(QMouseEvent* event) {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        }
    }

    void mouseMoveEvent(QMouseEvent* event) {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - m_dragStartPos);
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
        }
    }

    void leaveEvent(QEvent* event)
    {
        QDialog::leaveEvent(event);
        if (closeButton) {
            closeButton->hide();
        }
    }

    void enterEvent(QEvent* event)
    {
        QDialog::enterEvent(event);
        if (m_visibleCloseButton && closeButton) {
            closeButton->show();
            // 确保关闭按钮位置正确（右上角）
            closeButton->move(this->width() - closeButton->width() - 10, 10);
        }
    }

    void resizeEvent(QResizeEvent* event)
    {
        QDialog::resizeEvent(event);
        // 关闭按钮使用绝对定位，确保始终在右上角
        if (closeButton) {
            closeButton->move(this->width() - closeButton->width() - 10, 10);
        }
        // 更新窗口遮罩以适应新的窗口大小
        QPainterPath path;
        path.addRoundedRect(0, 0, width(), height(), m_radius, m_radius);
        QRegion region(path.toFillPolygon().toPolygon());
        setMask(region);
    }

    void setTitleName(const QString& name) {
        m_titleName = name;
        update();
    }

    void visibleCloseButton(bool val)
    {
        m_visibleCloseButton = val;
    }

    void setBackgroundColor(const QColor& color) {
        m_backgroundColor = color;
        update();
    }

    void setBorderColor(const QColor& color) {
        m_borderColor = color;
        update();
    }

    void setBorderWidth(int val) {
        m_borderWidth = val;
        update();
    }

    void setRadius(int val) {
        m_radius = val;
        update();
    }

    void InitData(UserInfo userInfo)
    {
        m_userInfo = userInfo;
        if (schoolInfoWidget)
        {
            schoolInfoWidget->InitData(m_userInfo);
        }
        if (memberMgrWidget)
        {
            memberMgrWidget->InitData(m_userInfo);
        }
    }

    QSchoolInfoWidget* schoolInfoWidget = NULL;
    QClassMgr* classMgr = NULL;
    QHBoxLayout* mainLayout = NULL;
    MemberManagerWidget* memberMgrWidget = NULL;
    QWidget* curWidget = NULL;
    QColor m_backgroundColor;
    QColor m_borderColor;
    int m_borderWidth;
    int m_radius;
    QString m_titleName;
    bool m_dragging;
    bool m_visibleCloseButton;
    QPoint m_dragStartPos;
    QPushButton* closeButton = NULL;
    QVBoxLayout* m_vBoxLayout = NULL;
    QVBoxLayout* menuLayout = NULL;
    QHBoxLayout* buttonLayout = NULL;  // 底部按钮布局
    QPushButton* cancelButton = NULL;  // 取消按钮
    QPushButton* okButton = NULL;  // 确定按钮
    QStackedWidget* m_stack = NULL;  // 右侧内容堆叠容器
    QLabel* pLabel = NULL;
    QPushButton* btn1 = NULL;
    QPushButton* btn2 = NULL;
    QPushButton* btn3 = NULL;
    QPushButton* btn4 = NULL;
    QPushButton* btn5 = NULL;
    UserInfo m_userInfo;
};