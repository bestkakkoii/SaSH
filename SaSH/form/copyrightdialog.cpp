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
#include "copyrightdialog.h"
#include <QWhatsThis>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QGraphicsSvgItem>
#else
#include <QtSvgWidgets/QGraphicsSvgItem>
#endif

#include "util.h"

static QStringList getInstalledProgramsByKeyword(const QStringList& keywords)
{
	QStringList resultList;

	QString regPath = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
	QSettings uninstallSettings(regPath, QSettings::NativeFormat);
	QStringList subKeys = uninstallSettings.childGroups();

	for (const QString& subKey : subKeys)
	{
		QSettings subKeySettings(regPath + "\\" + subKey, QSettings::NativeFormat);
		QString displayName = subKeySettings.value("DisplayName").toString();
		if (displayName.isEmpty())
			continue;

		for (const QString& keyword : keywords)
		{
			if (displayName.contains(keyword, Qt::CaseInsensitive))
			{
				resultList.append(displayName);
				break;
			}
		}
	}

	resultList.removeDuplicates();

	std::sort(resultList.begin(), resultList.end(), [](const QString& a, const QString& b) {
		return a.compare(b, Qt::CaseInsensitive) < 0;
		});

	return resultList;
}


class ClickableSvgItem : public QGraphicsSvgItem
{
public:
	ClickableSvgItem(const QString& svgFilePath, const QString& linkUrl = "", QGraphicsItem* parent = nullptr)
		: QGraphicsSvgItem(svgFilePath, parent), linkUrl_(linkUrl)
	{
		setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);

		QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect;
		shadowEffect->setBlurRadius(10); // 設置陰影的模糊半徑，根據需要調整
		shadowEffect->setOffset(0, 1);   // 設置陰影的偏移量，根據需要調整
		shadowEffect->setColor(Qt::black); // 設置陰影的顏色，根據需要調整
		setGraphicsEffect(shadowEffect);
	}

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent* event) override
	{
		Q_UNUSED(event);
		if (!linkUrl_.isEmpty())
			QDesktopServices::openUrl(QUrl(linkUrl_));
	}

	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override
	{
		Q_UNUSED(widget);

		// Draw a rounded rectangle around the item
		QPainterPath path;
		path.addRoundedRect(boundingRect(), 3, 3);

		painter->save();

		painter->setPen(Qt::NoPen);
		painter->setBrush(Qt::gray);
		painter->drawPath(path);

		painter->setClipPath(path);
		QGraphicsSvgItem::paint(painter, option, widget);
		painter->restore();
	}

private:
	QString linkUrl_;
};

CopyRightDialog::CopyRightDialog(QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	installEventFilter(this);

	setStyleSheet("QLabel { font-family: 'Consolas';}");

	ui.listWidget->addItems(getInstalledProgramsByKeyword(QStringList{ "C++", ".NET", "Net Framework" }));

	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

	ui.label->setPixmap(QPixmap(":/image/hantapay.png").scaled(150, 150));

	ui.label_logo->setStyleSheet("color: rgb(111, 147, 198); font-family: 'Consolas'; font-size:40px;");

	const QString programName(u8"StoneAge Supreme Helper");
	const QString companyName(u8"Bestkakkoii llc.");
	const QString AuthorName(u8"Philip飞");
	const QString webUrl(u8"https://www.lovesa.cc");
	constexpr qint64 nowSysBit = QSysInfo::WordSize;
	constexpr qint64 yearStart = 2023;
	const QString qqImage(u8R"(:/image/icon_group.png)");
	const QString qqLink(u8"https://qm.qq.com/cgi-bin/qm/qr?"
		"k=Mm_Asx4CFMhLWttW10ScuPngFPFgGNFP" \
		"&jump_from=webapi" \
		"&authKey=4C/22fh/ddibr24j1XMXr4cq3GXffyyKSVEDqP5PeCecCLZnCAIQrF6lD5cXjXql");

	const QString qqLiskTitle(u8"石器助手SaSH");

	setWindowTitle(QObject::tr("About %1").arg(programName));

	ui.label_logo->setText(programName);

	ui.label_icons->setText(R"(<img alt="LOGO" src=":image/ico.png" width="50" height="50" style="border-radius: 50%;" />)");

	QGraphicsScene* scene = new QGraphicsScene(this);
	ui.graphicsView_webicon->setScene(scene);

	ClickableSvgItem* item1 = new ClickableSvgItem(":/image/icon_cplusplus17.svg");
	ClickableSvgItem* item2 = new ClickableSvgItem(":/image/icon_qt-5.15.svg");
	ClickableSvgItem* item3 = new ClickableSvgItem(":/image/icon_vs-2022.svg");
	ClickableSvgItem* item4 = new ClickableSvgItem(":/image/icon_platform-Windows-blueviolet.svg");
	ClickableSvgItem* item5 = new ClickableSvgItem(":/image/icon_windows_10_11.svg");
	ClickableSvgItem* item6 = new ClickableSvgItem(":/image/icon_license.svg");
	ClickableSvgItem* item7 = new ClickableSvgItem(":/image/icon_github.svg", "https://github.com/bestkakkoii/SaSH");

	item1->setPos(70, 0);
	item2->setPos(8, 25);
	item3->setPos(98, 25);
	item4->setPos(0, 50);
	item5->setPos(121, 50);
	item6->setPos(2, 75);
	item7->setPos(109, 75);

	scene->addItem(item1);
	scene->addItem(item2);
	scene->addItem(item3);
	scene->addItem(item4);
	scene->addItem(item5);
	scene->addItem(item6);
	scene->addItem(item7);

	ui.label_nameandver->setText(QObject::tr("%1 (%2 bit) - %3.%4.%5")
		.arg(programName).arg(nowSysBit).arg(SASH_VERSION_MAJOR).arg(SASH_VERSION_MINOR).arg(compile::buildDateTime(nullptr)));

	ui.label_version->setText(QObject::tr("Version %1.%2.%3")
		.arg(SASH_VERSION_MAJOR).arg(SASH_VERSION_MINOR).arg(compile::buildDateTime(nullptr)));

	ui.label_copyrighttext->setText(QString(u8"© %1 %2").arg(yearStart).arg(companyName));

	ui.label_txt->setText(QObject::tr("All right reserved."));

	ui.label_link->setText(QString(u8R"(<a href="%1" style="color:#6586B5; font-size: 14px; font-family: Consolas;"><strong>%2</strong> by %3</a>)")
		.arg(webUrl).arg("lovesa").arg(AuthorName));
	ui.label_link->setOpenExternalLinks(true);

	ui.label_group->setText(QString(u8R"(<a target="_blank" href="%1"><img border="0" src="%2" alt="%3" title="%4"></a>)")
		.arg(qqLink).arg(qqImage).arg(qqLiskTitle).arg(qqLiskTitle));
	ui.label_group->setOpenExternalLinks(true);

	ui.label_programname->setText(programName);

	ui.label_thanks->setText(u8"特别感谢: eric、辉、match_stick、手柄、老花、小雅、大头鱼、Jin、瑤瑤、大树、gjw000、无思走肉、SeasonV龙 热心帮忙测试、查找Bug和给予大量优质的建议");

	ui.label_warnings->setText(
		QObject::tr("Warning: This project is only for academic purposes," \
			"commercial use is prohibited." \
			"You are prohibited to publish this project elsewhere." \
			"However we make no promises to your game accounts and so you have to use this project at your own risk," \
			"including taking any damage to your accounts from scripts and binaries."));

	ui.label_ad->setText(QString(u8R"(<a href="%1" style="color:#6586B5; font-size: 14px; font-family: Consolas;">%2</a>)")
		.arg("https://mysa.cc").arg("盖亚石器攻略网"));
	ui.label_ad->setOpenExternalLinks(true);

	connect(ui.pushButton_copyinfo, &PushButton::clicked, this, &CopyRightDialog::pushButton_copyinfo_clicked);
	connect(ui.pushButton_sysinfo, &PushButton::clicked, this, &CopyRightDialog::pushButton_sysinfo_clicked);
	connect(ui.pushButton_dxdiag, &PushButton::clicked, this, &CopyRightDialog::pushButton_dxdiag_clicked);
}

CopyRightDialog::~CopyRightDialog()
{}

bool CopyRightDialog::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() == QEvent::EnterWhatsThisMode)
	{
		QDesktopServices::openUrl(QUrl("https://www.lovesa.cc/thread-700-1-1.html"));
		event->ignore();
		return true;
	}
	else if (event->type() == QEvent::QueryWhatsThis || event->type() == QEvent::LeaveWhatsThisMode)
	{
		event->ignore();
		return false;
	}

	// For other events, proceed with default processing
	return QDialog::eventFilter(watched, event);
}

void CopyRightDialog::pushButton_copyinfo_clicked()
{
	QStringList infos;

	infos.append(ui.label_nameandver->text());
	infos.append(ui.label_version->text());
	infos.append(ui.label_copyrighttext->text());
	infos.append(ui.label_txt->text());

	qint64 size = ui.listWidget->count();
	for (qint64 i = 0; i < size; ++i)
	{
		QListWidgetItem* item = ui.listWidget->item(i);
		if (item != nullptr && !item->text().isEmpty())
			infos.append(item->text());
	}

	if (infos.isEmpty())
		return;

	QClipboard* clipboard = QApplication::clipboard();
	if (clipboard == nullptr)
		return;

	clipboard->setText(infos.join("\n"));
}

void CopyRightDialog::pushButton_sysinfo_clicked()
{
	QProcess sysInfoProcess;
	sysInfoProcess.startDetached("msinfo32", QStringList());
}

void CopyRightDialog::pushButton_dxdiag_clicked()
{
	QProcess dxdiagProcess;
	dxdiagProcess.startDetached("dxdiag", QStringList());
}