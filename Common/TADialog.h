#pragma once

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include "TABaseDialog.h"
class TADialog  : public TABaseDialog
{
	Q_OBJECT

public:
    explicit TADialog(QWidget *parent);
	~TADialog();


private slots:
    void onCancelClicked();
    void onEnterClicked();
signals:
    void cancelClicked();
    void enterClicked();

private:
    QWidget* buttonWidget;
    QPushButton* cancelButton;
    QPushButton* enterButton;
    QHBoxLayout* buttonLayout;
};
