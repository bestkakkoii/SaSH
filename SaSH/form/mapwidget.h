#pragma once
#ifndef MAPWIDGET_H
#define MAPWIDGET_H
#include <QWidget>
#include "ui_mapwidget.h"
#include <QShowEvent>

#define OPEN_GL_ON 1

#if OPEN_GL_ON
//#include "mapglwidget.h"
#endif

class MapWidget : public QWidget
{
	Q_OBJECT;
public:
	explicit MapWidget(QWidget* parent);
	virtual ~MapWidget();

	void writeSettings();
private:

	bool m_bClicked = false;
	QPointF pLast = { 0.0, 0.0 };
	QPointF m_MovePoint = { 0.0, 0.0 };
	QRectF m_rectangle_src = { 0.0, 0.0, 0.0, 0.0 };
	QRectF m_rectangle_dst = { 0.0, 0.0, 0.0, 0.0 };

	bool m_IsDownloadingMap = false;
	int m_downloadMapX = 0;
	int m_downloadMapY = 0;
	int m_downloadMapXSize = 0;
	int m_downloadMapYSize = 0;
	qreal m_downloadMapProgress = 0.0;

	QTimer downloadMapTimer;

	qreal m_fix_zoom_value = 0.0;
	qreal m_zoom_value = 0.0;
	qreal m_scaleWidth = 0.0;
	qreal m_scaleHeight = 0.0;

	Ui::MapWidgetClass ui;

	QPointF m_curMousePos = { 0,0 };

	int m_counter = 10;

	void on_NPCListUpdateAllContent(const QVariant& d);

	void readSettings();

#if OPEN_GL_ON

	QTimer gltimer;

	//qreal m_zoom_value = 0.0;
protected:
	void leaveEvent(QEvent*) override;

	void closeEvent(QCloseEvent* event) override;

	int paintEngine() { return 0; }

	void showEvent(QShowEvent* e) override
	{
		setAttribute(Qt::WA_Mapped);
		QWidget::showEvent(e);
	}
private slots:
	void on_tableWidget_NPCList_cellDoubleClicked(int row, int column);

	void on_timeOut();
	void on_clear();

	void on_downloadMapTimeout();

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
	QTimer m_timer;

	void paintEvent(QPaintEvent* pevent) override;
#endif

signals:
	void on_close(int index);
};
#endif
