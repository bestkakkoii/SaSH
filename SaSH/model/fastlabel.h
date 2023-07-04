#pragma once
#ifndef MYLABEL_H
#define MYLABEL_H

#include <QLabel>
#include <QRect>
#include <QPixmap>
#include <QResizeEvent>
#include <QWidget>

class FastLabel : public QWidget
{
	Q_OBJECT;
	Q_PROPERTY(QColor text_color WRITE setTextColor READ getTextColor);
	Q_DISABLE_COPY(FastLabel);
public:
	explicit FastLabel(QWidget* parent);
	~FastLabel() override;
	FastLabel();

	void setTextColor(const QColor& color);
	QColor getTextColor();
	void setText(const QString& text);
	QString getText();
	void setFlag(int flag = Qt::AlignLeft | Qt::AlignVCenter);
	int getFlag();
protected:
	void resizeEvent(QResizeEvent* event) override;
	void paintEvent(QPaintEvent*) override;

private:
	int flag_ = Qt::AlignLeft | Qt::AlignVCenter;

	QPixmap pixmap_;
	QString content_msg_;
	QString text_cache_;
	QColor text_color_;
};
#endif // MYLABEL_H

