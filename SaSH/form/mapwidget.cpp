#include "stdafx.h"
#include "mapwidget.h"
#include "util.h"
#include "injector.h"
#include "net/tcpserver.h"
#include "map/mapanalyzer.h"
#include "script/interpreter.h"

constexpr int MAP_REFRESH_TIME = 100;
constexpr int MAX_BLOCK_SIZE = 24;

MapWidget::MapWidget(QWidget* parent)
	: QWidget(parent)

{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	setStyleSheet("background-color:rgb(0,0,1)");
	//setAttribute(Qt::WA_PaintOnScreen, true);
	setAttribute(Qt::WA_OpaquePaintEvent, true);
	//setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setAttribute(Qt::WA_NoSystemBackground);
	setAttribute(Qt::WA_WState_WindowOpacitySet);
	//setWindowOpacity(0.9);

	//set header text
	ui.tableWidget_NPCList->setColumnCount(2);
	ui.tableWidget_NPCList->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

	//vec invisible
	ui.tableWidget_NPCList->verticalHeader()->setVisible(false);
	ui.tableWidget_NPCList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);//隐藏滚动条

	readSettings();

#if !OPEN_GL_ON
	ui.openGLWidget->close();
	connect(&m_timer, &QTimer::timeout, [this]()
		{
			update();
		}
	);
	m_timer.start(1000);
#else
	connect(&gltimer_, &QTimer::timeout, this, &MapWidget::onRefreshTimeOut);
	gltimer_.start(MAP_REFRESH_TIME);
#endif

	connect(&downloadMapTimer_, &QTimer::timeout, this, &MapWidget::onDownloadMapTimeout);
	downloadMapTimer_.start(500);

	util::FormSettingManager formManager(this);
	formManager.loadSettings();
}

MapWidget::~MapWidget()
{
#if !OPEN_GL_ON
	m_timer.stop();
#endif
	writeSettings();
	qDebug() << "";
}

void MapWidget::writeSettings()
{

}

void MapWidget::readSettings()
{

}

void MapWidget::closeEvent(QCloseEvent*)
{
	isDownloadingMap_ = false;
	gltimer_.stop();
	downloadMapTimer_.stop();
	writeSettings();
	onClear();
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
		tmp_zoom = ((p->height()) / (imageHeight));
		*scaleWidth = ((imageWidth) * (tmp_zoom + fix));
		*scaleHeight = ((imageHeight) * (tmp_zoom + fix));
	}
	if (0.0 == (tmp_zoom))
		tmp_zoom = 1.0;

	*zoom_value = tmp_zoom + fix;
}

#if !OPEN_GL_ON
void CHECK_FOLDER(const QString& current_dir, const QString& map_dirPath)
{
	const QDir dir(map_dirPath);
	if (!dir.exists(map_dirPath))
		dir.mkdir(map_dirPath);

	//建立子資料夾
	int j = 0;
	for (int i = 0; i <= 1; ++i)
	{
		QString msubdir(QString("%1/map/%2").arg(current_dir).arg(i));
		const QDir sub(msubdir);
		if (!sub.exists(msubdir))
			sub.mkdir(msubdir);
		if (i == 1)
		{
			for (j = 1; j <= MAX_THREAD; ++j)
			{
				QString msubsubdir(QString("%1/map/%2/%3").arg(current_dir).arg(i).arg(j));
				const QDir subsub(msubsubdir);
				if (!subsub.exists(msubsubdir))
					subsub.mkdir(msubsubdir);
			}
		}
	}
}

void MapWidget::paintEvent(QPaintEvent* paint)
{
	if (!qasm)
	{
		close();
		return;
	}

	if (paint->type() == QEvent::Quit || paint->type() == QEvent::None) return;

	const QString current_dir(qgetenv("CGHP_DIR_PATH_UTF8"));
	const QString map_dirPath(QString("%1/map").arg(current_dir));
	CHECK_FOLDER(current_dir, map_dirPath);

	int width = 0, height = 0;


	const QString map_current_fileName(qasm->GetCurrentMapPath());
	if (map_current_fileName.isEmpty())return;

	const QString bitmap_fileName(QString("%1/%2.bmp").arg(current_dir).arg(map_current_fileName.mid(0, map_current_fileName.lastIndexOf('.'))));
	bool bret = false;

	Char* _ch = _CHAR_getChar(qasm);
	QPoint qp_current = _ch->p;

	setWindowTitle(QString("map:%1 floor:%2 [%3,%4] file:%5").arg(_ch->map).arg(_ch->floor).arg(qp_current.x()).arg(qp_current.y()).arg(map_current_fileName));

	if (!qasm._MAP_ReadFromBinary(qasm, true))
		return;

	QPixmap m_pix(bitmap_fileName, "BMP", Qt::ImageConversionFlag::NoOpaqueDetection);

	qreal zoom_value = 0.0;

	qreal scaleWidth_ = width();
	qreal m_scalHeight = height();

	zoom(this, m_pix, &scaleWidth_, &m_scalHeight, &zoom_value);
	//resize(static_cast<int>(scaleWidth_), static_cast<int>(m_scalHeight));
	//setUpdatesEnabled(false);

	QPainter paintImage;
	paintImage.begin(this);
	paintImage.drawPixmap(0, 0, static_cast<int>(scaleWidth_), static_cast<int>(m_scalHeight), m_pix);
	paintImage.end();

	//畫刷。填充幾何圖形的調色板，由顏色和填充風格組成
	QPen m_penBlue(QColor(65, 105, 225));
	QBrush m_brushBlue(QColor(65, 105, 225), Qt::SolidPattern);
	QPainter lineChar;
	lineChar.begin(this);
	lineChar.setBrush(m_brushBlue);
	lineChar.setPen(m_penBlue);
	lineChar.drawLine(0, (qp_current.y() * zoom_value), width(), qp_current.y() * zoom_value);//繪制橫向線
	lineChar.drawLine(qp_current.x() * zoom_value, 0, (qp_current.x() * zoom_value), height());	//繪制縱向線
	lineChar.end();

	QPainter rectChar;
	QPen m_penRed(Qt::red);
	QBrush m_brushRed(Qt::red, Qt::SolidPattern);
	rectChar.begin(this);
	rectChar.setBrush(m_brushRed);
	rectChar.setPen(m_penRed);
	rectChar.drawRect((qp_current.x() * zoom_value) + 1, ((qp_current.y() - 1) * zoom_value), zoom_value, zoom_value);//繪制橫向線
	rectChar.end();
	//setUpdatesEnabled(true);
}
#else

void MapWidget::onRefreshTimeOut()
{

	Injector& injector = Injector::getInstance();
	if (injector.server.isNull()) return;


	if (!injector.server->IS_ONLINE_FLAG) return;

	PC _ch = injector.server->pc;
	int floor = injector.server->nowFloor;
	const QPointF qp_current = injector.server->nowPoint;

	QString caption(tr("%1 map:%2 floor:%3 [%4,%5] mouse:%6,%7")
		.arg(_ch.name)
		.arg(injector.server->nowFloorName)
		.arg(floor)
		.arg(qp_current.x()).arg(qp_current.y())
		.arg(qFloor(curMousePos_.x())).arg(qFloor(curMousePos_.y())));

	if (isDownloadingMap_)
		caption += " " + tr("downloading(%1%2)").arg(QString::number(downloadMapProgress_, 'f', 2)).arg("%");
	setWindowTitle(caption);

	int map_ret = injector.server->mapAnalyzer->readFromBinary(floor, injector.server->nowFloorName, true);
	if (map_ret <= 0)
		return;

	QPixmap ppix = injector.server->mapAnalyzer->getPixmapByIndex(floor);
	if (ppix.isNull())  return;

	map_t m_map = {};
	injector.server->mapAnalyzer->getMapDataByFloor(floor, &m_map);

	util::SafeHash<int, mapunit_t> unitHash = injector.server->mapUnitHash;
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

	for (const qmappoint_t& it : m_map.stair)
	{
		if (stair_cache.contains(it.p)) continue;
		QString typeStr = "\0";
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

		stair_cache.insert(it.p);
		mapunit_t u = {};
		if (findMapUnitByPoint(it.p, &u))
		{
			if (!u.name.isEmpty() && (u.name != _ch.name))
			{
				if (u.isvisible)
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

	const QList<mapunit_t> units = unitHash.values();
	for (const mapunit_t& it : units)
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
				if (it.isvisible)
					vPET.append(it.freeName.isEmpty() ? QString(tr("[P]%2")).arg(it.name) : QString(tr("[P]%2")).arg(it.freeName));
				else
					vPET.append(it.freeName.isEmpty() ? QString(tr("X[P]%2")).arg(it.name) : QString(tr("[P]%2")).arg(it.freeName));
				vPET.append(QString("%1,%2").arg(it.p.x()).arg(it.p.y()));
				break;
			}
			case util::OBJ_HUMAN:
			{
				if (it.isvisible)
					vHUMAN.append(QString(tr("[H]%1")).arg(it.name));
				else
					vHUMAN.append(QString(tr("X[H]%1")).arg(it.name));
				vHUMAN.append(QString("%1,%2").arg(it.p.x()).arg(it.p.y()));
				break;
			}
			case util::OBJ_NPC:
			{
				if (it.isvisible)
					vNPC.append(QString("[NPC]%1").arg(it.name));
				else
					vNPC.append(QString(tr("X[NPC]%1")).arg(it.name));
				vNPC.append(QString("%1,%2").arg(it.p.x()).arg(it.p.y()));
				break;
			}
			case util::OBJ_JUMP:
			case util::OBJ_DOWN:
			case util::OBJ_UP:
			{
				if (stair_cache.contains(it.p)) continue;
				QString typeStr = "\0";
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

				if (!it.name.isEmpty())
				{
					if (it.isvisible)
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
		if (it.name.contains(u8"傳送石"))
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
	//if (MAP_TPYE_MAZE == m_map.ismaze)
	//vlist.append(vSTAIR);
	vlist.append(vGM);
	vlist.append(vNPC);
	vlist.append(vITEM);
	vlist.append(vGOLD);
	vlist.append(vPET);
	vlist.append(vHUMAN);
	//if (MAP_TPYE_NORMAL == m_map.ismaze)
	vlist.append(vSTAIR);
	dataVar.setValue(vlist);
	updateNpcListAllContents(dataVar);
#endif

	zoom(ui.widget, ppix, &scaleWidth_, &scaleHeight_, &zoom_value_, fix_zoom_value_);

	if (ui.openGLWidget->width() != static_cast<int>(scaleWidth_) || ui.openGLWidget->height() != static_cast<int>(scaleHeight_))
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

void MapWidget::on_openGLWidget_notifyMousePosition(const QPointF& pos)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;

	if (!injector.server->IS_ONLINE_FLAG)
		return;

	if (!interpreter_.isNull() && interpreter_->isRunning())
		return;

	interpreter_.reset(new Interpreter());


	const QPointF predst(pos / zoom_value_);
	const QPoint dst(predst.toPoint());

	int x = dst.x();
	int y = dst.y();

	if (x < 0 || x > 1500 || y < 0 || y > 1500)
		return;

	interpreter_->doString(QString(u8"尋路 %1, %2, 1").arg(x).arg(y));
}

void MapWidget::on_openGLWidget_notifyLeftClick(const QPointF& gpos, const QPointF& pos)
{
	bClicked_ = true;
	pLast_ = gpos - pos;
	/*if (!ThreadManager::isValid(m_index)) return;
	QSharedPointer<MainThread> thread = ThreadManager::value(m_index);
	if (thread.isNull()) return;
	QString retstring("\0");
	const QPointF predst(pos / zoom_value_);
	const QPoint dst(predst.toPoint());
	thread->_MAP_ReadFromBinary(dst, &retstring);
	if (!retstring.isEmpty())
	{
		QClipboard* clipboard = QApplication::clipboard();
		clipboard->setText(retstring);
	}*/
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

#if 0
bool MapWidget::CHECK_BATTLE_THEN_WAITFOR(QAsm& qa)
{
	bool bret = false;
	if (qa.IS_BATTLE_FLAG)
	{
		bret = true;
		do
		{
			if (!qa.IsWindowAlive())
				break;
			if (!isVisible())
				break;
			QThread::msleep(100UL);
		} while (qa.IS_BATTLE_FLAG);
		QThread::msleep((DWORD)qa.GetWorkInt(QAsm::SET_WAITFOR_BATTLE_DELAY_VALUE));
	}
	return bret;
}

bool MapWidget::findpath(QPoint dst)
{
	if (!ThreadManager::isValid(m_index)) return false;
	QSharedPointer<MainThread> thread = ThreadManager::value(m_index);
	if (thread.isNull()) return false;


	ui.openGLWidget->blockSignals(true);
	if (!m_thread.isNull())
	{
		m_thread->on_pause(false);
		m_thread->on_stop(QLuaBase::WORKINT_FROM_USER_STOP_REQUEST_STATE);
		while (!m_thread.isNull()) { QApplication::processEvents(); }
	}
	ui.openGLWidget->blockSignals(false);

	const QString str(QString("map.findpath(%1, %2); sys.EO();").arg(dst.x()).arg(dst.y()));
	m_thread.reset(q_check_ptr(new QLuaThread(thread, QLuaDebuger::WORKINT_DEBUG_DISABLE, str, this, this)));
	connect(m_thread.get(), &QLuaThread::clear, this, &MapWidget::on_clear);
	if (m_thread.isNull()) return false;
	m_thread->setThreadTypeAsSubThread();
	m_thread->start(QThread::Priority::TimeCriticalPriority);
	return true;
}
#endif

void MapWidget::downloadNextBlock()
{
	int blockWidth = qMin(MAX_BLOCK_SIZE, downloadMapXSize_ - downloadMapX_);
	int blockHeight = qMin(MAX_BLOCK_SIZE, downloadMapYSize_ - downloadMapY_);

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

	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
	{
		isDownloadingMap_ = false;
		return;
	}

	injector.server->downloadMap(downloadMapX_, downloadMapY_);
	int floor = injector.server->nowFloor;
	QString name = injector.server->nowFloorName;

	//downloadMapX_ += 24;

	//if (downloadMapX_ > downloadMapXSize_)
	//{
	//	downloadMapX_ = 0;
	//	downloadMapY_ += 24;
	//}

	downloadNextBlock();

	//downloadMapProgress_ = (static_cast<qreal>(downloadMapY_) / static_cast<qreal>(downloadMapYSize_)) * 100.0;

	if (downloadMapProgress_ >= 100.0)
	{
		//downloadMapProgress_ = 100.0;
		const QPoint qp_current = injector.server->nowPoint;
		QString caption(tr("%1 map:%2 floor:%3 [%4,%5] mouse:%6,%7")
			.arg(injector.server->pc.name)
			.arg(injector.server->nowFloorName)
			.arg(injector.server->nowFloor)
			.arg(qp_current.x()).arg(qp_current.y())
			.arg(qFloor(curMousePos_.x())).arg(qFloor(curMousePos_.y())));
		caption += " " + tr("downloading(%1%2)").arg(QString::number(downloadMapProgress_, 'f', 2)).arg("%");
		setWindowTitle(caption);

		injector.server->mapAnalyzer->clear(floor);
		const QString fileName(QCoreApplication::applicationDirPath() + "/map/" + QString::number(floor) + ".dat");
		QFile file(fileName);
		if (file.exists())
		{
			file.remove();
		}
		isDownloadingMap_ = false;
		injector.server->EO();
		injector.server->mapAnalyzer->readFromBinary(floor, name, true);
	}

}

void MapWidget::on_pushButton_download_clicked()
{

	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
	{
		return;
	}

	ui.pushButton_download->setEnabled(false);

	map_t map = {};
	injector.server->mapAnalyzer->getMapDataByFloor(injector.server->nowFloor, &map);

	downloadCount_ = 0;
	downloadMapXSize_ = map.width;
	downloadMapYSize_ = map.height;
	downloadMapX_ = 0;
	downloadMapY_ = 0;
	downloadMapProgress_ = 0.0;

	int numBlocksX = (downloadMapXSize_ + MAX_BLOCK_SIZE - 1) / MAX_BLOCK_SIZE;
	int numBlocksY = (downloadMapYSize_ + MAX_BLOCK_SIZE - 1) / MAX_BLOCK_SIZE;
	totalMapBlocks_ = static_cast<qreal>(numBlocksX * numBlocksY);

	isDownloadingMap_ = true;

}

void MapWidget::on_pushButton_returnBase_clicked()
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
	{
		return;
	}

	injector.server->logBack();

	//if (!ThreadManager::isValid(m_index)) return;
	//QSharedPointer<MainThread> thread = ThreadManager::value(m_index);
	//thread->_SYS_returnbase();
}

void MapWidget::on_pushButton_findPath_clicked()
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;

	if (!injector.server->IS_ONLINE_FLAG)
		return;

	if (!interpreter_.isNull() && interpreter_->isRunning())
		return;

	interpreter_.reset(new Interpreter());

	int x = ui.spinBox_findPathX->value();
	int y = ui.spinBox_findPathY->value();
	if (x < 0 || x > 1500 || y < 0 || y > 1500)
		return;

	interpreter_->doString(QString(u8"尋路 %1, %2, 1").arg(x).arg(y));

	//findpath(dst);
}

void MapWidget::onClear()
{
	if (!interpreter_.isNull() && interpreter_->isRunning())
	{
		interpreter_->stop();
	}
}
#endif

void MapWidget::updateNpcListAllContents(const QVariant& d)
{
	int col = 0;
	int row = 0;
	int size = 0;
	QVariantList D = d.value<QVariantList>();

	ui.tableWidget_NPCList->setUpdatesEnabled(false);
	for (const QVariant& it : D)
	{
		size += it.value<QStringList>().size();
	}
	size /= 2;
	if (size > ui.tableWidget_NPCList->rowCount())
	{
		for (int i = 0; i < size - ui.tableWidget_NPCList->rowCount(); ++i)
			ui.tableWidget_NPCList->insertRow(ui.tableWidget_NPCList->rowCount());
	}
	else if (size < ui.tableWidget_NPCList->rowCount())
	{
		for (int i = 0; i < (ui.tableWidget_NPCList->rowCount() - size); ++i)
			ui.tableWidget_NPCList->removeRow(ui.tableWidget_NPCList->rowCount() - 1);
	}
	ui.tableWidget_NPCList->setUpdatesEnabled(true);

	for (const QVariant& it : D)
	{
		QStringList list = it.value<QStringList>();

		for (const QString& i : list)
		{
			QTableWidgetItem* item = ui.tableWidget_NPCList->item(row, col);
			if (item)
			{
				if (item->text() != i)
					item->setText(i);
			}
			else
			{
				item = q_check_ptr(new QTableWidgetItem(i));
				ui.tableWidget_NPCList->setItem(row, col, item);
			}
			++col;
			if (col >= 2)
			{
				++row;
				col = 0;
			}
		}
	}

	//ui.tableWidget_NPCList->setUpdatesEnabled(true);
}

void MapWidget::on_tableWidget_NPCList_cellDoubleClicked(int row, int)
{
	QTableWidgetItem* item = ui.tableWidget_NPCList->item(row, 1);
	QTableWidgetItem* item_name = ui.tableWidget_NPCList->item(row, 0);
	if (!item || !item_name)
	{
		return;
	}

	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;

	if (!injector.server->IS_ONLINE_FLAG)
		return;

	if (!interpreter_.isNull() && interpreter_->isRunning())
		return;

	interpreter_.reset(new Interpreter());

	QString name(item_name->text());
	QString text(item->text());
	QStringList list(text.split(util::rexComma));
	if (list.size() != 2)
		return;

	bool okx, oky;
	int x = list.at(0).toInt(&okx);
	int y = list.at(1).toInt(&oky);
	if (!okx || !oky)
		return;

	if (x < 0 || x > 1500 || y < 0 || y > 1500)
		return;

	mapunit_t unit;
	if (name.contains("NPC"))
	{
		name = name.remove("[NPC]");
		if (!injector.server->findUnit(name, util::OBJ_NPC, &unit))
			return;
	}
	else if (name.contains(tr("[P]")))
	{
		name = name.remove(tr("[P]"));
		if (!injector.server->findUnit(name, util::OBJ_PET, &unit))
			return;
	}
	else if (name.contains(tr("[H]")))
	{
		name = name.remove(tr("[H]"));
		if (!injector.server->findUnit(name, util::OBJ_HUMAN, &unit))
			return;
	}
	else if (name.contains(tr("[I]")))
	{
		name = name.remove(tr("[I]"));
		if (!injector.server->findUnit(name, util::OBJ_ITEM, &unit))
			return;
	}
	else if (name.contains(tr("[G]")))
	{
		name = name.remove(tr("[G]"));
		if (!injector.server->findUnit(name, util::OBJ_GOLD, &unit))
			return;
	}
	else
		return;
	int floor = injector.server->nowFloor;
	QPoint point = injector.server->nowPoint;
	//npc前方一格
	QPoint newPoint = util::fix_point.at(unit.dir) + unit.p;
	//檢查是否可走
	if (injector.server->mapAnalyzer->isPassable(floor, point, newPoint))
	{
		x = newPoint.x();
		y = newPoint.y();
	}
	else
	{
		//再往前一格
		QPoint additionPoint = util::fix_point.at(unit.dir) + newPoint;
		//檢查是否可走
		if (injector.server->mapAnalyzer->isPassable(floor, point, additionPoint))
		{
			x = additionPoint.x();
			y = additionPoint.y();
		}
		else
		{
			//檢查NPC周圍8格
			bool flag = false;
			for (int i = 0; i < 8; i++)
			{
				newPoint = util::fix_point.at(i) + unit.p;
				if (injector.server->mapAnalyzer->isPassable(floor, point, newPoint))
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

	interpreter_->doString(QString(u8"尋路 %1, %2, 1").arg(x).arg(y));

	//	if (list.size() == 2)
	//	{
	//		bool okx, oky;
	//		int x = list.at(0).toInt(&okx);
	//		int y = list.at(1).toInt(&oky);
	//		if (okx && oky && x >= 0 && y >= 0)
	//		{
	//			QPoint dst(x, y);
	//			if (!ThreadManager::isValid(m_index)) return;
	//			QSharedPointer<MainThread> thread = ThreadManager::value(m_index);
	//			if (thread.isNull()) return;


	//			ui.openGLWidget->blockSignals(true);
	//			if (!m_thread.isNull())
	//			{
	//				m_thread->on_pause(false);
	//				m_thread->on_stop(QLuaBase::WORKINT_FROM_USER_STOP_REQUEST_STATE);
	//				while (!m_thread.isNull()) { QApplication::processEvents(); }
	//			}
	//			ui.openGLWidget->blockSignals(false);
	//			QString str = "";
	//			if (!name.isEmpty() && (name.contains(tr("DWON")) || name.contains(tr("JUMP")) || name.contains(tr("UP")) || name.contains(tr("WARP"))))
	//			{
	//				str = (QString(R"(map.findpath(%1, %2); sys.EO();)").arg(dst.x()).arg(dst.y()));
	//			}
	//			else
	//			{
	//				str = (QString(R"(
	//					local t = map.getfollowinfo(char.x, char.y, %1, %2);
	//					if (t ~= nil) then
	//						if map.findpath(t.x, t.y) then
	//							sleep(1000);
	//							map.setdir(t.dir);
	//						end
	//					end
	//			)").arg(dst.x()).arg(dst.y()));
	//			}
	//			m_thread.reset(q_check_ptr(new QLuaThread(thread, QLuaDebuger::WORKINT_DEBUG_DISABLE, str, this, this)));
	//			connect(m_thread.get(), &QLuaThread::clear, this, &MapWidget::on_clear);
	//			if (m_thread.isNull()) return;
	//			m_thread->setThreadTypeAsSubThread();
	//			m_thread->start(QThread::Priority::TimeCriticalPriority);
	//		}
	//	}
	//}
}