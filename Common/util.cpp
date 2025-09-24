#pragma execution_character_set("utf-8")
#include "util.h"
#include <QRencode.h>
QImage TAUtil::generateQRCode(const QString& text, int size) {
	QByteArray data = text.toUtf8();
	QRcode* qr = QRcode_encodeString(data.constData(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
	if (!qr) {
		return QImage();
	}
	QImage image(size, size, QImage::Format_RGB32);
	image.fill(Qt::white);

	int scale = size / qr->width;

	for (int y = 0; y < qr->width; ++y) {
		for (int x = 0; x < qr->width; ++x) {
			unsigned char b = qr->data[y * qr->width + x] & 0x01;
			QColor color = b ? Qt::black : Qt::white;
			for (int dy = 0; dy < scale; ++dy) {
				for (int dx = 0; dx < scale; ++dx) {
					image.setPixel(x * scale + dx, y * scale + dy, color.rgb());
				}
			}
		}
	}
	QRcode_free(qr);
	return image;
}
QString TAUtil::getCurrentDateTime()
{
	return QDateTime::currentDateTime().toString("yyyy年M月d日 HH:mm:ss");
}
QString TAUtil::getCurrentDate()
{
	return QDateTime::currentDateTime().toString("yyyy年M月d日");
}
QString TAUtil::getCurrentTime()
{
	return QDateTime::currentDateTime().toString("HH:mm:ss");
}
QString TAUtil::getCurrentDateTimeWithFormat(const QString& format)
{
	return QDateTime::currentDateTime().toString(format);
}