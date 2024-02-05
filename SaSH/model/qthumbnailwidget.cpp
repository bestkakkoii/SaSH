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

#include "stdafx.h"
#include "qthumbnailwidget.h"
#include <QPixmapCache>

void QThumbnailWidget::setHWND(HWND hWnd)
{
	QReadLocker locker(&m_lock);
	if (hWnd_ == hWnd)
	{
		if (!m_timer.isActive())
		{
			m_timer.start(150);
		}
		return;
	}

	hWnd_ = hWnd;
	if (hWnd)
	{
		if (!m_timer.isActive())
		{
			m_timer.start(150);
		}
	}
	else
		m_timer.stop();
}

POINT QThumbnailWidget::calc(QMouseEvent* e)
{
	// Get the current cursor position in screen coordinates
	QPoint pos = e->pos();

	// Convert the coordinates to a POINT struct
	POINT point{ pos.x(), pos.y() };

	// Get the aspect ratio of the current window
	int windowWidth = this->width();
	int windowHeight = this->height();
	float currentAspectRatio = (float)windowWidth / (float)windowHeight;

	// Get the aspect ratio of the target window
	RECT targetRect;
	GetClientRect(getHWND(), &targetRect);
	float targetWidth = (float)(targetRect.right - targetRect.left);
	float targetHeight = (float)(targetRect.bottom - targetRect.top);
	float targetAspectRatio = targetWidth / targetHeight;

	// Calculate the new coordinates based on the aspect ratio difference
	if (currentAspectRatio > targetAspectRatio)
	{
		point.x = (float)point.x * targetWidth / windowWidth;
		point.y = (float)point.y * targetWidth / windowWidth * targetAspectRatio / currentAspectRatio;
	}
	else
	{
		point.x = (float)point.x * targetHeight / windowHeight * currentAspectRatio / targetAspectRatio;
		point.y = (float)point.y * targetHeight / windowHeight;
	}
	return point;
}

void QThumbnailWidget::mousePressEvent(QMouseEvent* e)
{
	POINT point = calc(e);
	emit sigmousePressEvent(getHWND(), point, e->button());
}

void QThumbnailWidget::mouseReleaseEvent(QMouseEvent* e)
{
	POINT point = calc(e);
	emit sigmouseReleaseEvent(getHWND(), point, e->button());
}

void QThumbnailWidget::mouseMoveEvent(QMouseEvent* e)
{
	POINT point = calc(e);
	emit sigmouseMoveEvent(getHWND(), point);
}

QThumbnailWidget::QThumbnailWidget(int col, int row, HWND hWnd, QWidget* parent)
	: QOpenGLWidget(parent)
	, hWnd_(hWnd)
	, screen(QGuiApplication::primaryScreen())
{
	QOpenGLWidget::setAutoFillBackground(false);
	constexpr int w = 4 * 50;
	constexpr int h = 3 * 50;
	setFixedSize(w, h);
	move(col * (w + 1), row * (h + 5));
	//透明度
	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);//  | Qt::X11BypassWindowManagerHint
	setStyleSheet("background:transparent;");
	//setWindowOpacity(0.8);

	//滑鼠追蹤
	setMouseTracking(true);

	connect(&m_timer, &QTimer::timeout, this, [this]() { repaintGL(); });
}

void QThumbnailWidget::cleanup()
{
	m_timer.stop();
	makeCurrent();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	m_texture.reset();
	m_program.reset();
#endif
	if (textureId)
		glDeleteTextures(1, &textureId);
	doneCurrent();
}

QThumbnailWidget::~QThumbnailWidget()
{
	cleanup();
}

void QThumbnailWidget::initializeGL()
{
	initializeOpenGLFunctions();
	initializeTextures();
}

void QThumbnailWidget::initializeTextures()
{
	if (textureId)
	{
		glDeleteTextures(1, &textureId);
		textureId = 0;
	}
	glGenTextures(1, &textureId);
	if (textureId)
		glBindTexture(GL_TEXTURE_2D, textureId);
	// 設置紋理選項
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void QThumbnailWidget::resizeGL(int w, int h)
{
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
}

void QThumbnailWidget::repaintGL()
{
	WId wid = (WId)getHWND();
	if (!wid) { m_timer.stop(); return; }
	try
	{
		pixmap_ = screen->grabWindow(wid);
		if (pixmap_.isNull()) { m_timer.stop(); return; }

		QPixmapCache::insert("image_key" + util::toQString((int)wid), pixmap_);
	}
	catch (...)
	{
		close(); return;
	}

	update();
}


void QThumbnailWidget::paintGL()
{
	if (!screen) { m_timer.stop(); return; }
	WId wid = (WId)getHWND();
	if (!wid) { m_timer.stop(); return; }

	QPixmap cachedPixmap;
	if (!QPixmapCache::find("image_key" + util::toQString((int)wid), &cachedPixmap))
	{
		try
		{
			QPixmap pixmap = screen->grabWindow(wid);
			if (pixmap.isNull()) { m_timer.stop(); return; }

			QPixmapCache::insert("image_key" + util::toQString((int)wid), pixmap);
			cachedPixmap = pixmap;
		}
		catch (...)
		{
			close(); return;
		}
	}

	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, width(), height());

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width(), height(), 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	initializeTextures();
	QImage image = cachedPixmap.toImage();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, image.bits());

	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(width(), 0.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex2f(width(), height());
	glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, height());
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
}