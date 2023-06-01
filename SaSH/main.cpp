#include "stdafx.h"
#include "mainform.h"
#include <QtWidgets/QApplication>

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

	UINT acp = GetACP();
	QFont defaultFont;
	if (acp == 950)
	{
		defaultFont = QFont("新細明體", 12, QFont::Normal);
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


	qputenv("DIR_PATH", QCoreApplication::applicationDirPath().toUtf8());
	qputenv("JSON_PATH", (QCoreApplication::applicationDirPath() + "/system.json").toUtf8());

	MainForm w;
	w.show();
	return a.exec();
}
