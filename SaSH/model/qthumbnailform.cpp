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
#include "qthumbnailform.h"


QThumbnailForm::QThumbnailForm(const QList<HWND>& v, QWidget* parent)
	: QWidget(parent)
	, m_glWidgets(50)
	, m_hWnds(v)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);
	//此窗口背景透明
	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::WindowTransparentForInput);
	setAttribute(Qt::WA_TranslucentBackground, true);

	//stylesheet透明
	setStyleSheet("background:transparent;");

	//當前螢幕解析度
	QScreen* screen = QGuiApplication::primaryScreen();
	setFixedSize(screen->size());
	int row = 0;
	int col = 0;
	int size = m_glWidgets.size();

	for (int i = 0; i < size; ++i)
	{
		if (i < v.size())
			m_glWidgets[i] = q_check_ptr(new QThumbnailWidget(col, row, v.value(i), this));
		else
			m_glWidgets[i] = q_check_ptr(new QThumbnailWidget(col, row, nullptr, this));

		connect(m_glWidgets[i], &QThumbnailWidget::sigmouseMoveEvent, this, &QThumbnailForm::on_mouseMoveEvent, Qt::UniqueConnection);
		connect(m_glWidgets[i], &QThumbnailWidget::sigmousePressEvent, this, &QThumbnailForm::on_mousePressEvent, Qt::UniqueConnection);
		connect(m_glWidgets[i], &QThumbnailWidget::sigmouseReleaseEvent, this, &QThumbnailForm::on_mouseReleaseEvent, Qt::UniqueConnection);

		col++;
		if (col >= 9)
		{
			col = 0;
			row++;
		}
	}

	connect(&timer, &QTimer::timeout, this, &QThumbnailForm::refresh);
	timer.start(300);
	refresh();
}

void QThumbnailForm::initThumbnailWidget(const QList<HWND>& v)
{
	QWriteLocker locker(&m_lock);
	m_hWnds = v;
}

void QThumbnailForm::start()
{
	show();
	refresh();
}

void QThumbnailForm::refresh()
{
	QReadLocker locker(&m_lock);
	QList<HWND> v = m_hWnds;
	QVector<QThumbnailWidget*> list = m_glWidgets;
	int thread_size = v.size();
	int widget_size = list.size();


	//多餘的widget隱藏
	if (widget_size > thread_size)
	{
		//從後面
		for (int i = widget_size - 1; i >= thread_size; i--)
		{
			list.value(i)->setHWND(NULL);
			list.value(i)->hide();
		}
	}

	int n = 0;
	for (const auto& it : v)
	{
		QThumbnailWidget* nail = list.value(n++);
		nail->setHWND(it);
		nail->show();
	}

	emit finishedRefresh();
}

void QThumbnailForm::RemoteMouseMove(HWND hWnd, const POINT& point)
{
	SendMessage(hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(point.x, point.y));
}

void QThumbnailForm::RemoteMouseRelease(HWND hWnd, const POINT& point, Qt::MouseButton button)
{
	// Check the type of button that was released
	if (button == Qt::LeftButton)
	{
		// Send a WM_LBUTTONUP message to the target window
		SendMessage(hWnd, WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(point.x, point.y));
	}
	else if (button == Qt::RightButton)
	{
		// Send a WM_RBUTTONUP message to the target window
		SendMessage(hWnd, WM_RBUTTONUP, MK_RBUTTON, MAKELPARAM(point.x, point.y));
	}
	else if (button == Qt::MiddleButton)
	{
		// Send a WM_MBUTTONUP message to the target window
		SendMessage(hWnd, WM_MBUTTONUP, MK_MBUTTON, MAKELPARAM(point.x, point.y));
	}
}

void QThumbnailForm::RemoteMousePress(HWND hWnd, const POINT& point, Qt::MouseButton button)
{
	// Check the type of button that was pressed
	if (button == Qt::LeftButton)
	{
		// Send a WM_LBUTTONDOWN message to the target window
		SendMessage(hWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(point.x, point.y));
	}
	else if (button == Qt::RightButton)
	{
		// Send a WM_RBUTTONDOWN message to the target window
		SendMessage(hWnd, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(point.x, point.y));
	}
	else if (button == Qt::MiddleButton)
	{
		// Send a WM_MBUTTONDOWN message to the target window
		SendMessage(hWnd, WM_MBUTTONDOWN, MK_MBUTTON, MAKELPARAM(point.x, point.y));
	}
}

int findbyName(QList<HWND> v, const QString& name, HWND* phWnd)
{
	////查找 ->GetCharData().name
	//int size = v.size();
	//for (int i = 0; i < size; ++i)
	//{
	//	if (!v.value(i)) continue;
	//	if (v.value(i)->GetCharData().name == name)
	//	{
	//		if (phWnd)
	//			*phWnd = v.value(i);
	//		return i;
	//	}
	//}
	return -1;
};

int QThumbnailForm::findbyIndex(QList<HWND> v, int index, HWND* phWnd)
{
	//int size = v.size();
	//for (int i = 0; i < size; ++i)
	//{
	//	if (v.value(i)) continue;
	//	if (v.value(i)->GetIndex() == index)
	//	{
	//		if (phWnd)
	//			*phWnd = v.valuet(i);
	//		return i;
	//	}
	//}
	QReadLocker locker(&m_lock);
	if (index < 0 || index >= m_hWnds.size())
		return -1;

	if (phWnd)
		*phWnd = m_hWnds.value(index);

	return index;
};

void QThumbnailForm::RemoteRun(HWND h, const POINT& point, Qt::MouseButton button, bool isrelease)
{
	QReadLocker locker(&m_lock);
	QList<HWND> v = m_hWnds;
	auto run = [this, &point, button, isrelease](HWND hWnd)
	{
		if (button == Qt::MouseButton::NoButton)
			RemoteMouseMove(hWnd, point);
		else if ((button != Qt::MouseButton::NoButton) && !isrelease)
			RemoteMousePress(hWnd, point, button);
		else if ((button != Qt::MouseButton::NoButton) && isrelease)
			RemoteMouseRelease(hWnd, point, button);
	};

	//沒有//setoc 
	run(h);
}

void QThumbnailForm::on_mousePressEvent(HWND h, POINT point, Qt::MouseButton button)
{
	RemoteRun(h, point, button, false);
}

void QThumbnailForm::on_mouseReleaseEvent(HWND h, POINT point, Qt::MouseButton button)
{
	RemoteRun(h, point, button, true);
}
void QThumbnailForm::on_mouseMoveEvent(HWND h, POINT point)
{
	RemoteRun(h, point, Qt::NoButton, false);
}