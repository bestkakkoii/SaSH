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
//#define OPENGL_LABEL
class FastLabel
#ifdef OPENGL_LABEL
	: public QOpenGLWidget, protected QOpenGLFunctions
#else
	: public QWidget
#endif
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
	void setFlag(long long flag = Qt::AlignLeft | Qt::AlignVCenter);
	long long getFlag();

	inline void setAutoResize(bool isAutoResize) { isAutoResize_ = isAutoResize; }
protected:
#ifdef OPENGL_LABEL
	virtual void initializeGL() override;
	virtual void resizeGL(int w, int h) override;
	virtual void paintGL() override;
#else
	virtual void resizeEvent(QResizeEvent* event) override;
	virtual void paintEvent(QPaintEvent*) override;
#endif

private:
	long long flag_ = Qt::AlignLeft | Qt::AlignVCenter;
	bool isAutoResize_ = false;
	QPen pen_;
	QFont font_;
	QPixmap pixmap_;
	QString content_msg_;
	QColor text_color_ = Qt::black;
	QColor background_color_ = Qt::white;
	long long new_font_size_ = 12;
};
#endif // MYLABEL_H

