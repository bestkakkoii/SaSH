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
#include "progressbar.h"

#include <QPainterPath>

void ProgressBar::setProgressBarStyle(const QString& qstrcolor)
{
	QString styleSheet = QString(R"(
		QProgressBar {
			/*border: 1px solid transparent;*/
			/*border-radius: 5px;*/
			text-align: center;
			/*background-color: rbg(255, 255, 255);*/
			/*font-size: 10px;*/
		}

		QProgressBar::chunk {
			/*border: 1px solid transparent;*/
			/*background-color: %1;*/
			/* subcontrol-position: left; 调整chunk的位置为左侧 */
			/*border-radius: 5px;*/
		}
	)").arg(qstrcolor);

	setStyleSheet("text-align: center;");
	setOrientation(Qt::Horizontal);
	setAlignment(Qt::AlignTop);
	setTextVisible(true);
	setTextDirection(QProgressBar::TopToBottom);
}

ProgressBar::ProgressBar(QWidget* parent)
	: QProgressBar(parent)
{
	setTextVisible(true);
	setRange(0, 100);
	setValue(0);
	setAttribute(Qt::WA_StyledBackground);
	setStyleSheet("text-align: center;");
	setOrientation(Qt::Horizontal);
	setAlignment(Qt::AlignTop);
	setTextVisible(true);
	setTextDirection(QProgressBar::TopToBottom);
	//QFont font = this->font();
	//font.setPointSize(10);
	//setFont(font);
}

void ProgressBar::setType(ProgressBar::Type type)
{
	type_ = type;
	//if (kHP == type_)
	//	setProgressBarStyle("qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 rgba(255, 65, 65, 255), stop:0.556818 rgba(255, 138, 98, 255), stop:0.977273 rgba(255, 163, 107, 255), stop:1 rgba(255, 150, 100, 255))");
	//else
	//	setProgressBarStyle("qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 rgba(75, 67, 255, 255), stop:0.556818 rgba(132, 114, 255, 255), stop:0.977273 rgba(148, 143, 255, 255), stop:1 rgba(140, 130, 255, 255))");
}

void ProgressBar::setName(const QString& name)
{
	name_ = name;
}

//slots
void ProgressBar::onCurrentValueChanged(int level, int value, int maxvalue)
{
	level_ = level;
	if (maxvalue == 0)
		maxvalue = 100;

	if (value > maxvalue)
	{
		value = 0;
		maxvalue = 100;
	}

	setMaximum(maxvalue);
	setValue(value);
	QString text;
	if (kHP == type_)
	{
		text = QString("%1lv%2,hp%3/%4(%5)")
			.arg(name_)
			.arg(level_)
			.arg("%v")
			.arg("%m")
			.arg("%p%");
	}
	else
	{
		text = QString("mp%1/%2(%3)")
			.arg("%v")
			.arg("%m")
			.arg("%p%");
	}
	setFormat(text);
}

void ProgressBar::paintEvent(QPaintEvent* event)
{
	QProgressBar::paintEvent(event);
}