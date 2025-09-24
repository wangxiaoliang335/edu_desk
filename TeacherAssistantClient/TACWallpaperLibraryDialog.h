#pragma once

#include "TABaseDialog.h"
class TACWallpaperItemWidget : public QWidget
{
	Q_OBJECT
public:
	explicit TACWallpaperItemWidget(QWidget* parent = nullptr);
	void setBackgroundImage(const QString& path);
protected:
	void enterEvent(QEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void paintEvent(QPaintEvent*) override;
private:
	bool hover;
	QPixmap backgroundImage;
};
class TACWallpaperLibraryDialog  : public TABaseDialog
{
	Q_OBJECT

public:
	TACWallpaperLibraryDialog(QWidget *parent);
	~TACWallpaperLibraryDialog();
	void init();
private:
	QGridLayout* gridLayout;
};
