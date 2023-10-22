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

#include "stdafx.h"
#include "fastlabel.h"
#include <QPainter>

FastLabel::FastLabel(QWidget* parent)
#ifdef OPENGL_LABEL
	: QOpenGLWidget(parent)
#else
	: QWidget(parent)
#endif
{
	font_ = util::getFont();
	content_msg_ = "";
	pen_.setCosmetic(true);
}

FastLabel::FastLabel(const QString& text, QColor textColor, QColor backgroundColor, QWidget* parent)
#ifdef OPENGL_LABEL
	: QOpenGLWidget(parent)
#else
	: QWidget(parent)
#endif
	, text_color_(textColor)
	, background_color_(backgroundColor)
{
	font_ = util::getFont();
	content_msg_ = text;
	pen_.setColor(textColor);
	pen_.setCosmetic(true);
}

FastLabel::FastLabel()
{
	content_msg_ = "";
	pen_.setCosmetic(true);
}

FastLabel::~FastLabel()
{
}

void FastLabel::setTextColor(const QColor& color)
{
	text_color_ = color;
}

void FastLabel::setBackgroundColor(const QColor& color)
{
	background_color_ = color;
}

QColor FastLabel::getTextColor()
{
	return text_color_;
}

QString FastLabel::getText() const
{
	return content_msg_;
}

void FastLabel::setFlag(int flag)
{
	flag_ = flag;
}
int FastLabel::getFlag()
{
	return flag_;
}

void FastLabel::setText(const QString& text)
{
	setUpdatesEnabled(false);
	content_msg_ = text;

	if (isAutoResize_)
	{
		QFontMetrics fm(font_);
		int textWidth = fm.horizontalAdvance(text) + 4;

		setFixedWidth(textWidth);
	}

	setUpdatesEnabled(true);
	update();
}


#ifdef OPENGL_LABEL
void FastLabel::initializeGL()
{
	initializeOpenGLFunctions();

	glClearColor(background_color_.redF(), background_color_.greenF(), background_color_.blueF(), 1.0f);

	glEnable(GL_MULTISAMPLE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SMOOTH);
}

void FastLabel::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h); //設置視口
	glMatrixMode(GL_PROJECTION); //設置矩陣模式
	glLoadIdentity(); //重置矩陣
	glOrtho(0, w, h, 0, -1, 1); //設置正交投影
	glMatrixMode(GL_MODELVIEW); //設置矩陣模式
	glLoadIdentity(); //重置矩陣
}

void FastLabel::paintGL()
{
	glClearColor(background_color_.redF(), background_color_.greenF(), background_color_.blueF(), 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// 在指定位置绘制文本
	QPainter painter(this);
	pen_.setColor(text_color_);
	painter.setFont(font_);
	painter.setPen(pen_);
	painter.setRenderHint(QPainter::Antialiasing);// 抗锯齿
	painter.setRenderHint(QPainter::TextAntialiasing); // 文本抗锯齿
	painter.setRenderHint(QPainter::SmoothPixmapTransform); // 平滑像素变换
	painter.setRenderHint(QPainter::LosslessImageRendering);

	painter.beginNativePainting();

	// 使用存储的字体和文本内容进行绘制
	painter.fillRect(rect(), background_color_);
	painter.drawText(rect(), flag_, content_msg_);

	painter.endNativePainting();

	glFinish();
}
#else
void FastLabel::resizeEvent(QResizeEvent*)
{
	update();
}

void FastLabel::paintEvent(QPaintEvent* e)
{
	QPainter painter(this);
	painter.setFont(font_);
	painter.setPen(QPen(text_color_));
	painter.setRenderHint(QPainter::Antialiasing);// 抗锯齿
	painter.setRenderHint(QPainter::TextAntialiasing); // 文本抗锯齿
	painter.setRenderHint(QPainter::SmoothPixmapTransform); // 平滑像素变换
	painter.setRenderHint(QPainter::LosslessImageRendering);
	painter.beginNativePainting();
	painter.fillRect(rect(), background_color_);
	painter.drawText(rect(), flag_, content_msg_);
	painter.endNativePainting();
}
#endif
