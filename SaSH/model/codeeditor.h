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

//#ifndef QSCINTILLA_DLL
//#define QSCINTILLA_DLL
//#endif
//
//#ifndef QSCINTILLA_MAKE_DLL
//#define QSCINTILLA_MAKE_DLL
//#endif

#include <Qsci/qsciscintilla.h>
#include "QSci/qsciapis.h"
#include <QStyleOption>
#include <QPainter>
#include <highlighter.h>
#include <QDialog>
#include <QHash>
#include <indexer.h>
#include "form/findandreplaceform.h"

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

	enum MackerMask
	{
		S_BREAK = 0b0001,//1
		S_ARROW = 0b0010,//2
		S_ERRORMARK = 0b0100,//4
		S_STEPMARK = 0b1000,//8
	};

	explicit CodeEditor(QWidget* parent = nullptr);

	inline bool isBreak(long long mode) const { return ((mode & S_BREAK) == S_BREAK); }
	inline bool isArrow(long long mode) const { return ((mode & S_ARROW) == S_ARROW); }
	inline bool isError(long long mode) const { return ((mode & S_ERRORMARK) == S_ERRORMARK); }
	inline bool isStep(long long mode) const { return ((mode & S_STEPMARK) == S_STEPMARK); }

	QFont getOldFont() { return QsciScintilla::font(); }
	void setNewFont(const QFont& f) { font_ = f; setFont(f); textLexer_->setDefaultFont(f); }

public slots:
	void commentSwitch();

private slots:
	void onFindFirst(const QString& expr, bool re, bool cs, bool wo, bool wrap, bool forward = true, int line = -1, int index = -1, bool show = true, bool posix = false);
	void onFindNext();
	void onReplaceAll(const QString& text, const QString& expr, bool re, bool cs, bool wo, bool wrap, bool forward = true, int line = -1, int index = -1, bool show = true, bool posix = false);
	void onFindAll(const QString& expr, bool re, bool cs, bool wo, bool wrap, bool forward = true, int line = -1, int index = -1, bool show = true, bool posix = false);

signals:
	void closeJumpToLineDialog();
	void findAllFinished(const QString& expr, const QVariant& varmap);

private:
	Highlighter* textLexer_ = nullptr;
	QsciAPIs* apis_ = nullptr;
	QFont font_;
	QFont linefont_;
	bool isDialogOpened_ = false;

	FindAndReplaceForm findAndReplaceForm_;

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
		if (isDialogOpened_)
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
	JumpToLineDialog(CodeEditor* parent = nullptr, long long* lineNumber = nullptr)
		: QDialog(parent), m_lineNumber(lineNumber)
	{
		setAttribute(Qt::WA_DeleteOnClose);
		setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
		//無標題覽
		setWindowFlags(windowFlags() | Qt::FramelessWindowHint);

		connect(parent, &CodeEditor::closeJumpToLineDialog, this, &JumpToLineDialog::close);

		// Set the width to 1/2 of the parent's width, but keep the height unchanged
		long long dialogWidth = parent->width() / 2;
		long long dialogHeight = height(); // Keep the height unchanged

		// Calculate the dialog position relative to the parent window
		QPoint parentCenter = parent->mapToGlobal(QPoint(parent->width() / 2, 0));
		long long dialogX = parentCenter.x() - dialogWidth / 2;
		long long dialogY = parentCenter.y();

		// Set the dialog's geometry
		setGeometry(dialogX, dialogY, dialogWidth, dialogHeight);

		// Move the dialog to the top and center of the parent
		move(dialogX, dialogY);


		//gray border
		setStyleSheet("QDialog{border: 1px solid gray;}");


		int currentLine, currentIndex;
		parent->getCursorPosition(&currentLine, &currentIndex);

		QLabel* label = q_check_ptr(new QLabel(tr("Current Line: %1 Index: %2").arg(currentLine + 1).arg(currentIndex), this));
		sash_assume(label != nullptr);

		QLineEdit* lineEdit = q_check_ptr(new QLineEdit(this));
		sash_assume(lineEdit != nullptr);

		lineEdit->setPlaceholderText(tr(":"));
		lineEdit->setValidator(q_check_ptr(new QIntValidator(1, parent->text().split("\n").size(), this)));

		QHBoxLayout* layout = q_check_ptr(new QHBoxLayout);
		sash_assume(layout != nullptr);

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
					long long line = lineEdit->text().toLongLong(&ok);
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
	long long* m_lineNumber;
};


#endif // HIGHLIGHTER_H
