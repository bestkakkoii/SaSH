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

#ifndef CURLDOWNLOAD_H
#define CURLDOWNLOAD_H

#include <iostream>
#include <string>
#include <vector>
#include <QMutex>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>

struct tNode
{
	FILE* fp = nullptr;
	long startPos = 0U;
	long endPos = 0U;
	void* curl = nullptr;
	void* header = nullptr;
	int id = 0;
	int index = 0;
	QFuture<void> tid;
};


using pfnProgressFunc = int(_cdecl*)(void* clientp, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded);

class CurlDownload
{
public:
	CurlDownload();

	virtual ~CurlDownload() = default;

	inline void setProgressFunPtrs(const std::vector<pfnProgressFunc>& vpfnProgressFunc) { vpfnProgressFunc_ = vpfnProgressFunc; }

	inline void setIndex(int index) { index_ = index; }

	bool downLoad(int threadNum, std::string Url, std::string Path, std::string fileName);

	static QString oneShotDownload(const std::string szUrl);

private:
	long getDownloadFileLenth(const char* url);

	static size_t writeFunc(void* clientp, size_t size, size_t nmemb, void* userdata);

	static void workThread(void* pData);

private:
	//static int threadCnt_;
	int index_ = 0;
	static std::atomic_int threadCnt_;
	static QMutex mutex_;
	std::vector<pfnProgressFunc> vpfnProgressFunc_;
};

#endif // CURLDOWNLOAD_H
