#ifndef QTHUMBNAILWIDGET_H
#define QTHUMBNAILWIDGET_H
#include <QGuiApplication>
#include <QOpenGLWidget>
#include <QtGui/QOpenGLFunctions_4_5_Core>
#include <QtGui/QOpenGLBuffer>
#include <QtGui/QOpenGLFramebufferObject>
#include <QtGui/QOpenGLVertexArrayObject>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLTexture>
#include <QtCore/QTimer>
#include <QtOpenGL/qgl.h>
#include <GL/glu.h>
#include <GL/GL.h>

#include <QScreen>
#include <QReadWriteLock>
#include <QMouseEvent>

#pragma comment(lib, "Glu32.lib")
#pragma comment(lib, "OpenGL32.lib")

class QThumbnailWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core
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
	QSharedPointer<QOpenGLTexture> m_texture = nullptr;
	QSharedPointer<QOpenGLShaderProgram> m_program = nullptr;

	QSharedPointer<QOpenGLFramebufferObject> m_fbo = nullptr;
	QOpenGLVertexArrayObject m_vao;
	QOpenGLBuffer m_vbo;

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