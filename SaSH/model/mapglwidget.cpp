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

MapGLWidget::MapGLWidget(QWidget* parent)
	: QOpenGLWidget(parent)
{
	QOpenGLWidget::setAutoFillBackground(false);
}

MapGLWidget::~MapGLWidget()
{
	delete texture;
	delete vshader;
	delete fshader;
}

// 重寫QGLWidget類的接口
void MapGLWidget::initTextures()
{
	// 加載 Avengers.jpg 圖片
	texture = q_check_ptr(new QOpenGLTexture(QOpenGLTexture::Target2D));
	// 設置最近的過濾模式，以縮小紋理
	texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear); //濾波
	// 設置雙線性過濾模式，以放大紋理
	texture->setMagnificationFilter(QOpenGLTexture::Linear);
	// 重覆使用紋理坐標
	// 紋理坐標(1.1, 1.2)與(0.1, 0.2)相同
	texture->setWrapMode(QOpenGLTexture::Repeat);
	//設置紋理大小
	texture->setSize(this->width(), this->height());
	//分配儲存空間
	texture->allocateStorage();
}

void MapGLWidget::initShaders()
{
	//texture coordinates
	texCoords.append(QVector2D(0, 1)); // left-top
	texCoords.append(QVector2D(1, 1)); // right-top
	texCoords.append(QVector2D(0, 0)); // left-bottom
	texCoords.append(QVector2D(1, 0)); // right-bottom

	// vertices coordinates
	vertices.append(QVector3D(-1, -1, 1));// left-bottom
	vertices.append(QVector3D(1, -1, 1)); // right-bottom
	vertices.append(QVector3D(-1, 1, 1)); // left-top
	vertices.append(QVector3D(1, 1, 1));  // right-top
	vshader = q_check_ptr(new QOpenGLShader(QOpenGLShader::Vertex, this));
	const char* vsrc =
		"attribute vec4 vertex;\n"
		"attribute vec2 texCoord;\n"
		"varying vec2 texc;\n"
		"void main(void)\n"
		"{\n"
		"    gl_Position = vertex;\n"
		"    texc = texCoord;\n"
		"}\n";
	vshader->compileSourceCode(vsrc);// compile the vertex shader code
	fshader = q_check_ptr(new QOpenGLShader(QOpenGLShader::Fragment, this));
	const char* fsrc =
		"uniform sampler2D texture;\n"
		"varying vec2 texc;\n"
		"void main(void)\n"
		"{\n"
		"    gl_FragColor = texture2D(texture,texc);\n"
		"}\n";
	fshader->compileSourceCode(fsrc); //編譯紋理著色器代碼
	program.addShader(vshader);       //添加頂點著色器
	program.addShader(fshader);       //添加紋理碎片著色器
	program.bindAttributeLocation("vertex", 0);  //綁定頂點屬性位置
	program.bindAttributeLocation("texCoord", 1);//綁定紋理屬性位置
	// 鏈接著色器管道
	if (!program.link())
		close();
	// 綁定著色器管道
	if (!program.bind())
		close();
}

void MapGLWidget::initializeGL()
{
	initializeOpenGLFunctions(); //初始化OPenGL功能函數
	glClearColor(0, 0, 0, 0);    //設置背景為黑色
	glEnable(GL_TEXTURE_2D);     //設置紋理2D功能可用
	initTextures();              //初始化紋理設置
	initShaders();               //初始化shaders
}

void MapGLWidget::resizeGL(int w, int h)
{
	// 計算窗口橫縱比
	qreal aspect = qreal(w) / qreal(h ? h : 1);
	// 設置近平面值 3.0, 遠平面值 7.0, 視場45度
	const qreal zNear = 3.0, zFar = 7.0, fov = 45.0;
	// 重設投影
	projection.setToIdentity();
	// 設置透視投影
	projection.perspective(fov, static_cast<float>(aspect), zNear, zFar);
}

void MapGLWidget::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //清除屏幕緩存和深度緩沖

	//QMatrix4x4 matrix;
	//matrix.translate(0.0, 0.0, -5.0);                   //矩陣變換
	//program.enableAttributeArray(0);
	//program.enableAttributeArray(1);
	//program.setAttributeArray(0, vertices.constData());
	//program.setAttributeArray(1, texCoords.constData());
	//program.setUniformValue("texture", 0); //將當前上下文中位置的統一變量設置為value
	//texture->bind();  //綁定紋理

	glDisable(GL_DEPTH_TEST);
	QPainter paintImage;
	paintImage.begin(this);
	paintImage.drawPixmap(rectangle_dst_, m_image, rectangle_src_);
	paintImage.end();

	//畫刷。填充幾何圖形的調色板，由顏色和填充風格組成
	QPen m_penBlue(QColor(65, 105, 225));
	QBrush m_brushBlue(QColor(65, 105, 225), Qt::SolidPattern);
	QPainter lineChar;
	lineChar.begin(this);
	lineChar.setBrush(m_brushBlue);
	lineChar.setPen(m_penBlue);
	lineChar.drawLine(m_hStart, m_hEnd);//繪制橫向線
	lineChar.drawLine(m_vStart, m_vEnd);	//繪制縱向線
	lineChar.end();

	QPen m_penRedM(QColor(225, 128, 128));
	QBrush m_brushRedM(QColor(225, 128, 128), Qt::SolidPattern);
	QPainter lineMouse;
	lineMouse.begin(this);
	lineMouse.setBrush(m_brushRedM);
	lineMouse.setPen(m_penRedM);
	lineMouse.drawLine(m_hCurStart, m_hCurEnd);//繪制橫向線
	lineMouse.drawLine(m_vCurStart, m_vCurEnd);	//繪制縱向線
	lineMouse.end();

	QPainter rectChar;
	QPen m_penRed(Qt::red);
	QBrush m_brushRed(Qt::red, Qt::SolidPattern);
	rectChar.begin(this);
	rectChar.setBrush(m_brushRed);
	rectChar.setPen(m_penRed);
	rectChar.drawRect(m_rect);//繪制橫向線
	rectChar.end();
	glEnable(GL_DEPTH_TEST);
	//glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);//繪制紋理

	//texture->release(); //釋放綁定的紋理
	//texture->destroy(); //消耗底層的紋理對象
	//texture->create();
}

void MapGLWidget::setBackground(const QPixmap& image)
{
	m_image = image;
	if (texture != nullptr)
	{
		//texture->setSize(image.width(), image.height(), 1);
		//texture->setData(image);
		//texture->setLevelofDetailBias(-1);//值越小，圖像越清晰
	}
	update();
}

void MapGLWidget::setCurLineH(const QPointF& start, const QPointF& end)
{
	m_hCurStart = start;
	m_hCurEnd = end;
}

void MapGLWidget::setCurLineV(const QPointF& start, const QPointF& end)
{
	m_vCurStart = start;
	m_vCurEnd = end;
}

void MapGLWidget::setLineH(const QPointF& start, const QPointF& end)
{
	m_hStart = start;
	m_hEnd = end;
}

void MapGLWidget::setLineV(const QPointF& start, const QPointF& end)
{
	m_vStart = start;
	m_vEnd = end;
}

void MapGLWidget::setRect(const QRectF& rect)
{
	m_rect = rect;
}

void MapGLWidget::setPix(const QPixmap& image, const QRectF& src, const QRectF& dst)
{
	m_image = image;

	rectangle_src_ = src;//{ 0.0, 0.0, static_cast<qreal>(image.width()), static_cast<qreal>(image.height()) };
	rectangle_dst_ = dst;
}

void MapGLWidget::mouseMoveEvent(QMouseEvent* event)
{
	emit notifyMouseMove(event->button(), event->globalPos(), event->pos());
	if (bClicked_)
	{
		offest = event->pos() - pLast_;
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