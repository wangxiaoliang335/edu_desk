#pragma once
#include <QImage>
#include <QDateTime>
class TAUtil {
	static QImage generateQRCode(const QString& text, int size = 100);
	static QString getCurrentDateTime();
	static QString getCurrentTime();
	static QString getCurrentDate();
	static QString getCurrentDateTimeWithFormat(const QString &format);
};