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
#include <QOpenGLWidget>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtOpenGL/qgl.h>
#endif
#include <QWheelEvent>
#include <GL/GLU.h>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QtGui>
#include <QtGui/qopenglcontext.h>

class MapGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

public:
	enum
	{
		Left_Bottom_X,
		Left_Bottom_Y,
		Right_Bottom_X,
		Right_Bottom_Y,
		Right_Top_X,
		Right_Top_Y,
		Left_Top_X,
		Left_Top_Y,
		Pos_Max
	};

	explicit MapGLWidget(QWidget* parent = nullptr);
	virtual ~MapGLWidget();

	void setCurLineH(const QPointF& start, const QPointF& end);
	void setCurLineV(const QPointF& start, const QPointF& end);
	void setLineH(const QPointF& start, const QPointF& end);
	void setLineV(const QPointF& start, const QPointF& end);
	void setRect(const QRectF& rect);
	void setPix(const QPixmap& image, const QRectF& src, const QRectF& dst);

protected:

	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;  //雙擊
	void mousePressEvent(QMouseEvent* event) override;  //按下
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private:

protected:
	virtual void initializeGL() override;

	virtual void resizeGL(int w, int h) override;

	virtual void paintGL() override;


signals:
	void notifyLeftDoubleClick(const QPointF& pos);

	void notifyMouseMove(Qt::MouseButton button, const QPointF& gpos, const QPointF& pos);

	void notifyRightClick();

	void notifyLeftRelease();

	void notifyLeftClick(const QPointF& gpos, const QPointF& pos);

	void notifyWheelMove(const QPointF& zoom, const QPointF& pos);


private:


private:
	bool bClicked_ = false;

	QPointF pLast_ = { 0.0, 0.0 };
	QPointF movePoint_ = { 0.0, 0.0 };
	QPointF offest_ = { 0.0, 0.0 };

	QPixmap image_;

	GLfloat scaleWidth_ = 0.0;
	GLfloat scaleHeight_ = 0.0;

	QPointF vStart_ = { 0.0, 0.0 };
	QPointF vEnd_ = { 0.0, 0.0 };

	QPointF hStart_ = { 0.0, 0.0 };
	QPointF hEnd_ = { 0.0, 0.0 };

	QPointF vCurStart_ = { 0.0, 0.0 };
	QPointF vCurEnd_ = { 0.0, 0.0 };

	QPointF hCurStart_ = { 0.0, 0.0 };
	QPointF hCurEnd_ = { 0.0, 0.0 };

	QRectF rect_ = { 0.0, 0.0, 0.0, 0.0 };

	QRectF rectangle_dst_ = { 0.0, 0.0, 0.0, 0.0 };
	QRectF rectangle_src_ = { 0.0, 0.0, 0.0, 0.0 };
};
