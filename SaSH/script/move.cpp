#include "stdafx.h"
#include "interpreter.h"
#include "util.h"
#include "injector.h"

//move
int Interpreter::setdir(const TokenMap& TK)
{
	static const QHash<QString, QString> dirhash = {
		{ "北", "A"}, { "東北", "B"}, { "東", "C"}, { "東南", "D"},
		{ "南", "E"}, { "西南", "F"}, { "西", "G"}, { "西北", "H"},
		{ "北", "A"}, { "东北", "B"}, { "东", "C"}, { "东南", "D"},
		{ "南", "E"}, { "西南", "F"}, { "西", "G"}, { "西北", "H"}
	};

	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	Injector& injector = Injector::getInstance();


}

int Interpreter::move(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	Injector& injector = Injector::getInstance();

}

int Interpreter::fastmove(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	Injector& injector = Injector::getInstance();
}

int Interpreter::packetmove(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	Injector& injector = Injector::getInstance();
}

int Interpreter::findpath(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	Injector& injector = Injector::getInstance();
}

int Interpreter::movetonpc(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	Injector& injector = Injector::getInstance();
}

int Interpreter::findnpc(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	Injector& injector = Injector::getInstance();
}
