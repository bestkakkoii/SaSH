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

#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <Qsci/qscilexerlua.h>
#include <Qsci/qscilexercpp.h>
#include <QSettings>
#include <string>

class Highlighter : public QsciLexerLua
{
public:
	explicit Highlighter(QObject* parent = 0);

	virtual const char* keywords(int set) const override;

	virtual QColor defaultColor(int style) const override;

	virtual QFont defaultFont(int style) const override;

	virtual QStringList autoCompletionWordSeparators() const override;

	virtual QColor defaultPaper(int style) const override;

	virtual const char* autoCompletionFillups() const override;

	virtual const char* blockEnd(int* style) const override;

	virtual const char* blockStartKeyword(int* style) const override;

	virtual const char* blockStart(int* style) const override;

	virtual const char* wordCharacters() const override;

	// Returns the end-of-line fill for a style.
	virtual bool defaultEolFill(int style) const override;

private:
	QFont font_;
};
#endif // HIGHLIGHTER_H
