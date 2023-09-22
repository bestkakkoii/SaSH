#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class OpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT
public:
	explicit OpenGLWidget(QWidget* parent = nullptr)
		: QOpenGLWidget(parent)
	{
		font_ = util::getFont();
		pen_.setCosmetic(true);
		pen_.setColor(Qt::black);
	}
	virtual ~OpenGLWidget() = default;

	void setBackgroundColor(const QColor& color)
	{
		background_color_ = color;
	}

protected:
	virtual void initializeGL() override
	{
		initializeOpenGLFunctions();
		glClearColor(0, 0, 0, 0);    //設置背景為黑色
		glEnable(GL_TEXTURE_2D);     //設置紋理2D功能可用
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_SMOOTH);
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
		glClearColor(background_color_.redF(), background_color_.greenF(), background_color_.blueF(), 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		QPainter painter(this);
		painter.setPen(pen_);
		painter.setFont(font_);
		painter.setRenderHint(QPainter::Antialiasing);// 抗锯齿
		painter.setRenderHint(QPainter::TextAntialiasing); // 文本抗锯齿
		painter.setRenderHint(QPainter::SmoothPixmapTransform); // 平滑像素变换
		painter.setRenderHint(QPainter::LosslessImageRendering);

		painter.beginNativePainting();
		painter.fillRect(rect(), background_color_);
		painter.endNativePainting();

		glFinish();
	}

private:
	QPen pen_;
	QFont font_;
	QColor background_color_ = QColor("#F1F1F1");
};