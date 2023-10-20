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

#ifndef MAPWIDGET_H
#define MAPWIDGET_H
#include <QWidget>
#include "ui_mapwidget.h"
#include <indexer.h>
#include <QShowEvent>

#define OPEN_GL_ON 1

#if OPEN_GL_ON
//#include "mapglwidget.h"
#endif

class MapWidget : public QMainWindow, public Indexer
{
	Q_OBJECT;
public:
	explicit MapWidget(long long index, QWidget* parent);

	virtual ~MapWidget();

signals:
	void on_close(long long index);
	void updateMap(const QPixmap& map, const QPoint& characterPos);

protected:
	virtual void leaveEvent(QEvent*) override;

	virtual void closeEvent(QCloseEvent* event) override;

	virtual void showEvent(QShowEvent* e) override;

private slots:
	void onFindPathFinished();

	void onRefreshTimeOut();

	void onClear();

	void onDownloadMapTimeout();

	void on_tableWidget_NPCList_cellDoubleClicked(int row, int column);

	void on_openGLWidget_notifyMouseMove(Qt::MouseButton button, const QPointF& gpos, const QPointF& pos);

	void on_openGLWidget_notifyLeftDoubleClick(const QPointF& pos);

	void on_openGLWidget_notifyRightClick();

	void on_openGLWidget_notifyLeftClick(const QPointF& gpos, const QPointF& pos);

	void on_openGLWidget_notifyLeftRelease();

	void on_openGLWidget_notifyWheelMove(const QPointF& zoom, const QPointF& pos);

	void on_pushButton_download_clicked();

	void on_pushButton_clear_clicked();

	void on_pushButton_findPath_clicked();

	void on_pushButton_returnBase_clicked();

private:
	void __fastcall downloadNextBlock();

	void __fastcall updateNpcListAllContents(const QVariant& d);

private:
	bool bClicked_ = false;
	QPointF pLast_ = { 0.0, 0.0 };
	QPointF movePoint_ = { 0.0, 0.0 };
	QRectF rectangle_src_ = { 0.0, 0.0, 0.0, 0.0 };
	QRectF rectangle_dst_ = { 0.0, 0.0, 0.0, 0.0 };

	bool isDownloadingMap_ = false;
	long long downloadMapX_ = 0;
	long long downloadMapY_ = 0;
	long long downloadMapXSize_ = 0;
	long long downloadMapYSize_ = 0;
	long long downloadCount_ = 0;
	qreal totalMapBlocks_ = 0;
	qreal downloadMapProgress_ = 0.0;

	QTimer downloadMapTimer_;

	qreal fix_zoom_value_ = 0.0;
	qreal zoom_value_ = 0.0;
	qreal scaleWidth_ = 0.0;
	qreal scaleHeight_ = 0.0;

	Ui::MapWidgetClass ui;

	QPointF curMousePos_ = { 0,0 };

	static QHash<long long, QHash<QPoint, QString>> entrances_;

	QFuture<void> findPathFuture_;

	long long counter_ = 10;

#if OPEN_GL_ON

	QTimer gltimer_;

	const long long boundaryWidth_ = 1;

#else
	QTimer timer_;
#endif
};
#endif
