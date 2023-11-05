#pragma once
#include <QtWidgets>
#include <QPixmap>

enum Enum_ZoomState {
	NO_STATE,
	RESET,
	ZOOM_IN,
	ZOOM_OUT
};

// class ImageWidget :public QObject, QGraphicsItem
class ImageWidget :public QGraphicsItem
{
	//Q_OBJECT
public:
	ImageWidget(QPixmap* pixmap)
	{
		m_pix = pixmap;
		setAcceptDrops(true);//If enabled is true, this item will accept hover events; otherwise, it will ignore them. By default, items do not accept hover events.
		m_scaleValue = 0;
		m_scaleDafault = 0;
		m_isMove = false;

		setCursor(Qt::ArrowCursor);
	}

	QRectF boundingRect() const
	{
		return QRectF(-m_pix->width() / 2, -m_pix->height() / 2,
			m_pix->width(), m_pix->height());
	}

	void setQGraphicsViewWH(long long nwidth, long long nheight)//将主界面的控件QGraphicsView的width和height传进本类中，并根据图像的长宽和控件的长宽的比例来使图片缩放到适合控件的大小
	{
		long long nImgWidth = m_pix->width();
		long long nImgHeight = m_pix->height();
		qreal temp1 = nwidth * 1.0 / nImgWidth;
		qreal temp2 = nheight * 1.0 / nImgHeight;
		if (temp1 > temp2)
		{
			m_scaleDafault = temp2;
		}
		else
		{
			m_scaleDafault = temp1;
		}
		setScale(m_scaleDafault);
		m_scaleValue = m_scaleDafault;
	}

	void ResetItemPos()//重置图片位置
	{
		m_scaleValue = m_scaleDafault;//缩放比例回到一开始的自适应比例
		setScale(m_scaleDafault);//缩放到一开始的自适应大小
		setPos(0, 0);
	}

	qreal getScaleValue() const
	{
		return m_scaleValue;
	}

	void setCurLineH(const QPointF& start, const QPointF& end)
	{
		if (hCurStart_ != start)
			hCurStart_ = start;

		if (hCurEnd_ != end)
			hCurEnd_ = end;
	}

	void setCurLineV(const QPointF& start, const QPointF& end)
	{
		if (vCurStart_ != start)
			vCurStart_ = start;

		if (vCurEnd_ != end)
			vCurEnd_ = end;
	}

	void setLineH(const QPointF& start, const QPointF& end)
	{
		if (hStart_ != start)
			hStart_ = start;

		if (hEnd_ != end)
			hEnd_ = end;
	}

	void setLineV(const QPointF& start, const QPointF& end)
	{
		if (vStart_ != start)
			vStart_ = start;

		if (vEnd_ != end)
			vEnd_ = end;
	}

	void setRect(const QRectF& rect)
	{
		rect_ = rect;
	}

	qreal getZoomValue() const
	{
		return m_scaleValue;
	}

	QPointF characterPos;

protected:
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override
	{
		painter->drawPixmap(-m_pix->width() / 2, -m_pix->height() / 2, *m_pix);

		QPaintDevice* device = painter->device();
		QPainter paintImage;
		paintImage.begin(device);

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

	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event) override
	{
		if (event->button() == Qt::LeftButton)
		{
			m_startPos = event->pos();//鼠标左击时，获取当前鼠标在图片中的坐标，
			m_isMove = true;//标记鼠标左键被按下
		}
		else if (event->button() == Qt::RightButton)
		{
			ResetItemPos();//右击鼠标重置大小
		}
	}

	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override
	{
		QPointF pos = event->pos();

		if (m_isMove)
		{
			QPointF point = (event->pos() - m_startPos) * m_scaleValue;
			moveBy(point.x(), point.y());
		}
	}

	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent*)
	{
		m_isMove = false;//标记鼠标左键已经抬起
	}

	virtual void wheelEvent(QGraphicsSceneWheelEvent* event) override
	{
		if ((event->delta() > 0) && (m_scaleValue >= 50))//最大放大到原始图像的50倍
		{
			return;
		}
		else if ((event->delta() < 0) && (m_scaleValue <= m_scaleDafault))//图像缩小到自适应大小之后就不继续缩小
		{
			ResetItemPos();//重置图片大小和位置，使之自适应控件窗口大小
		}
		else
		{
			qreal qrealOriginScale = m_scaleValue;
			if (event->delta() > 0)//鼠标滚轮向前滚动
			{
				m_scaleValue *= 1.1;//每次放大10%
			}
			else
			{
				m_scaleValue *= 0.9;//每次缩小10%
			}
			setScale(m_scaleValue);
			if (event->delta() > 0)
			{
				moveBy(-event->pos().x() * qrealOriginScale * 0.1, -event->pos().y() * qrealOriginScale * 0.1);//使图片缩放的效果看起来像是以鼠标所在点为中心进行缩放的
			}
			else
			{
				moveBy(event->pos().x() * qrealOriginScale * 0.1, event->pos().y() * qrealOriginScale * 0.1);//使图片缩放的效果看起来像是以鼠标所在点为中心进行缩放的
			}
		}
	}

private:
	qreal       m_scaleValue = 0.0;

	qreal m_scaleDafault = 0.0;

	QPixmap* m_pix;
	long long   m_zoomState = NO_STATE;
	bool        m_isMove = false;
	QPointF     m_startPos;

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

class GraphicView : public QGraphicsView
{
	Q_OBJECT

public:
	GraphicView(QWidget* parent = nullptr)
		: QGraphicsView(parent)
	{
		// 设置一些视图属性
		setRenderHint(QPainter::Antialiasing);
		setDragMode(QGraphicsView::ScrollHandDrag);
		setInteractive(true);

		setMouseTracking(true);

		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

		imageItem_ = q_check_ptr(new ImageWidget(&map_));
		sash_assume(imageItem_ != nullptr);
		scene_.addItem(imageItem_);
		setScene(&scene_);//Sets the current scene to scene. If scene is already being viewed, this function does nothing.

		setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
		setCacheMode(QGraphicsView::CacheNone);

		setSceneRect(QRectF(-(width() / 2), -(height() / 2), width(), height()));
		imageItem_->setQGraphicsViewWH(width(), height());
		imageItem_->update();
	}

	ImageWidget* getImageItem() const
	{
		return imageItem_;
	}

public slots:
	void onUpdateMap(const QPixmap& map, const QPoint& characterPos)
	{
		bool b = this->map_.isNull();
		this->map_ = map;

		qreal zoomValue = imageItem_->getScaleValue();
		const QPointF dst(mousePos_ / zoomValue);

		imageItem_->setLineH({ 0.0, (characterPos.y() * zoomValue) }, { static_cast<qreal>(width()), characterPos.y() * zoomValue });
		imageItem_->setLineV({ characterPos.x() * zoomValue, 0.0 }, { (characterPos.x() * zoomValue), static_cast<qreal>(height()) });
		imageItem_->setRect({ (characterPos.x() * zoomValue) + 0.5, ((characterPos.y() - 1.0) * zoomValue) + zoomValue + 0.5, zoomValue, zoomValue });
		imageItem_->setCurLineH({ 0.0, (dst.y() * zoomValue) }, { static_cast<qreal>(width()), dst.y() * zoomValue });
		imageItem_->setCurLineV({ dst.x() * zoomValue , 0.0 }, { (dst.x() * zoomValue), static_cast<qreal>(height()) });


		//使视窗的大小固定在原始大小，不会随图片的放大而放大（默认状态下图片放大的时候视窗两边会自动出现滚动条，
		//并且视窗内的视野会变大），防止图片放大后重新缩小的时候视窗太大而不方便观察图片
		setSceneRect(QRectF(-(width() / 2), -(height() / 2), width(), height()));
		imageItem_->update();

		if (b)
			imageItem_->setQGraphicsViewWH(width(), height());
	}

protected:
	void resizeEvent(QResizeEvent* event) override
	{
		imageItem_->setQGraphicsViewWH(width(), height());//将界面控件Graphics View的width和height传进类m_Image中
		QGraphicsView::resizeEvent(event);
	}

	virtual void mouseMoveEvent(QMouseEvent* event) override
	{
		mousePos_ = event->pos();
		QGraphicsView::mouseMoveEvent(event);
	}

private:
	QGraphicsScene scene_;
	QPixmap map_;

	QPoint mousePos_;
	QPoint characterPos_;
	QPen crosshairPen_;

	ImageWidget* imageItem_ = nullptr;
};