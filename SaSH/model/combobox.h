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
#ifndef COMBOBOX_H
#define COMBOBOX_H
#pragma execution_character_set("utf-8")
#include <QComboBox>
#include <QMouseEvent>
class ComboBox :
	public QComboBox
{
	Q_OBJECT  //不寫這個沒辦法用信號槽機制，必須得寫
public:
	explicit ComboBox(QWidget* parent = 0);
	virtual ~ComboBox();
protected:
	virtual void mousePressEvent(QMouseEvent* e);  //重寫鼠標點擊事件

signals:
	void clicked();  //自定義點擊信號，在mousePressEvent事件發生時觸發，名字無所謂，隨自己喜歡就行
};
#endif // COMBOBOX_H
