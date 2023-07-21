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
	Highlighter(QObject* parent = 0);

	enum
	{
		//! The default.
		InactiveDefault = Default + 64,

		//! A C comment.

		InactiveComment = Comment + 64,

		//! A C++ comment line.

		InactiveCommentLine = LineComment + 64,

		//! A JavaDoc/Doxygen style C comment.
		CommentDoc = 3,
		InactiveCommentDoc = CommentDoc + 64,

		//! A number.

		InactiveNumber = Number + 64,

		//! A keyword.

		InactiveKeyword = Keyword + 64,

		//! A double-quoted string.

		InactiveDoubleQuotedString = String + 64,

		//! A single-quoted string.
		InactiveSingleQuotedString = Character + 64,

		//! An IDL UUID.
		InactiveUUID = LiteralString + 64,

		//! A pre-processor block.
		InactivePreProcessor = Preprocessor + 64,

		//! An operator.
		InactiveOperator = Operator + 64,

		//! An identifier
		InactiveIdentifier = Identifier + 64,

		//! The end of a line where a string is not closed.
		InactiveUnclosedString = UnclosedString + 64,

		//! A C# verbatim string.
		InactiveVerbatimString = BasicFunctions + 64,

		//! A JavaScript regular expression.
		InactiveRegex = StringTableMathsFunctions + 64,

		//! A JavaDoc/Doxygen style C++ comment line.
		InactiveCommentLineDoc = CoroutinesIOSystemFacilities + 64,

		//! A keyword defined in keyword set number 2.  The class must be
		//! sub-classed and re-implement keywords() to make use of this style.

		InactiveKeywordSet2 = KeywordSet5 + 64,

		//! A JavaDoc/Doxygen keyword.
		InactiveCommentDocKeyword = KeywordSet6 + 64,

		//! A JavaDoc/Doxygen keyword error.
		InactiveCommentDocKeywordError = KeywordSet7 + 64,

		//! A global class or typedef defined in keyword set number 5.  The
		//! class must be sub-classed and re-implement keywords() to make use
		//! of this style.
		InactiveGlobalClass = KeywordSet8 + 64,

		//! A C++ raw string.
		InactiveRawString = Label + 64,

		//! A Vala triple-quoted verbatim string.
		TripleQuotedVerbatimString = 21,
		InactiveTripleQuotedVerbatimString = TripleQuotedVerbatimString + 64,

		//! A Pike hash-quoted string.
		HashQuotedString = 22,
		InactiveHashQuotedString = HashQuotedString + 64,

		//! A pre-processor stream comment.
		PreProcessorComment = 23,
		InactivePreProcessorComment = PreProcessorComment + 64,

		//! A JavaDoc/Doxygen style pre-processor comment.
		PreProcessorCommentLineDoc = 24,
		InactivePreProcessorCommentLineDoc = PreProcessorCommentLineDoc + 64,

		//! A user-defined literal.
		UserLiteral = 25,
		InactiveUserLiteral = UserLiteral + 64,

		//! A task marker.
		TaskMarker = 26,
		InactiveTaskMarker = TaskMarker + 64,

		//! An escape sequence.
		EscapeSequence = 27,
		InactiveEscapeSequence = EscapeSequence + 64,
	};

	const char* keywords(int set) const;

	QColor defaultColor(int style) const;

	QFont defaultFont(int style) const;

	QStringList autoCompletionWordSeparators() const;

	QColor defaultPaper(int style) const;

	const char* autoCompletionFillups() const;

	const char* blockEnd(int* style) const;

	const char* blockStartKeyword(int* style) const;

	const char* blockStart(int* style) const;

	const char* wordCharacters() const;

	// Returns the end-of-line fill for a style.
	bool defaultEolFill(int style) const;

private:
	QFont m_font;
};
#endif // HIGHLIGHTER_H
