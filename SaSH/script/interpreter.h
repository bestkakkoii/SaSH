/*
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
#include <QObject>
#include <atomic>
#include "parser.h"
#include "util.h"

constexpr long long DEFAULT_FUNCTION_TIMEOUT = 5000;

class Interpreter : public QObject, public Indexer
{
	Q_OBJECT
public:
	enum VarShareMode
	{
		kNotShare,
		kShare,
	};

public:
	explicit Interpreter(long long index);
	virtual ~Interpreter();

	bool __fastcall isRunning() const;

	void __fastcall preview(const QString& fileName);

	void __fastcall doString(QString content);

	void __fastcall doFileWithThread(long long beginLine, const QString& fileName);

	bool __fastcall doFile(long long beginLine, const QString& fileName, Interpreter* pinterpretter, Parser* pparser, bool issub, Parser::Mode mode);

signals:
	void finished();

public slots:
	void proc();
	void onRunString();

private:
	long long scriptCallBack(long long currentIndex, long long currentLine, const TokenMap& token);

private:

	template<typename Func>
	void __fastcall registerFunction(const QString functionName, Func fun);
	void __fastcall openLibs();

private:
	bool __fastcall checkBattleThenWait();
	bool __fastcall checkOnlineThenWait();

	bool __fastcall waitfor(long long timeout, std::function<bool()> exprfun);
	bool __fastcall checkString(const TokenMap& TK, long long idx, QString* ret);
	bool __fastcall checkInteger(const TokenMap& TK, long long idx, long long* ret);

	long long __fastcall checkJump(const TokenMap& TK, long long idx, bool expr, JumpBehavior behavior);
	bool __fastcall checkRange(const TokenMap& TK, long long idx, long long* min, long long* max);
	bool __fastcall checkRelationalOperator(const TokenMap& TK, long long idx, RESERVE* ret) const;

	void __fastcall logExport(long long currentIndex, long long currentLine, const QString& text, long long color = 0);

	void __fastcall errorExport(long long currentIndex, long long currentLine, long long level, const QString& text);

	void __fastcall setParentParser(Parser* pParser) { pParentParser_ = pParser; }
	void __fastcall setParentInterpreter(Interpreter* pInterpreter) { pParentInterpreter_ = pInterpreter; }

private: //註冊給Parser的函數
	//system
	long long run(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long dostr(long long currentIndex, long long currentLine, const TokenMap& TK);

	long long usemagic(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long mail(long long currentIndex, long long currentLine, const TokenMap& TK);

	//action
	long long useitem(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long dropitem(long long currentIndex, long long currentLine, const TokenMap& TK);

	long long trade(long long currentIndex, long long currentLine, const TokenMap& TK);

	long long recordequip(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long wearequip(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long unwearequip(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long petequip(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long petunequip(long long currentIndex, long long currentLine, const TokenMap& TK);

	long long depositpet(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long withdrawpet(long long currentIndex, long long currentLine, const TokenMap& TK);

private:
	enum
	{
		WARN_LEVEL = 0,
		ERROR_LEVEL = 1,
	};

	Parser* pParentParser_ = nullptr;
	Interpreter* pParentInterpreter_ = nullptr;

	QThread thread_;
	Parser parser_;
	long long beginLine_ = 0;

	safe::Flag isRunning_ = false;
	ParserCallBack pCallback = nullptr;
	QList<std::shared_ptr<Interpreter>> subInterpreterList_;
	QFutureSynchronizer<bool> subThreadFutureSync_;
};