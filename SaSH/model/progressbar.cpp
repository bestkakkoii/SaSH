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
			border: 1px solid black;
			text-align: center;
			background-color: white;
		}

		QProgressBar::chunk {
			background-color: %1;
			subcontrol-position: left;
		}
	)").arg(qstrcolor);

	setStyleSheet(styleSheet);
}

ProgressBar::ProgressBar(QWidget* parent)
	: QProgressBar(parent)
{
	setTextVisible(true);
	setRange(0, 9999);
	setValue(9999);
	setAttribute(Qt::WA_StyledBackground);
	QString styleSheet = QString(R"(
		QProgressBar {
			text-align: left;
			color: black;
			background-color: white;
		}

		QProgressBar::chunk {
			background-color: #F0F0F0;
			subcontrol-position: left;
		}
	)");

	setStyleSheet(styleSheet);
	setOrientation(Qt::Horizontal);
	setAlignment(Qt::AlignTop);
	setTextVisible(true);
	setTextDirection(QProgressBar::TopToBottom);
}

void ProgressBar::setType(ProgressBar::Type type)
{
	type_ = type;
	if (kHP == type_)
		setProgressBarStyle("#FF8080");
	else
		setProgressBarStyle("#8080ff");
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
		text = QString("%1 %2,%3/%4(%5)")
			.arg(name_)
			.arg(level_)
			.arg("%v")
			.arg("%m")
			.arg("%p%");
	}
	else
	{
		text = QString("%1/%2(%3)")
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