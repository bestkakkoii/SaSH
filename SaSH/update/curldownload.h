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


using pProgressFunc = int(_cdecl*)(void* clientp, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded);

class CurlDownload
{
public:
	CurlDownload();


	void SetProgressFunPtrs(const std::vector<pProgressFunc>& vpProgressFunc) { m_vpProgressFunc = vpProgressFunc; }

	void setIndex(int index) { m_index = index; }
	bool DownLoad(int threadNum, std::string Url, std::string Path, std::string fileName);

	static QString OneShotDownload(const std::string szUrl);

private:
	long getDownloadFileLenth(const char* url);
	static size_t writeFunc(void* clientp, size_t size, size_t nmemb, void* userdata);
	static void workThread(void* pData);
private:
	//static int threadCnt;
	int m_index = 0;
	static std::atomic_int threadCnt;
	static QMutex g_mutex;
	std::vector<pProgressFunc> m_vpProgressFunc;
};

#endif // CURLDOWNLOAD_H
