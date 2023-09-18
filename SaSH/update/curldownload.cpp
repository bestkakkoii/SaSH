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

#include "stdafx.h"
#include "curldownload.h"

std::atomic_int CurlDownload::threadCnt_ = 0;
QMutex CurlDownload::mutex_;

CurlDownload::CurlDownload()
{
}

CurlDownload::~CurlDownload()
{
}

bool CurlDownload::downLoad(int threadNum, std::string Url, std::string Path, std::string fileName)
{
	if (!threadNum)
		return false;

	long fileLength = getDownloadFileLenth(Url.c_str());

	if (fileLength <= 0)
	{
		//printf("get the file length error...");
		return false;
	}

	// Create a file to save package.
	const std::string outFileName = Path + fileName;

	FILE* fp = nullptr;
	errno_t err = fopen_s(&fp, outFileName.c_str(), "wb");
	if (err || fp == nullptr)
	{
		return false;
	}

	long partSize = fileLength / static_cast<long>(threadNum);

	for (size_t i = 0; i <= static_cast<size_t>(threadNum); ++i)
	{
		tNode* pNode = q_check_ptr(new tNode());

		if (i < static_cast<size_t>(threadNum))
		{
			pNode->startPos = i * partSize;
			pNode->endPos = (i + 1) * partSize - 1;
		}
		else
		{
			if (fileLength % static_cast<long>(threadNum) != 0L)
			{
				pNode->startPos = i * partSize;
				pNode->endPos = fileLength - 1;
			}
			else
				break;
		}

		CURL* curl = curl_easy_init();

		pNode->curl = curl;
		pNode->fp = fp;

		char range[64] = { 0 };
		memset(range, 0, sizeof(range));
		snprintf(range, sizeof(range), "%ld-%ld", pNode->startPos, pNode->endPos);

		struct curl_slist* header_list = NULL;
		QString qheader = QString("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/107.0.0.0 Safari/537.36 Edg/107.0.1418.35 SaSH/download-update-file");
		std::string agent(qheader.toUtf8().constData());
		header_list = curl_slist_append(header_list, "authority: www.lovesa.cc");
		header_list = curl_slist_append(header_list, "Method: GET");
		header_list = curl_slist_append(header_list, "Scheme: https");
		header_list = curl_slist_append(header_list, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9");
		header_list = curl_slist_append(header_list, "Accept-Encoding: *");
		header_list = curl_slist_append(header_list, "accept-language: zh-TW,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6,zh-CN;q=0.5");
		header_list = curl_slist_append(header_list, "Cache-Control: max-age=0");
		header_list = curl_slist_append(header_list, "DNT: 1");
		header_list = curl_slist_append(header_list, R"(Sec-CH-UA: "Microsoft Edge";v="107", "Chromium";v="107", "Not=A?Brand";v="24")");
		header_list = curl_slist_append(header_list, "Sec-CH-UA-Mobile: ?0");
		header_list = curl_slist_append(header_list, R"(Sec-CH-UA-Platform: "Windows")");
		header_list = curl_slist_append(header_list, "Sec-Fetch-Dest: document");
		header_list = curl_slist_append(header_list, "Sec-Fetch-Mode: navigate");
		header_list = curl_slist_append(header_list, "Sec-Fetch-Site: none");
		header_list = curl_slist_append(header_list, "Sec-Fetch-User: ?1");
		header_list = curl_slist_append(header_list, "Upgrade-Insecure-Requests: 1");
		header_list = curl_slist_append(header_list, agent.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
		pNode->header = header_list;

		// Download pacakge
		curl_easy_setopt(curl, CURLOPT_URL, Url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunc);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)pNode);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_HEADER, 0);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

		if (vpfnProgressFunc_.size() > i)
		{
			curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, vpfnProgressFunc_[i]);
			pNode->id = index_;
			pNode->index = i;
			curl_easy_setopt(curl, CURLOPT_XFERINFODATA, pNode);
		}

		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		//curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
		//curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 5L);
		curl_easy_setopt(curl, CURLOPT_RANGE, range);

		++threadCnt_;
		pNode->tid = QtConcurrent::run(workThread, pNode);
	}

	for (;;)
	{
		QThread::msleep(1000ll);
		if (threadCnt_ <= 0)
			break;
	}

	fclose(fp);

	//printf ("download succed......\n");
	return true;
}

size_t CurlDownload::writeFunc(void* ptr, size_t size, size_t nmemb, void* userdata)
{
	tNode* node = reinterpret_cast<tNode*>(userdata);
	size_t written = 0;
	QMutexLocker locker(&mutex_);

	if (static_cast<size_t>(node->startPos) + size * nmemb <= static_cast<size_t>(node->endPos))
	{
		fseek(node->fp, node->startPos, SEEK_SET);
		written = fwrite(ptr, size, nmemb, node->fp);
		node->startPos += size * nmemb;
	}
	else
	{
		fseek(node->fp, node->startPos, SEEK_SET);
		written = fwrite(ptr, 1, static_cast<size_t>(node->endPos) - static_cast<size_t>(node->startPos) + 1, node->fp);
		node->startPos = node->endPos;
	}

	return written;
}

long CurlDownload::getDownloadFileLenth(const char* url)
{
	qreal downloadFileLenth = 0;
	CURL* handle = curl_easy_init();

	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_HEADER, 1);
	curl_easy_setopt(handle, CURLOPT_NOBODY, 1);
	curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(handle, CURLOPT_TIMEOUT, 10);
	curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
	if (curl_easy_perform(handle) == CURLE_OK)
	{
		curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &downloadFileLenth);
	}
	else
	{
		downloadFileLenth = -1;
	}
	curl_easy_cleanup(handle);
	return downloadFileLenth;
}

void CurlDownload::workThread(void* pData)
{
	tNode* pNode = reinterpret_cast<tNode*>(pData);

	int res = curl_easy_perform(pNode->curl);

	if (res != 0)
	{

	}
	curl_slist_free_all(reinterpret_cast<curl_slist*>(pNode->header));
	curl_easy_cleanup(pNode->curl);

	--threadCnt_;
	//printf ("thred %ld exit\n", pNode->tid);
	delete pNode;
	//pthread_exit (nullptr);
}

static size_t write_data(char* ptr, size_t size, size_t nmemb, void* userdata)
{
	std::string* pstr = reinterpret_cast<std::string*>(userdata);
	pstr->append(ptr, size * nmemb);
	return size * nmemb;
};

//一次性獲取小文件內容
QString CurlDownload::oneShotDownload(const std::string szUrl)
{
	CURL* curl = curl_easy_init();
	if (curl == nullptr)
	{
		return "";
	}

	struct curl_slist* header_list = NULL;
	QString qheader = QString("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/107.0.0.0 Safari/537.36 Edg/107.0.1418.35 SaSH/download-small-file");
	std::string agent(qheader.toUtf8().constData());
	header_list = curl_slist_append(header_list, "authority: www.lovesa.cc");
	header_list = curl_slist_append(header_list, "Method: GET");
	header_list = curl_slist_append(header_list, "Scheme: https");
	header_list = curl_slist_append(header_list, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9");
	header_list = curl_slist_append(header_list, "Accept-Encoding: *");
	header_list = curl_slist_append(header_list, "accept-language: zh-TW,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6,zh-CN;q=0.5");
	header_list = curl_slist_append(header_list, "Cache-Control: max-age=0");
	header_list = curl_slist_append(header_list, "DNT: 1");
	header_list = curl_slist_append(header_list, R"(Sec-CH-UA: "Microsoft Edge";v="107", "Chromium";v="107", "Not=A?Brand";v="24")");
	header_list = curl_slist_append(header_list, "Sec-CH-UA-Mobile: ?0");
	header_list = curl_slist_append(header_list, R"(Sec-CH-UA-Platform: "Windows")");
	header_list = curl_slist_append(header_list, "Sec-Fetch-Dest: document");
	header_list = curl_slist_append(header_list, "Sec-Fetch-Mode: navigate");
	header_list = curl_slist_append(header_list, "Sec-Fetch-Site: none");
	header_list = curl_slist_append(header_list, "Sec-Fetch-User: ?1");
	header_list = curl_slist_append(header_list, "Upgrade-Insecure-Requests: 1");
	header_list = curl_slist_append(header_list, agent.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);

	//同步下載
	curl_easy_setopt(curl, CURLOPT_URL, szUrl.c_str());
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_HEADER, 0);
	curl_easy_setopt(curl, CURLOPT_NOBODY, 0);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	std::string str;
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &str);
	curl_easy_perform(curl);
	curl_slist_free_all(header_list);
	curl_easy_cleanup(curl);

	return QString::fromUtf8(str.c_str());
}
