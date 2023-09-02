﻿#include "stdafx.h"
#include "copyrightdialog.h"
#include <QWhatsThis>

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

#include <QGraphicsSvgItem>
class ClickableSvgItem : public QGraphicsSvgItem
{
public:
	ClickableSvgItem(const QString& svgFilePath, const QString& linkUrl = "", QGraphicsItem* parent = nullptr)
		: QGraphicsSvgItem(svgFilePath, parent), linkUrl_(linkUrl)
	{
		setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
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


	ui.label_logo->setStyleSheet("color: rgb(111, 147, 198); font-family: 'Consolas';");

	const QString programName(u8"StoneAge Supreme Helper");
	const QString companyName(u8"Bestkakkoii llc.");
	const QString AuthorName(u8"Philip飞");
	const QString webUrl(u8"https://www.lovesa.cc");
	constexpr int nowSysBit = QSysInfo::WordSize;
	constexpr int yearStart = 2023;
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

	//	ui.textBroswer_webicon->setHtml(R"(<div align="center">
	//<div>
	//	<img src=":/image/icon_cplusplus17.svg" alt="C++" title="C++" width="73" height="20" style="border-radius: 50%;" />
	//</div>
	//<div>
	//	<img src=":/image/icon_qt-5.15.svg" alt="Qt" title="Qt" width="85" height="20" style="border-radius: 50%;" />
	//	<img src=":/image/icon_vs-2022.svg" alt="Visual Studio" title="Visual Studio" width="137" height="20" style="border-radius: 50%;" />
	//</div>
	//<div>
	//	<img src=":/image/icon_platform-Windows-blueviolet.svg" alt="Platform" title="Platform" width="116" height="20" style="border-radius: 50%;" />
	//	<img src=":/image/icon_windows_10_11.svg" alt="Windows" title="Windows" width="125" height="20" style="border-radius: 50%;" />
	//</div>
	//<div>
	//	<img src=":/image/icon_license.svg" alt="License" title="License" width="102" height="20" style="border-radius: 50%;" />
	//	<a target="_blank" href="https://github.com/bestkakkoii/SaSH"><img border="0" src=":/image/icon_github.svg" alt="Github" title="Github" width="135" height="20" style="border-radius: 50%;" /></a>
	//</div>)");
	//
	//	ui.textBroswer_webicon->setOpenExternalLinks(true);

	ui.label_nameandver->setText(QObject::tr("%1 (%2 bit) - %3.%4.%5")
		.arg(programName).arg(nowSysBit).arg(SASH_VERSION_MAJOR).arg(SASH_VERSION_MINOR).arg(util::buildDateTime(nullptr)));

	ui.label_version->setText(QObject::tr("Version %1.%2.%3")
		.arg(SASH_VERSION_MAJOR).arg(SASH_VERSION_MINOR).arg(util::buildDateTime(nullptr)));

	ui.label_copyrighttext->setText(QString(u8"© %1 %2").arg(yearStart).arg(companyName));

	ui.label_txt->setText(QObject::tr("All right reserved."));

	ui.label_link->setText(QString(u8R"(<a href="%1" style="color:#6586B5; font-size: 14px; font-family: Consolas;"><strong>%2</strong> by %3</a>)")
		.arg(webUrl).arg("lovesa").arg(AuthorName));

	ui.label_link->setOpenExternalLinks(true);

	ui.label_group->setText(QString(u8R"(<a target="_blank" href="%1"><img border="0" src="%2" alt="%3" title="%4"></a>)")
		.arg(qqLink).arg(qqImage).arg(qqLiskTitle).arg(qqLiskTitle));
	ui.label_group->setOpenExternalLinks(true);

	ui.label_programname->setText(programName);

	ui.label_warnings->setText(
		QObject::tr("Warning: This project is only for academic purposes," \
			"commercial use is prohibited." \
			"You are prohibited to publish this project elsewhere." \
			"However we make no promises to your game accounts and so you have to use this project at your own risk," \
			"including taking any damage to your accounts from scripts and binaries."));

	ui.label_thanks->setText(u8"特别感谢: eric、辉、match_stick、手柄、老花、小雅、大头鱼、Jin、瑤瑤、大树、gjw000 热心帮忙测试、查找Bug和给予大量优质的建议");

	connect(ui.pushButton_copyinfo, &QPushButton::clicked, this, &CopyRightDialog::pushButton_copyinfo_clicked);
	connect(ui.pushButton_sysinfo, &QPushButton::clicked, this, &CopyRightDialog::pushButton_sysinfo_clicked);
	connect(ui.pushButton_dxdiag, &QPushButton::clicked, this, &CopyRightDialog::pushButton_dxdiag_clicked);
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

	int size = ui.listWidget->count();
	for (int i = 0; i < size; ++i)
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