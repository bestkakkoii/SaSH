#ifndef BCEDITOR_H
#define BCEDITOR_H

#ifndef QSCINTILLA_DLL
#define QSCINTILLA_DLL
#endif

#ifndef QSCINTILLA_MAKE_DLL
#define QSCINTILLA_MAKE_DLL
#endif

#include <Qsci/qsciscintilla.h>
#include "QSci/qsciapis.h"
#include <QStyleOption>
#include <QPainter>
#include <highlighter.h>

class CodeEditor : public QsciScintilla
{
	Q_OBJECT
public:
	typedef enum
	{
		SYM_POINT = 0,
		SYM_ARROW,
		SYM_TRIANGLE,
		SYM_STEP,
	}SymbolHandler;
	explicit CodeEditor(QWidget* parent = nullptr);
	int S_BREAK = 0b0001;
	int S_ARROW = 0b0010;
	int S_ERRORMARK = 0b0100;
	int S_STEPMARK = 0b1000;

	inline bool isBreak(int mode) const { return ((mode & S_BREAK) == S_BREAK); }
	inline bool isArrow(int mode) const { return ((mode & S_ARROW) == S_ARROW); }
	inline bool isError(int mode) const { return ((mode & S_ERRORMARK) == S_ERRORMARK); }
	inline bool isStep(int mode) const { return ((mode & S_STEPMARK) == S_STEPMARK); }

public slots:
	void commentSwitch();

private:
	Highlighter textLexer;
	QsciAPIs apis;
	QFont font;
	QFont linefont;

protected:
	void keyPressEvent(QKeyEvent* e);

	void showEvent(QShowEvent* e) override
	{
		setAttribute(Qt::WA_Mapped);
		QWidget::showEvent(e);
	}
};
#endif // HIGHLIGHTER_H
