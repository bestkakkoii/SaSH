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
#include "mapwidget.h"
#include "util.h"
#include "injector.h"
#include "net/tcpserver.h"
#include "map/mapanalyzer.h"
#include "model/customtitlebar.h"

constexpr long long MAP_REFRESH_TIME = 144;
constexpr long long MAX_BLOCK_SIZE = 24;
constexpr long long MAX_DOWNLOAD_DELAY = 0;

QHash<long long, QHash<QPoint, QString>> MapWidget::entrances_;

MapWidget::MapWidget(long long index, QWidget* parent)
	: QMainWindow(parent)
	, Indexer(index)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_StyledBackground);
	setWindowTitle(QString("[%1]").arg(index));

	takeCentralWidget();
	setDockNestingEnabled(true);
	addDockWidget(Qt::LeftDockWidgetArea, ui.dockWidget_gl);
	addDockWidget(Qt::RightDockWidgetArea, ui.dockWidget_list);
	addDockWidget(Qt::BottomDockWidgetArea, ui.dockWidget_view);

	splitDockWidget(ui.dockWidget_gl, ui.dockWidget_view, Qt::Vertical);
	splitDockWidget(ui.dockWidget_gl, ui.dockWidget_list, Qt::Horizontal);

	ui.dockWidget_gl->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	ui.dockWidget_view->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	ui.dockWidget_list->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

	//set header text
	ui.tableWidget_NPCList->setColumnCount(2);
	ui.tableWidget_NPCList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

	//vec invisible
	ui.tableWidget_NPCList->verticalHeader()->setVisible(false);
	ui.tableWidget_NPCList->verticalHeader()->setDefaultSectionSize(11);
	ui.tableWidget_NPCList->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	//ui.tableWidget_NPCList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);//隐藏滚动条

	connect(this, &MapWidget::updateMap, ui.graphicsView, &GraphicView::onUpdateMap);


	connect(&gltimer_, &QTimer::timeout, this, &MapWidget::onRefreshTimeOut);
	gltimer_.start(MAP_REFRESH_TIME);

	connect(&downloadMapTimer_, &QTimer::timeout, this, &MapWidget::onDownloadMapTimeout);

	if (!entrances_.isEmpty())
		return;

	QString content;
	QStringList paths;
	util::searchFiles(util::applicationDirPath(), "point", ".txt", &paths, false);
	if (paths.isEmpty())
		return;

	if (!util::readFile(paths.front(), &content))
	{
		qDebug() << "Failed to open point.dat";
		return;
	}

	QStringList entrances = content.simplified().split(" ");
	QHash<long long, QHash<QPoint, QString>> preEntrances;

	for (const QString& entrance : entrances)
	{
		const QStringList entranceData(entrance.split(util::rexOR));
		if (entranceData.size() != 3)
			continue;

		bool ok = false;
		const long long floor = entranceData.value(0).toLongLong(&ok);
		if (!ok)
			continue;

		const QString pointStr(entranceData.value(1));
		const QStringList pointData(pointStr.split(util::rexComma));
		if (pointData.size() != 2)
			continue;

		long long x = pointData.value(0).toLongLong(&ok);
		if (!ok)
			continue;

		long long y = pointData.value(1).toLongLong(&ok);
		if (!ok)
			continue;

		const QPoint pos(x, y);

		const QString name(entranceData.value(2));


		if (!preEntrances.contains(floor))
			preEntrances.insert(floor, QHash<QPoint, QString>());

		preEntrances[floor].insert(pos, name);
	}

	entrances_ = preEntrances;
}

MapWidget::~MapWidget()
{
#if !OPEN_GL_ON
	timer_.stop();
#endif
	qDebug() << "";


	onClear();
}

void MapWidget::showEvent(QShowEvent*)
{
	setUpdatesEnabled(true);
	blockSignals(false);

	Injector& injector = Injector::getInstance(getIndex());
	if (injector.IS_FINDINGPATH.load(std::memory_order_acquire))
	{
		ui.pushButton_findPath->setEnabled(false);
	}


	gltimer_.start(MAP_REFRESH_TIME);

	util::FormSettingManager formManager(this);
	formManager.loadSettings();
	setAttribute(Qt::WA_Mapped);
	QWidget::showEvent(nullptr);
}

void MapWidget::onFindPathFinished()
{
	ui.pushButton_findPath->setEnabled(true);
}

void MapWidget::closeEvent(QCloseEvent*)
{
	setUpdatesEnabled(false);
	blockSignals(true);
	update();
	gltimer_.stop();
	downloadMapTimer_.stop();
	isDownloadingMap_ = false;

	util::FormSettingManager formManager(this);
	formManager.saveSettings();

	qDebug() << "";
}

static inline void zoom(QWidget* p, const QPixmap& pix, qreal* scaleWidth, qreal* scaleHeight, qreal* zoom_value, qreal fix)
{
	const qreal imageWidth = static_cast<qreal>(pix.width());
	const qreal imageHeight = static_cast<qreal>(pix.height());
	qreal tmp_zoom = 0.0;

	if (((p->width()) / (imageWidth)) <= ((p->height()) / (imageHeight)))
	{
		tmp_zoom = ((p->width()) / (imageWidth));
		*scaleWidth = ((imageWidth) * (tmp_zoom + fix));
		*scaleHeight = ((imageHeight) * (tmp_zoom + fix));
	}
	else
	{
		tmp_zoom = ((p->height() - 50) / (imageHeight));
		*scaleWidth = ((imageWidth) * (tmp_zoom + fix));
		*scaleHeight = ((imageHeight) * (tmp_zoom + fix));
	}
	if (0.0 == (tmp_zoom))
		tmp_zoom = 1.0;

	*zoom_value = tmp_zoom + fix;
}

void MapWidget::onRefreshTimeOut()
{
	if (!isVisible())
		return;

	long long currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.worker.isNull()) return;


	if (!injector.worker->getOnlineFlag()) return;

	PC _ch = injector.worker->getPC();
	long long floor = injector.worker->getFloor();
	const QPointF qp_current(injector.worker->getPoint());

	QString caption(tr("[%1] %2 map:%3 floor:%4 [%5,%6] mouse:%7,%8")
		.arg(currentIndex)
		.arg(_ch.name)
		.arg(injector.worker->getFloorName())
		.arg(floor)
		.arg(qp_current.x()).arg(qp_current.y())
		.arg(qFloor(curMousePos_.x())).arg(qFloor(curMousePos_.y())));

	if (isDownloadingMap_)
		caption += " " + tr("downloading(%1%2)").arg(util::toQString(downloadMapProgress_)).arg("%");
	setWindowTitle(caption);

	bool map_ret = injector.worker->mapAnalyzer.readFromBinary(currentIndex, floor, injector.worker->getFloorName(), true);
	if (!map_ret)
		return;

	QPixmap ppix = injector.worker->mapAnalyzer.getPixmapByIndex(floor);
	if (ppix.isNull())  return;

	map_t m_map = {};
	injector.worker->mapAnalyzer.getMapDataByFloor(floor, &m_map);

	QHash<long long, mapunit_t> unitHash = injector.worker->mapUnitHash.toHash();
	auto findMapUnitByPoint = [&unitHash](const QPoint& p, mapunit_t* u)->bool
		{
			for (auto it = unitHash.begin(); it != unitHash.end(); ++it)
			{
				if (it->p == p)
				{
					if (u)
						*u = it.value();
					return true;
				}
			}
			return false;
		};


#if 1
	QStringList vGM;
	QStringList vNPC;
	QStringList vITEM;
	QStringList vGOLD;
	QStringList vPET;
	QStringList vHUMAN;
	QStringList vSTAIR;
	QSet<QPoint> stair_cache;

	for (qmappoint_t it : m_map.stair)
	{
		if (stair_cache.contains(it.p)) continue;
		QString typeStr = "\0";

		if (entrances_.contains(m_map.floor) && entrances_.value(m_map.floor).contains(it.p))
		{
			typeStr = entrances_.value(m_map.floor).value(it.p);
			it.p = entrances_.value(m_map.floor).key(typeStr, it.p);
		}

		if (typeStr.isEmpty())
		{
			for (const auto& p : util::fix_point)
			{
				if (entrances_.contains(m_map.floor) && entrances_.value(m_map.floor).contains(it.p + p))
				{
					typeStr = entrances_.value(m_map.floor).value(it.p + p);
					it.p = entrances_.value(m_map.floor).key(typeStr, it.p + p);
				}
			}
		}

		if (typeStr.isEmpty())
		{
			switch (it.type)
			{
			case util::OBJ_UP:
				typeStr = tr("UP");
				break;
			case util::OBJ_DOWN:
				typeStr = tr("DWON");
				break;
			case util::OBJ_JUMP:
				typeStr = tr("JUMP");
				break;
			case util::OBJ_WARP:
				typeStr = tr("WARP");
				break;
			default:
				typeStr = tr("UNKNOWN");
				continue;
			}
		}

		stair_cache.insert(it.p);
		mapunit_t u = {};
		if (findMapUnitByPoint(it.p, &u))
		{
			if (!u.name.isEmpty() && (u.name != _ch.name))
			{
				if (u.isVisible)
				{
					vSTAIR.append(QString("[%1]%2").arg(typeStr).arg(u.name));
					vSTAIR.append(QString("%1,%2").arg(it.p.x()).arg(it.p.y()));
				}
				else
				{
					vSTAIR.append(QString(tr("X[%1]%2")).arg(typeStr).arg(u.name));
					vSTAIR.append(QString("%1,%2").arg(it.p.x()).arg(it.p.y()));
				}
				continue;
			}
		}
		vSTAIR.append(QString("[%1]").arg(typeStr));
		vSTAIR.append(QString("%1,%2").arg(it.p.x()).arg(it.p.y()));
	}

	QList<mapunit_t> units = unitHash.values();
	for (mapunit_t it : units)
	{
		if (it.objType == util::OBJ_GM)
		{
			vGM.append(QString("[GM]%1").arg(it.name));
			vGM.append(QString("%1,%2").arg(it.p.x()).arg(it.p.y()));
		}
		else
		{
			if (it.name == _ch.name)
				continue;

			switch (it.objType)
			{
			case util::OBJ_ITEM:
			{
				vITEM.append(QString(tr("[I]%1")).arg(it.item_name));
				vITEM.append(QString("%1,%2").arg(it.p.x()).arg(it.p.y()));
				break;
			}
			case util::OBJ_GOLD:
			{
				vGOLD.append(QString(tr("[G]%1")).arg(it.gold));
				vGOLD.append(QString("%1,%2").arg(it.p.x()).arg(it.p.y()));
				break;
			}
			case util::OBJ_PET:
			{
				if (it.isVisible)
					vPET.append(it.freeName.isEmpty() ? QString(tr("[P]%2")).arg(it.name) : QString(tr("[P]%2")).arg(it.freeName));
				else
					vPET.append(it.freeName.isEmpty() ? QString(tr("X[P]%2")).arg(it.name) : QString(tr("[P]%2")).arg(it.freeName));
				vPET.append(QString("%1,%2").arg(it.p.x()).arg(it.p.y()));
				break;
			}
			case util::OBJ_HUMAN:
			{
				if (it.isVisible)
					vHUMAN.append(QString(tr("[H]%1")).arg(it.name));
				else
					vHUMAN.append(QString(tr("X[H]%1")).arg(it.name));
				vHUMAN.append(QString("%1,%2").arg(it.p.x()).arg(it.p.y()));
				break;
			}
			case util::OBJ_NPC:
			{
				if (it.isVisible)
					vNPC.append(QString("[NPC][%1]%2").arg(it.modelid).arg(it.name));
				else
					vNPC.append(QString(tr("X[NPC][%1]%2")).arg(it.modelid).arg(it.name));
				vNPC.append(QString("%1,%2").arg(it.p.x()).arg(it.p.y()));
				break;
			}
			case util::OBJ_JUMP:
			case util::OBJ_DOWN:
			case util::OBJ_UP:
			{
				if (stair_cache.contains(it.p)) continue;

				QString typeStr = "\0";

				if (entrances_.contains(m_map.floor) && entrances_.value(m_map.floor).contains(it.p))
				{
					typeStr = entrances_.value(m_map.floor).value(it.p);
					it.p = entrances_.value(m_map.floor).key(typeStr, it.p);
				}

				if (typeStr.isEmpty())
				{
					for (const auto& p : util::fix_point)
					{
						if (entrances_.contains(m_map.floor) && entrances_.value(m_map.floor).contains(it.p + p))
						{
							typeStr = entrances_.value(m_map.floor).value(it.p + p);
							it.p = entrances_.value(m_map.floor).key(typeStr, it.p + p);
						}
					}
				}

				if (typeStr.isEmpty())
				{
					switch (it.objType)
					{
					case util::OBJ_UP:
						typeStr = tr("UP");
						break;
					case util::OBJ_DOWN:
						typeStr = tr("DWON");
						break;
					case util::OBJ_JUMP:
						typeStr = tr("JUMP");
						break;
					case util::OBJ_WARP:
						typeStr = tr("WARP");
						break;
					default:
						typeStr = tr("UNKNOWN");
						break;
					}
				}

				if (!it.name.isEmpty())
				{
					if (it.isVisible)
					{
						vSTAIR.append(QString("[%1]%2").arg(typeStr).arg(it.name));
						vSTAIR.append(QString("%1,%2").arg(it.p.x()).arg(it.p.y()));
					}
					else
					{
						vSTAIR.append(QString("X[%1]%2").arg(typeStr).arg(it.name));
						vSTAIR.append(QString("%1,%2").arg(it.p.x()).arg(it.p.y()));
					}
				}
				else
				{
					vSTAIR.append(QString("[%1]").arg(typeStr));
					vSTAIR.append(QString("%1,%2").arg(it.p.x()).arg(it.p.y()));
				}
				break;
			}
			default: continue;
			}
		}

		QBrush brush;
		if (it.name.contains("傳送石"))
		{
			brush = QBrush(MAP_COLOR_HASH.value(util::OBJ_JUMP), Qt::SolidPattern);
		}
		else
			brush = QBrush(MAP_COLOR_HASH.value(it.objType), Qt::SolidPattern);
		QPen pen(brush, 1.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

		QPainter painter(&ppix);
		painter.setPen(pen);
		painter.drawPoint(it.p);
		painter.end();
	}


	QVariantList vlist;
	QVariant dataVar;
	vlist.append(vGM);
	vlist.append(vNPC);
	vlist.append(vITEM);
	vlist.append(vGOLD);
	vlist.append(vPET);
	vlist.append(vHUMAN);
	vlist.append(vSTAIR);
	dataVar.setValue(vlist);
	updateNpcListAllContents(dataVar);
#endif

	emit updateMap(ppix, qp_current.toPoint());

	zoom(ui.dockWidget_gl, ppix, &scaleWidth_, &scaleHeight_, &zoom_value_, fix_zoom_value_);

	if (ui.openGLWidget->width() != static_cast<long long>(scaleWidth_) || ui.openGLWidget->height() != static_cast<long long>(scaleHeight_))
		ui.openGLWidget->resize(scaleWidth_, scaleHeight_);


	rectangle_src_ = QRectF{ 0.0, 0.0, static_cast<qreal>(ppix.width()), static_cast<qreal>(ppix.height()) };
	rectangle_dst_ = QRectF(0.0, 0.0, scaleWidth_, scaleHeight_);
	ui.openGLWidget->setPix(ppix, rectangle_src_, rectangle_dst_);

	//人物座標十字
	ui.openGLWidget->setLineH({ 0.0, (qp_current.y() * zoom_value_) }, { static_cast<qreal>(ui.openGLWidget->width()), qp_current.y() * zoom_value_ });
	ui.openGLWidget->setLineV({ qp_current.x() * zoom_value_, 0.0 }, { (qp_current.x() * zoom_value_), static_cast<qreal>(ui.openGLWidget->height()) });
	//人物座標方格
	ui.openGLWidget->setRect({ (qp_current.x() * zoom_value_) + 0.5, ((qp_current.y() - 1.0) * zoom_value_) + zoom_value_ + 0.5, zoom_value_, zoom_value_ });

	ui.openGLWidget->repaint(); //窗口重绘，repaint会调用paintEvent函数，paintEvent会调用paintGL函数实现重绘
}

void MapWidget::on_openGLWidget_notifyMouseMove(Qt::MouseButton, const QPointF& gpos, const QPointF& pos)
{
	if (bClicked_)
	{
		movePoint_ = gpos;
		ui.openGLWidget->repaint();
	}
	const QPointF dst(pos / zoom_value_);
	curMousePos_ = dst;

	//滑鼠十字
	ui.openGLWidget->setCurLineH({ 0.0, (curMousePos_.y() * zoom_value_) }, { static_cast<qreal>(ui.openGLWidget->width()), curMousePos_.y() * zoom_value_ });
	ui.openGLWidget->setCurLineV({ curMousePos_.x() * zoom_value_, 0.0 }, { (curMousePos_.x() * zoom_value_), static_cast<qreal>(ui.openGLWidget->height()) });

	ui.openGLWidget->repaint();
}

void MapWidget::leaveEvent(QEvent*)
{
	curMousePos_ = QPointF(-1, -1);
}

void MapWidget::on_openGLWidget_notifyRightClick()
{
	onClear();
}

void MapWidget::on_openGLWidget_notifyLeftDoubleClick(const QPointF& pos)
{
	long long currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.IS_SCRIPT_FLAG.load(std::memory_order_acquire))
		return;

	if (injector.worker.isNull())
		return;

	if (!injector.worker->getOnlineFlag())
		return;

	if (injector.IS_FINDINGPATH.load(std::memory_order_acquire))
	{
		injector.IS_FINDINGPATH.store(false, std::memory_order_acquire);
	}

	if (findPathFuture_.isRunning())
	{
		findPathFuture_.cancel();
		findPathFuture_.waitForFinished();
	}

	connect(injector.worker.get(), &Worker::findPathFinished, this, &MapWidget::onFindPathFinished, Qt::UniqueConnection);

	const QPointF predst(pos / zoom_value_);
	const QPoint dst(predst.toPoint());

	long long x = dst.x();
	long long y = dst.y();


	QPoint point(x, y);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	findPathFuture_ = QtConcurrent::run(injector.worker.get(), &Worker::findPathAsync, point);
#else
	findPathFuture_ = QtConcurrent::run(&Worker::findPathAsync, injector.worker.get(), point);
#endif

	ui.pushButton_findPath->setEnabled(false);
}

void MapWidget::on_openGLWidget_notifyLeftClick(const QPointF& gpos, const QPointF& pos)
{
	bClicked_ = true;
	pLast_ = gpos - pos;
}

void MapWidget::on_openGLWidget_notifyLeftRelease()
{
	bClicked_ = false;
}

void MapWidget::on_openGLWidget_notifyWheelMove(const QPointF& zoom, const QPointF&)
{
	fix_zoom_value_ += (zoom.y() > 0) ? 0.1 : -0.1;
	onRefreshTimeOut();
}

void MapWidget::downloadNextBlock()
{
	long long blockWidth = qMin(MAX_BLOCK_SIZE, downloadMapXSize_ - downloadMapX_);
	long long blockHeight = qMin(MAX_BLOCK_SIZE, downloadMapYSize_ - downloadMapY_);

	// 移除一個小區塊
	downloadMapX_ += blockWidth;
	if (downloadMapX_ >= downloadMapXSize_)
	{
		downloadMapX_ = 0;
		downloadMapY_ += blockHeight;
	}

	// 更新下載進度
	downloadCount_++;
	downloadMapProgress_ = static_cast<qreal>(downloadCount_) / totalMapBlocks_ * 100.0;
}

void MapWidget::onDownloadMapTimeout()
{
	if (!isDownloadingMap_)
	{
		if (!ui.pushButton_download->isEnabled())
			ui.pushButton_download->setEnabled(true);
		return;
	}
	long long currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.worker.isNull())
	{
		isDownloadingMap_ = false;
		return;
	}

	injector.worker->downloadMap(downloadMapX_, downloadMapY_);
	long long floor = injector.worker->getFloor();
	QString name = injector.worker->getFloorName();

	downloadNextBlock();

	if (downloadMapProgress_ >= 100.0)
	{
		downloadMapTimer_.stop();
		const QPoint qp_current = injector.worker->getPoint();
		QString caption(tr("[%1] %2 map:%3 floor:%4 [%5,%6] mouse:%7,%8")
			.arg(currentIndex)
			.arg(injector.worker->getPC().name)
			.arg(injector.worker->getFloorName())
			.arg(injector.worker->getFloor())
			.arg(qp_current.x()).arg(qp_current.y())
			.arg(qFloor(curMousePos_.x())).arg(qFloor(curMousePos_.y())));
		caption += " " + tr("downloading(%1%2)").arg(util::toQString(100.00)).arg("%");
		setWindowTitle(caption);

		injector.worker->mapAnalyzer.clear(floor);
		const QString fileName(injector.worker->mapAnalyzer.getCurrentPreHandleMapPath(currentIndex, floor));
		QFile file(fileName);
		if (file.exists())
		{
			file.remove();
		}
		isDownloadingMap_ = false;
		injector.worker->EO();
		injector.worker->mapAnalyzer.readFromBinary(currentIndex, floor, name, true, true);
		if (!ui.pushButton_download->isEnabled())
			ui.pushButton_download->setEnabled(true);
	}
}

void MapWidget::on_pushButton_download_clicked()
{
	if (isDownloadingMap_)
		return;
	long long currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.worker.isNull())
	{
		return;
	}

	ui.pushButton_download->setEnabled(false);

	map_t map = {};
	injector.worker->mapAnalyzer.getMapDataByFloor(injector.worker->getFloor(), &map);

	downloadCount_ = 0;
	downloadMapXSize_ = map.width;
	downloadMapYSize_ = map.height;
	downloadMapX_ = 0;
	downloadMapY_ = 0;
	downloadMapProgress_ = 0.0;

	const long long numBlocksX = (downloadMapXSize_ + MAX_BLOCK_SIZE - 1) / MAX_BLOCK_SIZE;
	const long long numBlocksY = (downloadMapYSize_ + MAX_BLOCK_SIZE - 1) / MAX_BLOCK_SIZE;
	const long long totalBlocks = numBlocksX * numBlocksY;
	totalMapBlocks_ = static_cast<qreal>(totalBlocks);

	isDownloadingMap_ = true;
	downloadMapTimer_.start(MAX_DOWNLOAD_DELAY);
}

void MapWidget::on_pushButton_clear_clicked()
{
	long long currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.worker.isNull())
		return;

	long long floor = injector.worker->getFloor();
	injector.worker->mapAnalyzer.clear(floor);

	const QString fileName(injector.worker->mapAnalyzer.getCurrentPreHandleMapPath(currentIndex, floor));
	QFile file(fileName);
	if (!file.exists())
		return;

	file.remove();

	injector.worker->mapAnalyzer.readFromBinary(currentIndex, floor, injector.worker->getFloorName(), true, true);
}

void MapWidget::on_pushButton_returnBase_clicked()
{
	long long currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.worker.isNull())
	{
		return;
	}

	if (injector.isValid())
		injector.setEnableHash(util::kLogBackEnable, true);
}

void MapWidget::on_pushButton_findPath_clicked()
{
	long long currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (!injector.isValid())
		return;

	if (injector.IS_SCRIPT_FLAG.load(std::memory_order_acquire))
		return;

	if (injector.IS_FINDINGPATH.load(std::memory_order_acquire))
		return;

	if (injector.worker.isNull())
		return;

	if (!injector.worker->getOnlineFlag())
		return;

	if (findPathFuture_.isRunning())
	{
		injector.IS_FINDINGPATH.store(false, std::memory_order_release);
		findPathFuture_.cancel();
		findPathFuture_.waitForFinished();
	}

	connect(injector.worker.get(), &Worker::findPathFinished, this, &MapWidget::onFindPathFinished, Qt::UniqueConnection);

	long long x = ui.spinBox_findPathX->value();
	long long y = ui.spinBox_findPathY->value();
	if (x < 0 || x > 1500 || y < 0 || y > 1500)
		return;

	QPoint point(x, y);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	findPathFuture_ = QtConcurrent::run(injector.worker.get(), &Worker::findPathAsync, point);
#else
	findPathFuture_ = QtConcurrent::run(&Worker::findPathAsync, injector.worker.get(), point);
#endif

	ui.pushButton_findPath->setEnabled(false);
}

void MapWidget::onClear()
{
	long long currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	injector.IS_FINDINGPATH.store(false, std::memory_order_release);
}

void MapWidget::updateNpcListAllContents(const QVariant& d)
{
	long long col = 0;
	long long row = 0;
	long long size = 0;
	QVariantList D = d.value<QVariantList>();

	ui.tableWidget_NPCList->setUpdatesEnabled(false);
	for (const QVariant& it : D)
	{
		size += it.value<QStringList>().size();
	}
	size /= 2;
	if (size > ui.tableWidget_NPCList->rowCount())
	{
		for (long long i = 0; i < size - ui.tableWidget_NPCList->rowCount(); ++i)
			ui.tableWidget_NPCList->insertRow(ui.tableWidget_NPCList->rowCount());
	}
	else if (size < ui.tableWidget_NPCList->rowCount())
	{
		for (long long i = 0; i < (ui.tableWidget_NPCList->rowCount() - size); ++i)
			ui.tableWidget_NPCList->removeRow(ui.tableWidget_NPCList->rowCount() - 1);
	}

	for (const QVariant& it : D)
	{
		QStringList list = it.value<QStringList>();

		for (const QString& i : list)
		{
			ui.tableWidget_NPCList->setText(row, col, i);
			++col;
			if (col >= 2)
			{
				++row;
				col = 0;
			}
		}
	}

	ui.tableWidget_NPCList->setUpdatesEnabled(true);
}

void MapWidget::on_tableWidget_NPCList_cellDoubleClicked(int row, int)
{
	QTableWidgetItem* item = ui.tableWidget_NPCList->item(row, 1);
	QTableWidgetItem* item_name = ui.tableWidget_NPCList->item(row, 0);

	if (nullptr == item || nullptr == item_name)
	{
		return;
	}

	long long currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (!injector.isValid())
		return;

	if (injector.IS_SCRIPT_FLAG.load(std::memory_order_acquire))
		return;

	if (injector.IS_FINDINGPATH.load(std::memory_order_acquire))
		return;

	if (injector.worker.isNull())
		return;

	if (!injector.worker->getOnlineFlag())
		return;

	connect(injector.worker.get(), &Worker::findPathFinished, this, &MapWidget::onFindPathFinished, Qt::UniqueConnection);

	QString name(item_name->text());
	QString text(item->text());
	QStringList list(text.split(util::rexComma));
	if (list.size() != 2)
		return;

	bool okx, oky;
	long long x = list.value(0).toLongLong(&okx);
	long long y = list.value(1).toLongLong(&oky);
	if (!okx || !oky)
		return;

	mapunit_t unit;
	if (name.contains("NPC"))
	{
		static const QRegularExpression rex(R"(\[NPC\]\[\d+\])");
		name = name.remove(rex);
		if (!injector.worker->findUnit(name, util::OBJ_NPC, &unit))
			return;
	}
	else if (name.contains(tr("[P]")))
	{
		name = name.remove(tr("[P]"));
		if (!injector.worker->findUnit(name, util::OBJ_PET, &unit))
			return;
	}
	else if (name.contains(tr("[H]")))
	{
		name = name.remove(tr("[H]"));
		if (!injector.worker->findUnit(name, util::OBJ_HUMAN, &unit))
			return;
	}
	else if (name.contains(tr("[I]")))
	{
		name = name.remove(tr("[I]"));
		if (!injector.worker->findUnit(name, util::OBJ_ITEM, &unit))
			return;
	}
	else if (name.contains(tr("[G]")))
	{
		name = name.remove(tr("[G]"));
		if (!injector.worker->findUnit(name, util::OBJ_GOLD, &unit))
			return;
	}
	else
	{
		if (findPathFuture_.isRunning())
		{
			injector.IS_FINDINGPATH.store(false, std::memory_order_release);
			findPathFuture_.cancel();
			findPathFuture_.waitForFinished();
		}

		QPoint point(x, y);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
		findPathFuture_ = QtConcurrent::run(injector.worker.get(), &Worker::findPathAsync, point);
#else
		findPathFuture_ = QtConcurrent::run(&Worker::findPathAsync, injector.worker.get(), point);
#endif

		ui.pushButton_findPath->setEnabled(false);
		return;
	}

	CAStar astar;
	long long floor = injector.worker->getFloor();
	QPoint point = injector.worker->getPoint();
	//npc前方一格
	QPoint newPoint = util::fix_point.value(unit.dir) + unit.p;

	//檢查是否可走
	if (injector.worker->mapAnalyzer.isPassable(currentIndex, astar, floor, point, newPoint))
	{
		x = newPoint.x();
		y = newPoint.y();
	}
	else
	{
		//再往前一格
		QPoint additionPoint = util::fix_point.value(unit.dir) + newPoint;
		//檢查是否可走
		if (injector.worker->mapAnalyzer.isPassable(currentIndex, astar, floor, point, additionPoint))
		{
			x = additionPoint.x();
			y = additionPoint.y();
		}
		else
		{
			//檢查NPC周圍8格
			bool flag = false;
			for (long long i = 0; i < 8; ++i)
			{
				newPoint = util::fix_point.value(i) + unit.p;
				if (injector.worker->mapAnalyzer.isPassable(currentIndex, astar, floor, point, newPoint))
				{
					x = newPoint.x();
					y = newPoint.y();
					flag = true;
					break;
				}
			}
			if (!flag)
			{
				return;
			}
		}
	}

	if (findPathFuture_.isRunning())
	{
		injector.IS_FINDINGPATH.store(false, std::memory_order_release);
		findPathFuture_.cancel();
		findPathFuture_.waitForFinished();
	}

	point = QPoint(x, y);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	findPathFuture_ = QtConcurrent::run(injector.worker.get(), &Worker::findPathAsync, point);
#else
	findPathFuture_ = QtConcurrent::run(&Worker::findPathAsync, injector.worker.get(), point);
#endif

	ui.pushButton_findPath->setEnabled(false);
}