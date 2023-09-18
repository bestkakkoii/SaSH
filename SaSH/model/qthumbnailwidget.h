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

#ifndef QTHUMBNAILWIDGET_H
#define QTHUMBNAILWIDGET_H
#include <QGuiApplication>
#include <QOpenGLWidget>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtGui/QOpenGLFunctions_4_5_Core>
#include <QtGui/QOpenGLBuffer>
#include <QtGui/QOpenGLFramebufferObject>
#include <QtGui/QOpenGLVertexArrayObject>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLTexture>
#include <QtOpenGL/qgl.h>
#endif
#include <QOpenGLFunctions>
#include <QtCore/QTimer>
#include <GL/glu.h>
#include <GL/GL.h>

#include <QScreen>
#include <QReadWriteLock>
#include <QMouseEvent>

#pragma comment(lib, "Glu32.lib")
#pragma comment(lib, "OpenGL32.lib")

class QThumbnailWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

public:
	QThumbnailWidget(int col, int row, HWND hWnd, QWidget* parent = nullptr);
	virtual ~QThumbnailWidget();

	void setHWND(HWND hWnd);
	HWND getHWND() { QReadLocker locker(&m_lock); return hWnd_; }
protected:
	void repaintGL();
	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;

private:
	QPixmap pixmap_;
	QReadWriteLock m_lock;
	QTimer m_timer;
	HWND hWnd_ = NULL;
	GLuint textureId = 0;
	QScreen* screen = nullptr;

	QSharedPointer<QPixmap> m_pixmap = nullptr;
	QSharedPointer<QImage> m_image = nullptr;

	POINT calc(QMouseEvent* e);
	void initializeTextures();

private slots:
	void cleanup();

signals:
	void sigmousePressEvent(HWND h, POINT pos, Qt::MouseButton button);
	void sigmouseReleaseEvent(HWND h, POINT pos, Qt::MouseButton button);
	void sigmouseMoveEvent(HWND h, POINT pos);

};

#endif // QTHUMBNAILWIDGET_H