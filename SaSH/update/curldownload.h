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
