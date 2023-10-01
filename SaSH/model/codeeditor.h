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

#ifndef CODEEDITOR_H
#define CODEEDITOR_H

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
#include "form/replaceform.h"
#include <QDialog>
#include <indexer.h>


class CodeEditor : public QsciScintilla, public Indexer
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

	QFont getOldFont() { return QsciScintilla::font(); }
	void setNewFont(const QFont& f) { font_ = f; setFont(f); textLexer.setDefaultFont(f); }
public slots:
	void commentSwitch();
	void findReplace();

signals:
	void closeJumpToLineDialog();
private:
	Highlighter textLexer;
	QsciAPIs apis;
	QFont font_;
	QFont linefont;
	bool isDialogOpened = false;
protected:
	virtual void keyPressEvent(QKeyEvent* e) override;

	//drop
	virtual void dropEvent(QDropEvent* e) override;

	virtual void showEvent(QShowEvent* e) override
	{
		setAttribute(Qt::WA_Mapped);
		QsciScintilla::showEvent(e);
	}

	virtual void mousePressEvent(QMouseEvent* e) override
	{
		if (isDialogOpened)
			emit closeJumpToLineDialog();

		QsciScintilla::mousePressEvent(e);
	}

private:
	void jumpToLineDialog();

};

class JumpToLineDialog : public QDialog
{
	Q_OBJECT

public:
	JumpToLineDialog(CodeEditor* parent = nullptr, int* lineNumber = nullptr)
		: QDialog(parent), m_lineNumber(lineNumber)
	{
		setAttribute(Qt::WA_DeleteOnClose);
		setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
		//無標題覽
		setWindowFlags(windowFlags() | Qt::FramelessWindowHint);

		connect(parent, &CodeEditor::closeJumpToLineDialog, this, &JumpToLineDialog::close);

		// Set the width to 1/2 of the parent's width, but keep the height unchanged
		int dialogWidth = parent->width() / 2;
		int dialogHeight = height(); // Keep the height unchanged

		// Calculate the dialog position relative to the parent window
		QPoint parentCenter = parent->mapToGlobal(QPoint(parent->width() / 2, 0));
		int dialogX = parentCenter.x() - dialogWidth / 2;
		int dialogY = parentCenter.y();

		// Set the dialog's geometry
		setGeometry(dialogX, dialogY, dialogWidth, dialogHeight);

		// Move the dialog to the top and center of the parent
		move(dialogX, dialogY);


		//gray border
		setStyleSheet("QDialog{border: 1px solid gray;}");


		int currentLine, currentIndex;
		parent->getCursorPosition(&currentLine, &currentIndex);

		QLabel* label = new QLabel(tr("Current Line: %1 Index: %2").arg(currentLine + 1).arg(currentIndex), this);
		QLineEdit* lineEdit = new QLineEdit(this);
		lineEdit->setPlaceholderText(tr(":"));
		lineEdit->setValidator(new QIntValidator(1, parent->text().split("\n").size(), this));

		QHBoxLayout* layout = new QHBoxLayout;
		layout->addWidget(lineEdit);
		layout->addWidget(label);
		setLayout(layout);

		lineEdit->setFocus();
	}

protected:
	void keyPressEvent(QKeyEvent* e) override
	{
		if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
		{
			QLineEdit* lineEdit = findChild<QLineEdit*>();
			if (lineEdit)
			{
				if (m_lineNumber)
				{
					bool ok;
					qint64 line = lineEdit->text().toLongLong(&ok);
					if (ok)
					{
						*m_lineNumber = line;
						accept();
						return;
					}
				}
				reject();
				return;
			}
		}
		else if (e->key() == Qt::Key_Escape)
		{
			reject();
			return;
		}
		QDialog::keyPressEvent(e);
	}

private:
	int* m_lineNumber;
};


#endif // HIGHLIGHTER_H
