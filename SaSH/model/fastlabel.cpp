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
	: QWidget(parent)
{
	content_msg_ = "";
	text_cache_ = "";
	text_color_ = Qt::black;
	//setAttribute(Qt::WA_PaintOnScreen, true);
	setAttribute(Qt::WA_NoSystemBackground, true);
	setAttribute(Qt::WA_StaticContents, true);
	//setAttribute(Qt::WA_OpaquePaintEvent, true);
}

FastLabel::FastLabel()
{
	content_msg_ = "";
	text_cache_ = "";
	text_color_ = Qt::black;
	//setAttribute(Qt::WA_PaintOnScreen, true);
	setAttribute(Qt::WA_NoSystemBackground, true);
	setAttribute(Qt::WA_StaticContents, true);
	//setAttribute(Qt::WA_OpaquePaintEvent, true);
}

FastLabel::~FastLabel()
{
}

void FastLabel::setTextColor(const QColor& color)
{
	text_color_ = color;
}

QColor FastLabel::getTextColor()
{
	return text_color_;
}

void FastLabel::resizeEvent(QResizeEvent*)
{
	update();
}

void FastLabel::paintEvent(QPaintEvent* e)
{
	QPainter painter(this);
	painter.setPen(QPen(text_color_));
	painter.drawText(this->rect(), flag_, content_msg_);
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
	content_msg_ = text;
	update();
}

