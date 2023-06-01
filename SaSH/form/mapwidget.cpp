#include "stdafx.h"
#include "mapwidget.h"
#include "util.h"
#include "injector.h"
#include "net/tcpserver.h"
#include "map/mapanalyzer.h"

constexpr int MAP_REFRESH_TIME = 50;

MapWidget::MapWidget(QWidget* parent)
	: QWidget(parent)

{
	ui.setupUi(this);
	this->setAttribute(Qt::WA_DeleteOnClose);
	this->setStyleSheet("background-color:rgb(0,0,1)");
	//this->setAttribute(Qt::WA_PaintOnScreen, true);
	//this->setAttribute(Qt::WA_OpaquePaintEvent, true);
	//this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	//this->setAttribute(Qt::WA_NoSystemBackground);
	//this->setAttribute(Qt::WA_WState_WindowOpacitySet);
	//this->setWindowOpacity(0.9);

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
			this->update();
		}
	);
	m_timer.start(1000);
#else
	connect(&gltimer, &QTimer::timeout, this, &MapWidget::on_timeOut);
	gltimer.start(MAP_REFRESH_TIME);
#endif

	connect(&downloadMapTimer, &QTimer::timeout, this, &MapWidget::on_downloadMapTimeout);
	downloadMapTimer.start(100);


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
	m_IsDownloadingMap = false;
	gltimer.stop();
	downloadMapTimer.stop();
	writeSettings();
	this->on_clear();
	//emit on_close(m_index);
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
		this->close();
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

	this->setWindowTitle(QString("map:%1 floor:%2 [%3,%4] file:%5").arg(_ch->map).arg(_ch->floor).arg(qp_current.x()).arg(qp_current.y()).arg(map_current_fileName));

	if (!qasm._MAP_ReadFromBinary(qasm, true))
		return;

	QPixmap m_pix(bitmap_fileName, "BMP", Qt::ImageConversionFlag::NoOpaqueDetection);

	qreal zoom_value = 0.0;

	qreal m_scaleWidth = this->width();
	qreal m_scalHeight = this->height();

	zoom(this, m_pix, &m_scaleWidth, &m_scalHeight, &zoom_value);
	//this->resize(static_cast<int>(m_scaleWidth), static_cast<int>(m_scalHeight));
	//this->setUpdatesEnabled(false);

	QPainter paintImage;
	paintImage.begin(this);
	paintImage.drawPixmap(0, 0, static_cast<int>(m_scaleWidth), static_cast<int>(m_scalHeight), m_pix);
	paintImage.end();

	//畫刷。填充幾何圖形的調色板，由顏色和填充風格組成
	QPen m_penBlue(QColor(65, 105, 225));
	QBrush m_brushBlue(QColor(65, 105, 225), Qt::SolidPattern);
	QPainter lineChar;
	lineChar.begin(this);
	lineChar.setBrush(m_brushBlue);
	lineChar.setPen(m_penBlue);
	lineChar.drawLine(0, (qp_current.y() * zoom_value), this->width(), qp_current.y() * zoom_value);//繪制橫向線
	lineChar.drawLine(qp_current.x() * zoom_value, 0, (qp_current.x() * zoom_value), this->height());	//繪制縱向線
	lineChar.end();

	QPainter rectChar;
	QPen m_penRed(Qt::red);
	QBrush m_brushRed(Qt::red, Qt::SolidPattern);
	rectChar.begin(this);
	rectChar.setBrush(m_brushRed);
	rectChar.setPen(m_penRed);
	rectChar.drawRect((qp_current.x() * zoom_value) + 1, ((qp_current.y() - 1) * zoom_value), zoom_value, zoom_value);//繪制橫向線
	rectChar.end();
	//this->setUpdatesEnabled(true);
}
#else

void MapWidget::on_timeOut()
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull()) return;


	if (!injector.server->IS_ONLINE_FLAG) return;

	PC _ch = injector.server->pc;
	int floor = injector.server->nowFloor;
	const QPointF qp_current = QPoint(injector.server->nowGx, injector.server->nowGy);

	QString caption = QString(tr("%1 map:%2 floor:%3 [%4,%5] file:%6 mouse:%7,%8")) \
		.arg(_ch.name).arg(injector.server->nowFloorName).arg(floor) \
		.arg(qp_current.x()).arg(qp_current.y()) \
		.arg(QString("./map/%1.dat").arg(floor)) \
		.arg(qFloor(m_curMousePos.x())).arg(qFloor(m_curMousePos.y()));

	if (m_IsDownloadingMap)
	{
		caption += " " + QString(tr("downloading(%1%2)")).arg(QString::number(m_downloadMapProgress, 'f', 2)).arg("%");
	}

	this->setWindowTitle(caption);

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
	this->on_NPCListUpdateAllContent(dataVar);
#endif

	zoom(ui.widget, ppix, &m_scaleWidth, &m_scaleHeight, &m_zoom_value, m_fix_zoom_value);

	if (ui.openGLWidget->width() != static_cast<int>(m_scaleWidth) || ui.openGLWidget->height() != static_cast<int>(m_scaleHeight))
		ui.openGLWidget->resize(m_scaleWidth, m_scaleHeight);


	m_rectangle_src = QRectF{ 0.0, 0.0, static_cast<qreal>(ppix.width()), static_cast<qreal>(ppix.height()) };
	m_rectangle_dst = QRectF(0.0, 0.0, m_scaleWidth, m_scaleHeight);
	ui.openGLWidget->setPix(ppix, m_rectangle_src, m_rectangle_dst);

	//人物座標十字
	ui.openGLWidget->setLineH({ 0.0, (qp_current.y() * m_zoom_value) }, { static_cast<qreal>(ui.openGLWidget->width()), qp_current.y() * m_zoom_value });
	ui.openGLWidget->setLineV({ qp_current.x() * m_zoom_value, 0.0 }, { (qp_current.x() * m_zoom_value), static_cast<qreal>(ui.openGLWidget->height()) });
	//人物座標方格
	ui.openGLWidget->setRect({ (qp_current.x() * m_zoom_value) + 0.5, ((qp_current.y() - 1.0) * m_zoom_value) + m_zoom_value + 0.5, m_zoom_value, m_zoom_value });

	ui.openGLWidget->repaint(); //窗口重绘，repaint会调用paintEvent函数，paintEvent会调用paintGL函数实现重绘
}

void MapWidget::on_openGLWidget_notifyMouseMove(Qt::MouseButton, const QPointF& gpos, const QPointF& pos)
{
	if (m_bClicked)
	{
		m_MovePoint = gpos;
		ui.openGLWidget->repaint();
	}
	const QPointF dst(pos / m_zoom_value);
	m_curMousePos = dst;

	//滑鼠十字
	ui.openGLWidget->setCurLineH({ 0.0, (m_curMousePos.y() * m_zoom_value) }, { static_cast<qreal>(ui.openGLWidget->width()), m_curMousePos.y() * m_zoom_value });
	ui.openGLWidget->setCurLineV({ m_curMousePos.x() * m_zoom_value, 0.0 }, { (m_curMousePos.x() * m_zoom_value), static_cast<qreal>(ui.openGLWidget->height()) });

	ui.openGLWidget->repaint();
}

void MapWidget::leaveEvent(QEvent*)
{
	m_curMousePos = QPointF(-1, -1);
}

void MapWidget::on_openGLWidget_notifyRightClick()
{
	this->on_clear();
}

void MapWidget::on_openGLWidget_notifyMousePosition(const QPointF& pos)
{
	const QPointF predst(pos / m_zoom_value);
	const QPoint dst(predst.toPoint());

	//findpath(dst);
}

void MapWidget::on_openGLWidget_notifyLeftClick(const QPointF& gpos, const QPointF& pos)
{
	m_bClicked = true;
	pLast = gpos - pos;
	/*if (!ThreadManager::isValid(m_index)) return;
	QSharedPointer<MainThread> thread = ThreadManager::value(this->m_index);
	if (thread.isNull()) return;
	QString retstring("\0");
	const QPointF predst(pos / m_zoom_value);
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
	m_bClicked = false;
}

void MapWidget::on_openGLWidget_notifyWheelMove(const QPointF& zoom, const QPointF&)
{
	m_fix_zoom_value += (zoom.y() > 0) ? 0.1 : -0.1;
	on_timeOut();
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
			if (!this->isVisible())
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
	QSharedPointer<MainThread> thread = ThreadManager::value(this->m_index);
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

void MapWidget::on_downloadMapTimeout()
{
	if (!m_IsDownloadingMap)
	{
		if (!ui.pushButton_download->isEnabled())
			ui.pushButton_download->setEnabled(true);
		return;
	}

	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
	{
		m_IsDownloadingMap = false;
		return;
	}

	injector.server->downloadMap(m_downloadMapX, m_downloadMapY);
	int floor = injector.server->nowFloor;
	QString name = injector.server->nowFloorName;

	m_downloadMapX += 24;

	if (m_downloadMapX > m_downloadMapXSize)
	{
		m_downloadMapX = 0;
		m_downloadMapY += 24;
	}

	if (m_downloadMapY > m_downloadMapYSize)
	{
		m_IsDownloadingMap = false;
		m_downloadMapProgress = 100.0;
		injector.server->mapAnalyzer->clear(floor);
		//thread->_MAP_GetUnitsFromMemory();
		injector.server->EO();
		injector.server->mapAnalyzer->readFromBinary(floor, name, true);
	}

	//計算百分比
	m_downloadMapProgress = (static_cast<qreal>(m_downloadMapY) / static_cast<qreal>(m_downloadMapYSize)) * 100.0;
}

int max_download_count = 50;
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

	m_downloadMapXSize = map.width;
	m_downloadMapYSize = map.height;
	m_downloadMapX = 0;
	m_downloadMapY = 0;
	m_downloadMapProgress = 0.0;
	m_IsDownloadingMap = true;

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
	//QSharedPointer<MainThread> thread = ThreadManager::value(this->m_index);
	//thread->_SYS_returnbase();
}

void MapWidget::on_pushButton_findPath_clicked()
{
	int x = ui.spinBox_findPathX->value();
	int y = ui.spinBox_findPathY->value();

	const QPoint dst(x, y);

	//findpath(dst);
}

void MapWidget::on_clear()
{
	//if (!m_thread.isNull())
	//{
	//	m_thread->on_pause(false);
	//	m_thread->on_stop(QLuaBase::WORKINT_FROM_USER_STOP_REQUEST_STATE);
	//	m_thread->wait();
	//	m_thread.clear();
	//	m_thread = nullptr;
	//	qDebug() << "thread finished";
	//}
}
#endif

void MapWidget::on_NPCListUpdateAllContent(const QVariant& d)
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
	//QTableWidgetItem* item = ui.tableWidget_NPCList->item(row, 1);
	//QTableWidgetItem* item_name = ui.tableWidget_NPCList->item(row, 0);
	//if (item && item_name)
	//{
	//	QString name(item_name->text());
	//	QString text(item->text());
	//	QStringList list(text.split(rexComma));
	//	if (list.size() == 2)
	//	{
	//		bool okx, oky;
	//		int x = list.at(0).toInt(&okx);
	//		int y = list.at(1).toInt(&oky);
	//		if (okx && oky && x >= 0 && y >= 0)
	//		{
	//			QPoint dst(x, y);
	//			if (!ThreadManager::isValid(m_index)) return;
	//			QSharedPointer<MainThread> thread = ThreadManager::value(this->m_index);
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