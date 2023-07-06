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
