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

#pragma once
#include "lexer.h"
#include <QStack>
#include <functional>

#include "threadplugin.h"
#include "util.h"
#include "script_lua/clua.h"

using Parser2CallBack = std::function<qint64(qint64 currentLine, const TokenMap& token)>;

class Interpreter;
class Parser2
{
public:
	Parser2();
	virtual ~Parser2();

	inline Q_REQUIRED_RESULT qint64 getBeginLine() const { return beginLine_; }
	inline Q_REQUIRED_RESULT QString getScriptFileName() const { return scriptFileName_; }
	inline Q_REQUIRED_RESULT bool isPrivate() const { return isPrivate_; }
	inline Q_REQUIRED_RESULT QHash<qint64, TokenMap> getTokens() const { return tokens_; }
	inline Q_REQUIRED_RESULT QHash<QString, qint64> getLabels() const { return labels_; }
	inline Q_REQUIRED_RESULT QList<FunctionNode> getFunctionNodeList() const { return functionNodeList_; }
	inline Q_REQUIRED_RESULT QList<ForNode> getForNodeList() const { return forNodeList_; }
	inline void setScriptFileName(const QString& scriptFileName) { scriptFileName_ = scriptFileName; }
	inline void setBeginLine(const qint64 beginLine) { beginLine_ = beginLine; }
	inline void setPrivate(bool isPrivate) { isPrivate_ = isPrivate; }
	inline void setCallBack(Parser2CallBack callBack) { callBack_ = callBack; }

	bool loadFile(const QString& fileName, QString* pcontent);

	bool loadString(const QString& content);

	void parse(Interpreter* pParentInterpreter = nullptr);

private:
	void processTokens();

private:
	Lexer lexer_;
	sol::state lua_;

	Interpreter* pParentInterpreter_ = nullptr;

	Parser2CallBack callBack_;

	qint64 beginLine_ = 0;
	QString scriptFileName_;
	bool isPrivate_ = false;
	QHash<qint64, TokenMap> tokens_;
	QHash<QString, qint64> labels_;
	QList<FunctionNode> functionNodeList_;
	QList<ForNode> forNodeList_;
};