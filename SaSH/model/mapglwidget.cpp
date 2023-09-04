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
#include "mapglwidget.h"

#pragma comment(lib, "Glu32.lib")
#pragma comment(lib, "OpenGL32.lib")

MapGLWidget::MapGLWidget(QWidget* parent)
	: QOpenGLWidget(parent)
{
	QOpenGLWidget::setAutoFillBackground(false);
}

MapGLWidget::~MapGLWidget()
{

}

void MapGLWidget::initTextures()
{

}

void MapGLWidget::initShaders()
{

}

void MapGLWidget::initCube()
{

}

void MapGLWidget::initializeGL()
{
	initializeOpenGLFunctions(); //初始化OPenGL功能函數
	glClearColor(0, 0, 0, 0);    //設置背景為黑色
	glEnable(GL_TEXTURE_2D);     //設置紋理2D功能可用
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_FLAT);
}

void MapGLWidget::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h); //設置視口
	glMatrixMode(GL_PROJECTION); //設置矩陣模式
	glLoadIdentity(); //重置矩陣
	glOrtho(0, w, h, 0, -1, 1); //設置正交投影
	glMatrixMode(GL_MODELVIEW); //設置矩陣模式
	glLoadIdentity(); //重置矩陣
}

void MapGLWidget::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //清除屏幕緩存和深度緩沖

	QPainter paintImage;
	paintImage.begin(this);

	paintImage.drawPixmap(rectangle_dst_, image_, rectangle_src_);

	//畫刷。填充幾何圖形的調色板，由顏色和填充風格組成
	QPen m_penBlue(QColor(65, 105, 225));
	QBrush m_brushBlue(QColor(65, 105, 225), Qt::SolidPattern);

	paintImage.setBrush(m_brushBlue);
	paintImage.setPen(m_penBlue);
	paintImage.drawLine(hStart_, hEnd_);//繪制橫向線
	paintImage.drawLine(vStart_, vEnd_);	//繪制縱向線

	QPen m_penRedM(QColor(225, 128, 128));
	QBrush m_brushRedM(QColor(225, 128, 128), Qt::SolidPattern);

	paintImage.setBrush(m_brushRedM);
	paintImage.setPen(m_penRedM);
	paintImage.drawLine(hCurStart_, hCurEnd_);//繪制橫向線
	paintImage.drawLine(vCurStart_, vCurEnd_);	//繪制縱向線

	QPainter rectChar;
	QPen m_penRed(Qt::red);
	QBrush m_brushRed(Qt::red, Qt::SolidPattern);

	paintImage.setPen(m_penRed);
	paintImage.drawRect(rect_);//繪制橫向線

	paintImage.end();
}

void MapGLWidget::setBackground(const QPixmap& image)
{
	if (image_ != image)
	{
		image_ = image;
		update();
	}
}

void MapGLWidget::setCurLineH(const QPointF& start, const QPointF& end)
{
	if (hCurStart_ != start)
		hCurStart_ = start;

	if (hCurEnd_ != end)
		hCurEnd_ = end;
}

void MapGLWidget::setCurLineV(const QPointF& start, const QPointF& end)
{
	if (vCurStart_ != start)
		vCurStart_ = start;

	if (vCurEnd_ != end)
		vCurEnd_ = end;
}

void MapGLWidget::setLineH(const QPointF& start, const QPointF& end)
{
	if (hStart_ != start)
		hStart_ = start;

	if (hEnd_ != end)
		hEnd_ = end;
}

void MapGLWidget::setLineV(const QPointF& start, const QPointF& end)
{
	if (vStart_ != start)
		vStart_ = start;

	if (vEnd_ != end)
		vEnd_ = end;
}

void MapGLWidget::setRect(const QRectF& rect)
{
	if (rect_ != rect)
		rect_ = rect;
}

void MapGLWidget::setPix(const QPixmap& image, const QRectF& src, const QRectF& dst)
{
	if (image_ != image)
	{
		rectangle_src_ = src;
		rectangle_dst_ = dst;
		image_ = image;
	}
}

void MapGLWidget::mouseMoveEvent(QMouseEvent* event)
{
	emit notifyMouseMove(event->button(), event->globalPos(), event->pos());
	if (bClicked_)
	{
		offest_ = event->pos() - pLast_;
		pLast_ = event->pos();
		glFlush();
		update();
	}
}

void MapGLWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		//get QCursor
		emit notifyMousePosition(event->pos());
	}
}

void MapGLWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::RightButton)
	{
		//get QCursor
		emit notifyRightClick();

	}
	else if (event->button() == Qt::LeftButton)
	{
		if (!bClicked_)
			bClicked_ = true;
		pLast_ = event->globalPos();
		emit notifyLeftClick(event->globalPos(), event->pos());
	}
}

void MapGLWidget::mouseReleaseEvent(QMouseEvent*)
{
	if (bClicked_)
		bClicked_ = false;

	emit notifyLeftRelease();
}

//滾動縮放圖片
void MapGLWidget::wheelEvent(QWheelEvent* event)
{
	emit notifyWheelMove(event->angleDelta(), event->globalPosition());
}