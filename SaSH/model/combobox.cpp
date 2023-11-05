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
#include <combobox.h>
#include <QListView>

ComboBox::ComboBox(QWidget* parent) :QComboBox(parent)
{
	QString styleStr = R"(
QComboBox {
	color:black;
	background-color: white;
    border: 1px solid gray;
	border-radius: 3px;
}

QComboBox:hover {
    border: 1px solid #4096FF;
}

QComboBox QAbstractItemView { 
	color:black;
	background-color: white;
	border: 1px solid gray;
	border-radius: 3px;
    min-width: 200px;
}

QComboBox::drop-down {
	color:black;
	background-color: white;
    width: 15px;
	/* 
	subcontrol-origin: padding;
    subcontrol-position: top right;
    border-left-width: 1px;
    border-left-color: darkgray;
    border-left-style: solid;
    border-top-right-radius: 2px;
    border-bottom-right-radius: 2px;
	*/
}

QComboBox::down-arrow {
    image: url(:/image/icon_downarrow.svg);
	width:12px;
	height:12px;
}


QComboBox::down-arrow:on { /* shift the arrow when popup is open */
    top: 1px;
    left: 1px;
}

QListView{
	color:black;
	background-color: white;
	border: 1px solid gray;
	border-radius: 10px;
	outline:0px;
}

QListView:item{
	color:black;
	background-color: white;
	border: 3px solid white;
	border-radius: 10px;
}

QListView:item:hover{
	color:black;
	border: 3px solid white;
	background-color: #E6F4FF;
}

QScrollBar:vertical {
	min-height: 30px;  
    background-color: white;
}

QScrollBar::handle:vertical {
    background-color: #3282F6;
  	border: 3px solid white;
	min-height:30px;
}

QScrollBar::handle:hover:vertical,
QScrollBar::handle:pressed:vertical {
    background-color: #3487FF;
}
)";

	setStyleSheet(styleStr);
	setAttribute(Qt::WA_StyledBackground);

	QListView* pview = q_check_ptr(new QListView(this));
	sash_assume(pview != nullptr);
	if (nullptr != pview)
	{
		pview->setStyleSheet(styleStr);
		pview->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	}
	setView(pview);
	view()->move(0, 40);
	setFixedHeight(19);
}

ComboBox::~ComboBox()
{
}

void ComboBox::mousePressEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)//判斷是不是鼠標左鍵按下了
	{
		disableFocusCheck_ = true;
		emit clicked();//是的話就發送咱們定義的信號
	}
	QComboBox::mousePressEvent(e);//如果你不寫這一句，事件傳遞到上一步就停止，
	//就不接著向下發了，父類也就沒辦法處理點擊事件了
}

void ComboBox::addItem(const QString& text, const QVariant& userData)
{
	if (!hasFocus() || disableFocusCheck_)
	{
		blockSignals(true);
		QComboBox::addItem(text, userData);
		blockSignals(false);
	}
}

void ComboBox::addItem(const QIcon& icon, const QString& text, const QVariant& userData)
{
	if (!hasFocus() || disableFocusCheck_)
	{
		blockSignals(true);
		QComboBox::addItem(icon, text, userData);
		blockSignals(false);
	}
}

void ComboBox::addItems(const QStringList& texts)
{
	if (!hasFocus() || disableFocusCheck_)
	{
		blockSignals(true);
		QComboBox::addItems(texts);
		blockSignals(false);
	}
}

void ComboBox::setCurrentIndex(int index)
{
	if (!hasFocus() || disableFocusCheck_)
	{
		blockSignals(true);
		QComboBox::setCurrentIndex(index);
		blockSignals(false);
	}
}

void ComboBox::clear()
{
	if (!hasFocus() || disableFocusCheck_)
	{
		blockSignals(true);
		QComboBox::clear();
		blockSignals(false);
	}
}

void ComboBox::setItemData(int index, const QVariant& value, int role)
{
	if (!hasFocus() || disableFocusCheck_)
	{
		blockSignals(true);
		QComboBox::setItemData(index, value, role);
		blockSignals(false);
	}
}