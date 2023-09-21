#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class OpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT
public:
	explicit OpenGLWidget(QWidget* parent = nullptr)
		: QOpenGLWidget(parent)
	{}
	virtual ~OpenGLWidget() = default;

protected:
	virtual void initializeGL() override
	{
		initializeOpenGLFunctions();
		glClearColor(0, 0, 0, 0);    //設置背景為黑色
		glEnable(GL_TEXTURE_2D);     //設置紋理2D功能可用
		glDisable(GL_DEPTH_TEST);
		glShadeModel(GL_FLAT);
	}

	virtual void resizeGL(int w, int h) override
	{
		glViewport(0, 0, w, h); //設置視口
		glMatrixMode(GL_PROJECTION); //設置矩陣模式
		glLoadIdentity(); //重置矩陣
		glOrtho(0, w, h, 0, -1, 1); //設置正交投影
		glMatrixMode(GL_MODELVIEW); //設置矩陣模式
		glLoadIdentity(); //重置矩陣
	}

	virtual void paintGL() override
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
};