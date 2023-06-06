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
class Interpreter;
class MapWidget : public QWidget
{
	Q_OBJECT;
public:
	explicit MapWidget(QWidget* parent);
	virtual ~MapWidget();

	void writeSettings();

private:
	void downloadNextBlock();
	void updateNpcListAllContents(const QVariant& d);
	void readSettings();
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
	QTimer m_timer;

	void paintEvent(QPaintEvent* pevent) override;
#endif

signals:
	void on_close(int index);
};
#endif
