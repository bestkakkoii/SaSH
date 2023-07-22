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

#pragma once
#include <QOpenGLWidget>
#include <QtOpenGL/qgl.h>
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
	explicit MapGLWidget(QWidget* parent = nullptr);
	virtual ~MapGLWidget();

	void __vectorcall setCurLineH(const QPointF& start, const QPointF& end);
	void __vectorcall setCurLineV(const QPointF& start, const QPointF& end);
	void __vectorcall setLineH(const QPointF& start, const QPointF& end);
	void __vectorcall setLineV(const QPointF& start, const QPointF& end);
	void __vectorcall setRect(const QRectF& rect);
	void __vectorcall setPix(const QPixmap& image, const QRectF& src, const QRectF& dst);

protected:

	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;  //雙擊
	void mousePressEvent(QMouseEvent* event) override;  //按下
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private:
	void updateImageTexture();
	void drawImage();


public slots:
	void initializeGL() override;

	void resizeGL(int w, int h) override;

	void paintGL() override;

	void setBackground(const QPixmap& image);

	void initTextures();

	void initShaders();

signals:
	void notifyMousePosition(const QPointF& pos);

	void notifyMouseMove(Qt::MouseButton button, const QPointF& gpos, const QPointF& pos);

	void notifyRightClick();

	void notifyLeftRelease();

	void notifyLeftClick(const QPointF& gpos, const QPointF& pos);

	void notifyWheelMove(const QPointF& zoom, const QPointF& pos);

private:
	bool bClicked_ = false;

	QPointF pLast_ = { 0.0, 0.0 };
	QPointF movePoint_ = { 0.0, 0.0 };
	QPointF offest = { 0.0, 0.0 };

	QVector<QVector3D> vertices;

	QVector<QVector2D> texCoords;

	QOpenGLShaderProgram program;

	QOpenGLTexture* texture = nullptr;
	QOpenGLShader* vshader = nullptr;
	QOpenGLShader* fshader = nullptr;

	QMatrix4x4 projection;

	QPixmap m_image;

	GLfloat scaleWidth_ = 0.0;
	GLfloat scaleHeight_ = 0.0;

	QPointF m_vStart = { 0.0, 0.0 };
	QPointF m_vEnd = { 0.0, 0.0 };

	QPointF m_hStart = { 0.0, 0.0 };
	QPointF m_hEnd = { 0.0, 0.0 };

	QPointF m_vCurStart = { 0.0, 0.0 };
	QPointF m_vCurEnd = { 0.0, 0.0 };

	QPointF m_hCurStart = { 0.0, 0.0 };
	QPointF m_hCurEnd = { 0.0, 0.0 };

	QRectF m_rect = { 0.0, 0.0, 0.0, 0.0 };

	QRectF rectangle_dst_ = { 0.0, 0.0, 0.0, 0.0 };
	QRectF rectangle_src_ = { 0.0, 0.0, 0.0, 0.0 };

	GLuint m_imageTexture = 0;
};
