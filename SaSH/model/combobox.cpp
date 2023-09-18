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
#if 0
	this->setStyleSheet(R"(
        /* 未下拉时，QComboBox的样式 */
        QComboBox {
            border: 1px solid #424242;   /* 边框 */
            border-radius: 0px;   /* 圆角 */
            /*padding: 1px 13px 1px 3px;    字体填衬 */
            color: rgb(250,250,250);
            /*font: normal normal 15px "Microsoft YaHei"*/;
            background:#383838;
        }

        QComboBox::hover {
            border: 1px solid #424242;   /* 边框 */
            border-radius: 0px;   /* 圆角 */
            /*padding: 1px 18px 1px 3px;    字体填衬 */
            color: rgb(250,250,250);
            /*font: normal normal 15px "Microsoft YaHei"*/;
            background:#3D3D3D;
        }

        /* 下拉后，整个下拉窗体样式 */
        QComboBox QAbstractItemView {
            outline: 0px solid #1F1F1F;   /* 选定项的虚框 */
            border: 1px solid #424242;    /* 整个下拉窗体的边框 */
            color: #FAFAFA;
            background-color: #2E2E2E;   /* 整个下拉窗体的背景色 */
            selection-background-color: #3D3D3D;   /* 整个下拉窗体被选中项的背景色 */
        }

        /* 下拉后，整个下拉窗体每项的样式 */
        QComboBox QAbstractItemView::item {
            height: 15px;  /* 项的高度（设置pComboBox->setView(new QListView());后，该项才起作用） */
	        border: 0px solid #707070;
	        background-color: #2E2E2E;
        }

        /* 下拉后，整个下拉窗体越过每项的样式 */
        QComboBox QAbstractItemView::item:hover {
            color: #FAFAFA;
	        border: 1px solid #707070;
            background-color:  #3D3D3D;   /* 整个下拉窗体越过每项的背景色 */
        }

        /* 下拉后，整个下拉窗体被选择的每项的样式 */
        QComboBox QAbstractItemView::item:selected {
            color: #FAFAFA;
	        /*border: 1px solid #707070;*/
            background-color:  #3D3D3D;
        }

        QComboBox QAbstractScrollArea QScrollBar:vertical {
            background: #2E2E2E;
	        width:  15px;
	        min-height: 60px;
        }

        QComboBox QAbstractScrollArea QScrollBar::handle:vertical {
	        background: #4D4D4D;
  	        border: 3px solid  #2E2E2E;
	        width:  15px;
	        min-height: 60px;
        }

        QComboBox QAbstractScrollArea QScrollBar::handle:hover:vertical,
        QComboBox QAbstractScrollArea QScrollBar::handle:pressed:vertical {
            background: #999999;
        }

        QComboBox QAbstractScrollArea QScrollBar::sub-page:vertical {
            background: 444444;
        }

        QComboBox QAbstractScrollArea QScrollBar::add-page:vertical {
            background: 5B5B5B;
        }

        QComboBox QAbstractScrollArea QScrollBar::add-line:vertical {
            background: none;
        }

        QComboBox QAbstractScrollArea QScrollBar::sub-line:vertical {
            background: none;
        }

        /* 设置为可编辑（setEditable(true)）editable时，编辑区域的样式 */
        QComboBox:editable {
            background: #383838;
        }

        /* 设置为非编辑（setEditable(false)）!editable时，整个QComboBox的样式 */
        QComboBox:!editable {
            background:#383838;
        }

        /* 设置为可编辑editable时，点击整个QComboBox的样式 */
        QComboBox:editable:on {
            background:#383838;
        }

        /* 设置为非编辑!editable时，点击整个QComboBox的样式 */
        QComboBox:!editable:on {
            background:#383838;
        }

        /* 设置为可编辑editable时，下拉框的样式 */
        QComboBox::drop-down:editable {
            background:#383838;
        }

        /* 设置为可编辑editable时，点击下拉框的样式 */
        QComboBox::drop-down:editable:on {
            background:#383838;
        }

        /* 设置为非编辑!editable时，下拉框的样式 */
        QComboBox::drop-down:!editable {
            background: #383838;
        }

        /* 设置为非编辑!editable时，点击下拉框的样式 */
        QComboBox::drop-down:!editable:on {
            background:#383838;
        }

        /* 点击QComboBox */
        QComboBox:on {
        }

        /* 下拉框样式 */
        QComboBox::drop-down {
            subcontrol-origin: padding;   /* 子控件在父元素中的原点矩形。如果未指定此属性，则默认为padding。 */
            subcontrol-position: top right;   /* 下拉框的位置（右上） */
            width: 15px;   /* 下拉框的宽度 */

            border-left-width: 0px;   /* 下拉框的左边界线宽度 */
            border-left-color: #383838;   /* 下拉框的左边界线颜色 */
            border-left-style: solid;   /* 下拉框的左边界线为实线 */
            border-top-right-radius: 0px;   /* 下拉框的右上边界线的圆角半径（应和整个QComboBox右上边界线的圆角半径一致） */
            border-bottom-right-radius: 0px;   /* 同上 */
        }
　        /* 越过下拉框样式 */

　        QComboBox::drop-down:hover {
　　　        background:#3D3D3D;

　        }
        /* 下拉箭头样式 */ QComboBox::down-arrow {
         width: 15px; /* 下拉箭头的宽度（建议与下拉框drop-down的宽度一致） */
        background: transparent; /* 下拉箭头的的背景色 */
        padding: 0px 0px 0px 0px; /* 上内边距、右内边距、下内边距、左内边距 */
        image: url(:/image/icon_down_arrow.png);
        }

        /* 点击下拉箭头 */
         QComboBox::down-arrow:on {
	        image: url(:/image/icon_up_arrow.png);
        }
	)");
#endif
	setStyleSheet(R"(
QComboBox QAbstractItemView { 
    min-width: 200px;
    /*border-radius: 3px;
    border: 0px solid #ccc;*/
}
)");
	setView(q_check_ptr(new QListView()));
	view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	//setFixedHeight(20);
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