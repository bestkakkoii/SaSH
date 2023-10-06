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

using pfnProgressFunc = qint64(_cdecl*)(void* clientp, qint64 totalToDownload, qint64 nowDownloaded, qint64 totalToUpLoad, qint64 nowUpLoaded);

struct tNode
{
	FILE* fp = nullptr;
	qint64 startPos = 0U;
	qint64 endPos = 0U;
	qint64 totalSize = 0U;
	void* curl = nullptr;
	void* header = nullptr;
	qint64 id = 0;
	qint64 index = 0;
	QFuture<void> tid;
	pfnProgressFunc progressFunc = nullptr;
};

class CurlDownload
{
public:
	CurlDownload();

	virtual ~CurlDownload();

	inline void setProgressFunPtrs(const std::vector<pfnProgressFunc>& vpfnProgressFunc) { vpfnProgressFunc_ = vpfnProgressFunc; }


	bool downLoad(qint64 threadNum, std::string Url, std::string Path, std::string fileName);

	static QString oneShotDownload(const std::string szUrl);

private:
	qint64 getDownloadFileLenth(const char* url);

	static size_t writeFunc(void* clientp, size_t size, size_t nmemb, void* userdata);

	static void workThread(void* pData);

private:
	//static qint64 threadCnt_;
	qint64 index_ = 0;
	static std::atomic_int threadCnt_;
	static QMutex mutex_;
	std::vector<pfnProgressFunc> vpfnProgressFunc_;
};

#endif // CURLDOWNLOAD_H
