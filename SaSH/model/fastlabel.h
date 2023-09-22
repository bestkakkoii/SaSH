/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2023 All Rights Reserved.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/

#pragma once
#ifndef MYLABEL_H
#define MYLABEL_H

#include <QLabel>
#include <QRect>
#include <QPixmap>
#include <QResizeEvent>
#include <QWidget>

class FastLabel : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT;
	Q_PROPERTY(QColor text_color WRITE setTextColor READ getTextColor);
	Q_DISABLE_COPY(FastLabel);
public:
	FastLabel();
	explicit FastLabel(QWidget* parent);
	FastLabel(const QString& text, QColor textColor, QColor backgroundColor, QWidget* parent = nullptr);
	virtual~FastLabel() override;

	void setTextColor(const QColor& color);
	void setBackgroundColor(const QColor& color);
	QColor getTextColor();
	void setText(const QString& text);
	QString getText() const;
	void setFlag(int flag = Qt::AlignLeft | Qt::AlignVCenter);
	int getFlag();
protected:
	//void resizeEvent(QResizeEvent* event) override;
	//void paintEvent(QPaintEvent*) override;

	virtual void initializeGL() override;
	virtual void resizeGL(int w, int h) override;
	virtual void paintGL() override;

private:
	int flag_ = Qt::AlignLeft | Qt::AlignVCenter;

	QPen pen_;
	QFont font_;
	QPixmap pixmap_;
	QString content_msg_;
	QColor text_color_ = Qt::black;
	QColor background_color_ = QColor("#F1F1F1");
	qint64 new_font_size_ = 12;
};
#endif // MYLABEL_H

