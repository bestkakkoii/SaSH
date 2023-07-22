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
#include <QObject>
#include <QProgressBar>

class QString;
class QPaintEvent;

class ProgressBar : public QProgressBar
{
	Q_OBJECT
public:
	enum Type
	{
		kHP,
		kMP,
	};

	explicit ProgressBar(QWidget* parent = nullptr);
	void setType(ProgressBar::Type type);
	void setName(const QString& name);

public slots:
	void onCurrentValueChanged(int level, int value, int maxvalue);

protected:
	//void paintEvent(QPaintEvent* event) override;

private:
	int level_ = 0;
	Type type_ = kHP;
	QString name_ = "";
};