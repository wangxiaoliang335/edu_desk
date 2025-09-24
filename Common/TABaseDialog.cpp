#include "TABaseDialog.h"
#include <QPainterPath>
TABaseDialog::TABaseDialog(QWidget *parent)
	: QWidget(parent),mainLayout(new QVBoxLayout(this)),
titleBar(new QWidget(this)),
titleLabel(new QLabel(this)),
titleLayout(new QHBoxLayout(titleBar)),
contentLayout(new QVBoxLayout()),
closeButton(new QPushButton(this)),
m_dragging(false), m_backgroundColor(QColor(50, 50, 50, 0.8)),
m_borderColor(QColor(255, 255, 255, 0.1)), m_radius(0), m_borderWidth(0)
{
    setWindowFlags(
        Qt::FramelessWindowHint |
        Qt::Tool |
        Qt::WindowStaysOnTopHint
    );
    setAttribute(Qt::WA_TranslucentBackground);

    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    closeButton->setObjectName("closeButton");
    closeButton->setFixedSize(QSize(23, 23));
    titleLayout->setSpacing(20);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleBar->setObjectName("titleBar");
    titleLayout->addWidget(closeButton);
    //titleLabel->setAlignment(Qt::AlignCenter);
    titleLayout->addWidget(titleLabel);
    titleBar->setFixedHeight(50);
    //titleLayout->addStretch();

    contentLayout->setContentsMargins(0, 0, 0, 0);

    mainLayout->addWidget(titleBar);
    mainLayout->addLayout(contentLayout);
    connect(closeButton, &QPushButton::clicked, this, &TABaseDialog::onClose);
}
TABaseDialog::~TABaseDialog()
{}
void TABaseDialog::onClose()
{
    this->close();
}

void TABaseDialog::setTitle(const QString& text)
{
    this->titleLabel->setText(text);
}
void TABaseDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

QColor TABaseDialog::borderColor() const
{
    return m_borderColor;
}

void TABaseDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPosition);
        event->accept();
    }
}

void TABaseDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
}
int TABaseDialog::borderWidth()
{
    return m_borderWidth;
}
int TABaseDialog::radius()
{
    return m_radius;
}
void TABaseDialog::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);


    QRect rect(0, 0, width(), height());
    int cornerRadius = m_radius;
    QPainterPath path;
    path.addRoundedRect(rect, cornerRadius, cornerRadius);

    painter.fillPath(path, QBrush(m_backgroundColor));

    QPen pen;
    pen.setWidth(m_borderWidth);
    pen.setColor(m_borderColor);
    painter.strokePath(path, pen);
}
QString TABaseDialog::titleName() const
{
    return m_titleName;
}
void TABaseDialog::setBackgroundColor(const QColor& color)
{
    if (m_backgroundColor != color)
    {
        m_backgroundColor = color;
        update();
    }
}
void TABaseDialog::clearLayout(QLayout* layout)
{
    if (!layout)
        return;

    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        else if (item->layout()) {
            clearLayout(item->layout());
        }
        else if (item->spacerItem()) {
            delete item->spacerItem();
        }
        delete item;
    }
}
void TABaseDialog::setRadius(int val)
{
    if (m_radius != val)
    {
        m_radius = val;
        update();
    }
}
void TABaseDialog::setBorderWidth(int val)
{
    if (m_borderWidth != val)
    {
        m_borderWidth = val;
        update();
    }
}
void TABaseDialog::setBorderColor(const QColor& color)
{
    if (m_borderColor != color)
    {
        m_borderColor = color;
        update();
    }
}
void TABaseDialog::setTitleName(const QString& name)
{
    if (m_titleName != name) {
        m_titleName = name;
    }
}
QColor TABaseDialog::backgroundColor() const
{
    return m_backgroundColor;
}