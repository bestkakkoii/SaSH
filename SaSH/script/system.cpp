#include "stdafx.h"
#include "interpreter.h"
#include "util.h"
#include "injector.h"


int Interpreter::sleep(const TokenMap& TK)
{
	int t;
	if (!checkInt(TK, 1, &t))
		return Parser::kError;

	if (t >= 1000u)
	{
		int i = 0u;
		for (; i < t / 1000u; ++i)
		{
			QThread::msleep(1000UL);
			QThread::yieldCurrentThread();
			if (isInterruptionRequested())
				break;
		}
		QThread::msleep(static_cast<DWORD>(i) % 1000UL);
	}
	else
		QThread::msleep(static_cast<DWORD>(t));

	return Parser::kNoChange;
}

int Interpreter::press(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString text;
	checkString(TK, 1, &text);

	int row = 0;
	checkInt(TK, 1, &row);

	if (text.isEmpty() && row == 0)
		return Parser::kError;

	if (!text.isEmpty())
	{
		BUTTON_TYPE button = buttonMap.value(text);
		injector.server->press(button);
	}
	else if (row > 0)
		injector.server->press(row);

	return Parser::kNoChange;
}

int Interpreter::eo(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull())
		injector.server->EO();

	return Parser::kNoChange;
}

int Interpreter::announce(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString text;
	if (!checkString(TK, 1, &text))
		return Parser::kError;

	int color = 4;
	checkInt(TK, 2, &color);

	injector.server->announce(text, color);
	return Parser::kNoChange;
}

int Interpreter::talk(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString text;
	if (!checkString(TK, 1, &text))
		return Parser::kError;

	int color = 4;
	checkInt(TK, 2, &color);

	injector.server->talk(text, color);

	return Parser::kNoChange;
}

int Interpreter::talkandannounce(const TokenMap& TK)
{
	announce(TK);
	return talk(TK);
}

int Interpreter::logout(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull())
		injector.server->logOut();

	return Parser::kNoChange;
}

int Interpreter::logback(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull())
		injector.server->logBack();

	return Parser::kNoChange;
}

int Interpreter::cleanchat(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull())
		injector.server->cleanChatHistory();

	return Parser::kNoChange;
}