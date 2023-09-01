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
#include <QShowEvent>

#define OPEN_GL_ON 1

#if OPEN_GL_ON
//#include "mapglwidget.h"
#endif
class Interpreter;
class MapWidget : public QMainWindow
{
	Q_OBJECT;
public:
	explicit MapWidget(QWidget* parent);
	virtual ~MapWidget();

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
	int downloadMapX_ = 0;
	int downloadMapY_ = 0;
	int downloadMapXSize_ = 0;
	int downloadMapYSize_ = 0;
	int downloadCount_ = 0;
	qreal totalMapBlocks_ = 0;
	qreal downloadMapProgress_ = 0.0;

	QTimer downloadMapTimer_;

	qreal fix_zoom_value_ = 0.0;
	qreal zoom_value_ = 0.0;
	qreal scaleWidth_ = 0.0;
	qreal scaleHeight_ = 0.0;

	Ui::MapWidgetClass ui;

	QScopedPointer<Interpreter> interpreter_;

	QPointF curMousePos_ = { 0,0 };

	int counter_ = 10;

#if OPEN_GL_ON

	QTimer gltimer_;

	int boundaryWidth_ = 4;
	QPoint clickPos_;

	//qreal zoom_value_ = 0.0;
protected:
	void leaveEvent(QEvent*) override;

	void closeEvent(QCloseEvent* event) override;

	int paintEngine() { return 0; }

	void showEvent(QShowEvent* e) override
	{
		setAttribute(Qt::WA_Mapped);
		QWidget::showEvent(e);
	}

	bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;

	void mousePressEvent(QMouseEvent* e)  override
	{
		if (e->button() == Qt::LeftButton)
			clickPos_ = e->pos();
	}
	void mouseMoveEvent(QMouseEvent* e) override
	{
		if (e->buttons() & Qt::LeftButton)
			move(e->pos() + pos() - clickPos_);
	}

private slots:
	void onRefreshTimeOut();
	void onClear();

	void onDownloadMapTimeout();

	void on_tableWidget_NPCList_cellDoubleClicked(int row, int column);

	void on_openGLWidget_notifyMouseMove(Qt::MouseButton button, const QPointF& gpos, const QPointF& pos);
	void on_openGLWidget_notifyMousePosition(const QPointF& pos);
	void on_openGLWidget_notifyRightClick();
	void on_openGLWidget_notifyLeftClick(const QPointF& gpos, const QPointF& pos);
	void on_openGLWidget_notifyLeftRelease();
	void on_openGLWidget_notifyWheelMove(const QPointF& zoom, const QPointF& pos);

	void on_pushButton_download_clicked();
	void on_pushButton_findPath_clicked();
	void on_pushButton_returnBase_clicked();
#else
protected:
	QTimer timer_;

	void paintEvent(QPaintEvent* pevent) override;
#endif

signals:
	void on_close(int index);
};
#endif
