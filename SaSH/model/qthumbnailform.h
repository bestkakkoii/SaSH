/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2024 All Rights Reserved.
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
#ifndef QTHUMBNAILFORM_H
#define QTHUMBNAILFORM_H
#include <QWidget>
#include "ui_qthumbnailform.h"
#include "qthumbnailwidget.h"
#include <QTimer>
#include <QPair>

class QThumbnailForm : public QWidget
{
	Q_OBJECT

public:
	explicit QThumbnailForm(const QList<HWND>& v, QWidget* parent = nullptr);
	virtual ~QThumbnailForm() { timer.stop(); }

	void initThumbnailWidget(const QList<HWND>& v);

private:
	Ui::QThumbnailFormClass ui;

	QTimer timer;
	QVector<QThumbnailWidget*> m_glWidgets;
	QList<HWND> m_hWnds;
	QPair<int, int> count = { 0, 0 };
	QReadWriteLock m_lock;
	bool isInit = false;
	void refresh();

	void RemoteMouseMove(HWND hWnd, const POINT& point);
	void RemoteMouseRelease(HWND hWnd, const POINT& point, Qt::MouseButton button);
	void RemoteMousePress(HWND hWnd, const POINT& point, Qt::MouseButton button);
	void RemoteRun(HWND h, const POINT& point, Qt::MouseButton button, bool isrelease);

	int findbyIndex(QList<HWND> v, int index, HWND* phWnd);
protected:
	//void mousePressEvent(QMouseEvent* event) override;
	//void mouseMoveEvent(QMouseEvent* event) override;
	//void mouseReleaseEvent(QMouseEvent* event) override;

signals:
	void finishedRefresh();
	void reset();

public slots:
	void start();

private slots:
	void on_mousePressEvent(HWND h, POINT pos, Qt::MouseButton button);
	void on_mouseReleaseEvent(HWND h, POINT pos, Qt::MouseButton button);
	void on_mouseMoveEvent(HWND h, POINT pos);
};

#endif // QTHUMBNAILFORM_H
