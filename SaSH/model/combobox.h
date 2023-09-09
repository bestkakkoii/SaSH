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
#if _MSC_VER >= 1600 
#pragma execution_character_set("utf-8") 
#endif

#include <QComboBox>
#include <QMouseEvent>
class ComboBox : public QComboBox
{
	Q_OBJECT  //不寫這個沒辦法用信號槽機制，必須得寫
public:
	explicit ComboBox(QWidget* parent = 0);
	virtual ~ComboBox();

	void addItem(const QString& text, const QVariant& userData = QVariant());
	void addItem(const QIcon& icon, const QString& text, const QVariant& userData = QVariant());
	void addItems(const QStringList& texts);
	void setCurrentIndex(int index);
	void clear();
	void setItemData(int index, const QVariant& value, int role = Qt::UserRole + 1);

	void setDisableFocusCheck(bool disableFocusCheck) { disableFocusCheck_ = disableFocusCheck; }
protected:
	virtual void mousePressEvent(QMouseEvent* e);  //重寫鼠標點擊事件


signals:
	void clicked();  //自定義點擊信號，在mousePressEvent事件發生時觸發，名字無所謂，隨自己喜歡就行

private:
	bool disableFocusCheck_ = false;
};
#endif // COMBOBOX_H
