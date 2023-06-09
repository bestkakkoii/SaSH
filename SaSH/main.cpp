#include "stdafx.h"
#include "mainform.h"
#include <QtWidgets/QApplication>

void fontInitialize(QApplication& a)
{
	auto installFont = [](const QString& fontName)->int
	{
		int fontId = QFontDatabase::addApplicationFont(QCoreApplication::applicationDirPath() + "/" + fontName);

		QString fontFilePath = QCoreApplication::applicationDirPath() + "/" + fontName;
		std::wstring fontFilePathW = fontFilePath.toStdWString();
		int res = AddFontResource(fontFilePathW.c_str());
		if (res == 0)
		{
			//qDebug() << "AddFontResource failed";
		}
		else
		{
			//qDebug() << "AddFontResource success";
		}
		return fontId;
	};

	installFont("JoysticMonospace.ttf");
	installFont("YaHei Consolas Hybrid 1.12.ttf");


	// Font size adjustment
	//constexpr qreal DEFAULT_DPI = 96.0;
	//qreal dpiX = QApplication::primaryScreen()->logicalDotsPerInch();
	//qreal fontSize = dpiX / DEFAULT_DPI;
	//qDebug() << QFontDatabase::applicationFontFamilies(fontId);
	//QString fontFamily = QFontDatabase::applicationFontFamilies(fontId).at(0);
	//QFont font(fontFamily);
	//font.setPointSize(font.pointSize() * fontSize - 3);
	//a.setFont(font);

	UINT acp = GetACP();
	QFont defaultFont;
	if (acp == 950)
	{
		defaultFont = QFont(u8"新細明體", 12, QFont::Normal);
	}
	else if (acp == 936)
	{
		defaultFont = QFont("SimSun", 12, QFont::Normal);
	}
	else
	{
		defaultFont = QFont("Arial", 12, QFont::Normal);
	}

	// Font size adjustment
	constexpr qreal DEFAULT_DPI = 96.0;
	qreal dpiX = QApplication::primaryScreen()->logicalDotsPerInch();
	qreal fontSize = dpiX / DEFAULT_DPI;
	QFont font(defaultFont);
	font.setPointSize(font.pointSize() * fontSize - 3);
	a.setFont(font);
}

int main(int argc, char* argv[])
{
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
	QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
	QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
	QApplication a(argc, argv);
	qSetMessagePattern("[%{threadid}] [@%{line}] [%{function}] [%{type}] %{message}");//%{file} 

	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	fontInitialize(a);

	qputenv("DIR_PATH", QCoreApplication::applicationDirPath().toUtf8());
	qputenv("JSON_PATH", (QCoreApplication::applicationDirPath() + "/system.json").toUtf8());

	QStringList filters;
	filters << "*.tmp";
	QDirIterator it(QCoreApplication::applicationDirPath(), filters, QDir::Files, QDirIterator::Subdirectories);
	while (it.hasNext())
	{
		it.next();
		QFile::remove(it.filePath());
	}

	MainForm w;
	w.show();
	return a.exec();
}
