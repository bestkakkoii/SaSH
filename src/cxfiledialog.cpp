/*
                GNU GENERAL PUBLIC LICENSE
                   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2024 All Rights Reserved.
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
#include <cxfiledialog.h>

#if 0
cx::file_dialog::file_dialog(HWND parent, WindowFlags flags)
    : parent_(parent)
    , flags_(flags)
{
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr))
    {
        std::wcerr << L"Failed to initialize COM library" << std::endl;
    }
}
#endif

cx::file_dialog::file_dialog(FileDialogAcceptMode mode, HWND parent, const QString& caption, const QString& directory)
    : parent_(parent)
    , directory_(directory)
    , caption_(caption)
    , fileMode_(mode)
{
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr))
    {
        std::wcerr << L"Failed to initialize COM library" << std::endl;
        return;
    }

    if (fileMode_ != FileDialogAcceptMode::OpenDirectory)
    {
        nameFilters_.append("All Files|*.*");
    }

    create();
}

cx::file_dialog::file_dialog(qint64 mode, HWND parent = nullptr)
    : parent_(parent)
    , directory_("")
    , caption_("")
    , fileMode_(static_cast<FileDialogAcceptMode>(mode))
{
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr))
    {
        std::wcerr << L"Failed to initialize COM library" << std::endl;
        return;
    }

    if (fileMode_ != FileDialogAcceptMode::OpenDirectory)
    {
        nameFilters_.append("All Files|*.*");
    }

    create();
}


void cx::file_dialog::setWindowTitle(const QString& title)
{
    if (nullptr == pDialog_)
    {
        std::wcerr << L"dialog pointer is null" << std::endl;
        return;
    }

    if (title.size() <= 0)
    {
        return;
    }

    pDialog_->SetTitle(title.toStdWString().c_str());
}


#if 0
void cx::file_dialog::setIcon(const QString& iconPath)
{
    if (iconPath.empty())
    {
        std::wcerr << L"icon path is empty" << std::endl;
        return;
    }

    if (icon_ != nullptr)
    {
        DestroyIcon(icon_);
        icon_ = nullptr;
    }

    icon_ = LoadIconW(nullptr, iconPath.toStdWString().c_str());
}
#endif

cx::file_dialog::~file_dialog()
{
    clear();
}

void cx::file_dialog::clear()
{
    if (pOpenDialog_ != nullptr)
    {
        pOpenDialog_->Release();
        pOpenDialog_ = nullptr;
    }

    if (pSaveDialog_ != nullptr)
    {
        pSaveDialog_->Release();
        pSaveDialog_ = nullptr;
    }

    if (pDialog_ != nullptr)
        pDialog_ = nullptr;

#if 0
    if (icon_ != nullptr)
    {
        DestroyIcon(icon_);
        icon_ = nullptr;
    }
#endif

    //uninstall the COM library
    CoUninitialize();
}

void cx::file_dialog::setDefaultFileName(const QString& filename)
{
    if (pSaveDialog_ = nullptr)
    {
        std::wcerr << L"dialog pointer is null" << std::endl;
        return;
    }

    pSaveDialog_->SetFileName(filename.toStdWString().c_str());
}

void cx::file_dialog::setLabelText(FileDialogLabel label, const QString& text)
{
    if (nullptr == pDialog_)
    {
        std::wcerr << L"dialog pointer is null" << std::endl;
        return;
    }

    switch (label)
    {
    case FileDialogLabel::OkButton:
        pDialog_->SetOkButtonLabel(text.toStdWString().c_str());
        break;
    case FileDialogLabel::FileName:
        pDialog_->SetFileNameLabel(text.toStdWString().c_str());
        break;
    default:
        std::wcerr << L"invalid label type" << std::endl;
        break;
    }
}

void cx::file_dialog::setDefaultFolder(const QString& folder)
{
    if (nullptr == pDialog_)
    {
        std::wcerr << L"dialog pointer is null" << std::endl;
        return;
    }

    IShellItem* pFolder = nullptr;
    HRESULT hr = SHCreateItemFromParsingName(folder.toStdWString().c_str(), nullptr, IID_PPV_ARGS(&pFolder));
    if (FAILED(hr))
    {
        std::wcerr << L"Failed to create IShellItem from folder path" << std::endl;
        return;
    }

    pDialog_->SetFolder(pFolder);
    pFolder->Release();
}

HRESULT cx::file_dialog::setNameFilters()
{
    if (nullptr == pDialog_)
    {
        std::wcerr << L"dialog pointer is null" << std::endl;
        return E_INVALIDARG;
    }

    if (nameFilters_.empty())
    {
        std::wcerr << L"filter is empty" << std::endl;
        return E_INVALIDARG;
    }

    if (fileMode_ == FileDialogAcceptMode::OpenDirectory)
    {
        return S_OK;
    }

    QVector<QPair<std::wstring, std::wstring>> rgSpecVec;
    qint64 size = nameFilters_.size();
    QString name;
    QString spec;
    QString temp;

    for (qint64 i = 0; i < size; ++i)
    {
        temp = nameFilters_.value(i);
        if (temp.size() <= 0)
        {
            continue;
        }

        QStringList list = temp.split("|");

        if (list.size() != 2)
        {
            continue;
        }

        name = list.value(0);
        if (name.size() <= 0)
        {
            continue;
        }

        spec = list.value(1);
        if (spec.size() <= 0)
        {
            continue;
        }

        rgSpecVec.append(QPair<std::wstring, std::wstring>{ name.toStdWString(), spec.toStdWString() });
    }

    if (rgSpecVec.size() <= 0)
    {
        qCritical().noquote() << L"none of the filter is valid";
        return E_INVALIDARG;
    }

    size = rgSpecVec.size();

    std::unique_ptr<COMDLG_FILTERSPEC[]> prgSpec(new COMDLG_FILTERSPEC[size]());
    for (qint64 i = 0; i < size; ++i)
    {
        prgSpec[i].pszName = rgSpecVec[i].first.c_str();
        prgSpec[i].pszSpec = rgSpecVec[i].second.c_str();
    }

    // 設置檔案類型過濾器
    HRESULT hr = pDialog_->SetFileTypes(static_cast<UINT>(size), prgSpec.get());
    if (SUCCEEDED(hr))
    {
        hr = pDialog_->SetFileTypeIndex(1);
    }

    return hr;
}

void cx::file_dialog::setSaveAsItem(const QString& filePath)
{
    if (nullptr == pSaveDialog_)
    {
        std::wcerr << L"SaveDialog pointer is null" << std::endl;
        return;
    }

    IShellItem* psi = nullptr;
    HRESULT hr = SHCreateItemFromParsingName(filePath.toStdWString().c_str(), nullptr, IID_PPV_ARGS(&psi));
    if (FAILED(hr))
    {
        std::wcerr << L"Failed to create IShellItem from file path" << std::endl;
        return;
    }

    hr = pSaveDialog_->SetSaveAsItem(psi);
    psi->Release();
}

void cx::file_dialog::setDefaultExtension(const QString& extension)
{
    if (nullptr == pDialog_)
    {
        std::wcerr << L"dialog pointer is null" << std::endl;
        return;
    }

    pDialog_->SetDefaultExtension(extension.toStdWString().c_str());
}

void cx::file_dialog::setOptions(FileDialogOptions options)
{
    if (nullptr == pDialog_)
    {
        std::wcerr << L"dialog pointer is null" << std::endl;
        return;
    }

    DWORD dwOptions;
    HRESULT hr = pDialog_->GetOptions(&dwOptions);
    if (FAILED(hr))
    {
        std::wcerr << L"Failed to get options" << std::endl;
        return;
    }

    if (dwOptions == static_cast<DWORD>(options))
        return;

    hr = pDialog_->SetOptions(static_cast<FILEOPENDIALOGOPTIONS>(options_));
    if (FAILED(hr))
    {
        std::wcerr << L"Failed to set options" << std::endl;
    }

    if (options_ != options)
        options_ = options;
}

void cx::file_dialog::setOption(FileDialogOption option, bool on)
{
    if (nullptr == pDialog_)
    {
        std::wcerr << L"dialog pointer is null" << std::endl;
        return;
    }

    //get old options
    DWORD dwOptions;
    HRESULT hr = pDialog_->GetOptions(&dwOptions);
    if (FAILED(hr))
    {
        std::wcerr << L"Failed to get options" << std::endl;
        return;
    }

    bool b = (dwOptions & static_cast<DWORD>(option)) == static_cast<DWORD>(option);

    //set new options
    if (on)
    {
        if (b)
            return;
        dwOptions |= static_cast<DWORD>(option);
    }
    else
    {
        if (!b)
            return;
        dwOptions &= ~static_cast<DWORD>(option);
    }

    hr = pDialog_->SetOptions(dwOptions);
    if (FAILED(hr))
    {
        std::wcerr << L"Failed to set options" << std::endl;
    }

    if ((DWORD)options_ != dwOptions)
        options_ = static_cast<FileDialogOptions>(dwOptions);
}

HRESULT cx::file_dialog::handleFilesOrDirectoriesSelection()
{
    if (nullptr == pOpenDialog_)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    do
    {
        // Retrieve the selected file(s)
        IShellItemArray* pResults = nullptr;
        hr = pOpenDialog_->GetResults(&pResults);
        if (FAILED(hr))
        {
            break;
        }

        DWORD itemCount = 0;
        hr = pResults->GetCount(&itemCount);
        if (FAILED(hr) || itemCount == 0)
        {
            pResults->Release();
            break;
        }

        // Process the selected file(s)
        for (DWORD i = 0; i < itemCount; ++i)
        {
            IShellItem* pItem = nullptr;
            hr = pResults->GetItemAt(i, &pItem);
            if (FAILED(hr))
            {
                continue;
            }

            PWSTR pszFilePath = nullptr;
            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
            if (SUCCEEDED(hr) && pszFilePath != nullptr)
            {
                QString path = QString::fromWCharArray(pszFilePath);
                path.replace("\\", "/");
                if (!path.endsWith("/"))
                {
                    path.append("/");
                }

                selectedFiles_.append(path);

                // Don't forget to free the allocated memory for pszFilePath
                CoTaskMemFree(pszFilePath);
            }
            pItem->Release();
        }

        pResults->Release();
    } while (false);

    return hr;
}

HRESULT cx::file_dialog::handleSaveFileSelection()
{
    if (nullptr == pSaveDialog_)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    do
    {
        // Retrieve the selected file to save
        IShellItem* pResult = nullptr;
        hr = pSaveDialog_->GetResult(&pResult);
        if (FAILED(hr))
        {
            break;
        }

        PWSTR pszFilePath = nullptr;
        hr = pResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
        if (SUCCEEDED(hr) && pszFilePath != nullptr)
        {
            QString path = QString::fromWCharArray(pszFilePath);
            path.replace("\\", "/");
            if (!path.endsWith("/"))
            {
                path.append("/");
            }

            selectedFiles_.append(path);

            // Don't forget to free the allocated memory for pszFilePath
            CoTaskMemFree(pszFilePath);

            // Release the IShellItem
            pResult->Release();
        }

    } while (false);

    return hr;
}

HRESULT cx::file_dialog::create()
{
    HRESULT hr = S_OK;
    //Create instance of IFileOpenDialog or IFileSaveDialog
    switch (fileMode_)
    {
    case FileDialogAcceptMode::OpenFile:
    {
        hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pOpenDialog_));
        setDialog(pOpenDialog_);
        setOptions(FileDialogOption::DefaultOpenFile);
        break;
    }
    case FileDialogAcceptMode::OpenDirectory:
    {
        hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pOpenDialog_));
        setDialog(pOpenDialog_);
        setOptions(FileDialogOption::DefaultOpenDirectory);
        break;
    }
    case FileDialogAcceptMode::SaveFile:
    {
        hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pSaveDialog_));
        setDialog(pSaveDialog_);
        setOptions(FileDialogOption::DefaultSaveFile);
        break;
    }
    default:
        hr = E_FAIL; // 不支持的模式
        break;
    }

    return hr;
}

bool cx::file_dialog::exec()
{
    HRESULT hr = S_OK;
    do
    {
        if (nullptr == pDialog_)
        {
            hr = create();
            if (FAILED(hr))
            {
                break;
            }
        }

#if 0 //暫時沒有精準獲取HWND的方法
        HWND hWnd_ = getWindowHandle();
        if (hWnd_ != nullptr)
        {
            // 設置窗口圖標
            if (icon_ != nullptr)
            {

                // 設置圖標
                SendMessageW(hWnd_, WM_SETICON, ICON_BIG, (LPARAM)icon_);
                SendMessageW(hWnd_, WM_SETICON, ICON_SMALL, (LPARAM)icon_);
            }
        }
#endif

        //設置 Options
        setOptions(options_);

        // 設置文件過濾器
        hr = setNameFilters();
        if (FAILED(hr))
        {
            break;
        }

        // 設置窗口標題
        setWindowTitle(caption_);

        hr = pDialog_->Show(parent_);
        if (FAILED(hr))
        {
            break;
        }

        // 根據不同的文件接受模式設置對話框類型
        switch (fileMode_)
        {
        case FileDialogAcceptMode::OpenFile:
        case FileDialogAcceptMode::OpenDirectory:
        {
            hr = handleFilesOrDirectoriesSelection();
            break;
        }
        case FileDialogAcceptMode::SaveFile:
        {
            hr = handleSaveFileSelection();
            break;
        }
        default:
            std::wcerr << L"Invalid file mode" << std::endl;
            hr = E_FAIL;
            break;
        }

        if (FAILED(hr))
        {
            break;
        }
    } while (false);

    clear();

    return SUCCEEDED(hr);
}

#if 0
void cx::file_dialog::getDialogHWND()
{
    hWnd_ = nullptr;
}

void cx::file_dialog::applyWindowFlags()
{
    if (nullptr == hWnd_)
    {
        std::wcerr << L"window handle is null" << std::endl;
        return;
    }

    LONG_PTR originStyle = GetWindowLongPtrW(hWnd_, GWL_STYLE);
    LONG_PTR originExStyle = GetWindowLongPtrW(hWnd_, GWL_EXSTYLE);

    // WindowStaysOnTopHint - 保持窗口在最前
    if (flags_ & WindowStaysOnTopHint)
    {
        SetWindowPos(hWnd_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    else
    {
        SetWindowPos(hWnd_, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    // WindowCloseButtonHint - 顯示關閉按鈕
    if (flags_ & WindowCloseButtonHint)
    {
        originStyle |= WS_SYSMENU;
    }
    else
    {
        originStyle &= ~WS_SYSMENU;
    }

    // WindowMinimizeButtonHint - 顯示最小化按鈕
    if (flags_ & WindowMinimizeButtonHint)
    {
        originStyle |= WS_MINIMIZEBOX;
    }
    else
    {
        originStyle &= ~WS_MINIMIZEBOX;
    }

    // WindowMaximizeButtonHint - 顯示最大化按鈕
    if (flags_ & WindowMaximizeButtonHint)
    {
        originStyle |= WS_MAXIMIZEBOX;
    }
    else
    {
        originStyle &= ~WS_MAXIMIZEBOX;
    }

    // WindowContextHelpButtonHint - 顯示上下文幫助按鈕
    if (flags_ & WindowContextHelpButtonHint)
    {
        originExStyle |= WS_EX_CONTEXTHELP;
    }
    else
    {
        originExStyle &= ~WS_EX_CONTEXTHELP;
    }

    // WindowTransparentForInput - 窗口透明
    if (flags_ & WindowTransparentForInput)
    {
        originExStyle |= WS_EX_TRANSPARENT;
    }
    else
    {
        originExStyle &= ~WS_EX_TRANSPARENT;
    }

    // WindowOverridesSystemGestures - 覆蓋系統手勢
    if (flags_ & WindowOverridesSystemGestures)
    {
        originExStyle |= WS_EX_NOREDIRECTIONBITMAP;
    }
    else
    {
        originExStyle &= ~WS_EX_NOREDIRECTIONBITMAP;
    }

    // WindowDoesNotAcceptFocus - 窗口不接受焦點
    if (flags_ & WindowDoesNotAcceptFocus)
    {
        originExStyle |= WS_EX_NOACTIVATE;
    }
    else
    {
        originExStyle &= ~WS_EX_NOACTIVATE;
    }

    // WindowFullscreenButtonHint - 顯示全屏按鈕
    if (flags_ & WindowFullscreenButtonHint)
    {
        originStyle |= WS_MAXIMIZEBOX;
    }
    else
    {
        originStyle &= ~WS_MAXIMIZEBOX;
    }

    // WindowStaysOnBottomHint - 保持窗口在最底
    if (flags_ & WindowStaysOnBottomHint)
    {
        SetWindowPos(hWnd_, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    else
    {
        SetWindowPos(hWnd_, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    // WindowTitleHint - 顯示標題欄
    if (flags_ & WindowTitleHint)
    {
        originStyle |= WS_CAPTION;
    }
    else
    {
        originStyle &= ~WS_CAPTION;
    }

    // WindowSystemMenuHint - 顯示系統菜單
    if (flags_ & WindowSystemMenuHint)
    {
        originStyle |= WS_SYSMENU;
    }
    else
    {
        originStyle &= ~WS_SYSMENU;
    }

    // FramelessWindowHint - 無邊框窗口
    if (flags_ & FramelessWindowHint)
    {
        originStyle &= ~WS_CAPTION;
        originStyle &= ~WS_THICKFRAME;
        originStyle &= ~WS_MINIMIZEBOX;
        originStyle &= ~WS_MAXIMIZEBOX;
        originStyle &= ~WS_SYSMENU;
    }

    // MSWindowsFixedSizeDialogHint - 固定大小窗口
    if (flags_ & MSWindowsFixedSizeDialogHint)
    {
        originStyle |= WS_OVERLAPPEDWINDOW;
        originStyle &= ~WS_THICKFRAME;
        originStyle &= ~WS_MAXIMIZEBOX;
        originStyle &= ~WS_MINIMIZEBOX;
    }

    // MSWindowsOwnDC - 使用自己的 DC
    if (flags_ & MSWindowsOwnDC)
    {
        originStyle |= WS_CLIPCHILDREN;
        originStyle |= WS_CLIPSIBLINGS;
    }

    // 設置新的樣式
    SetWindowLongPtrW(hWnd_, GWL_STYLE, originStyle);

    // 設置新的擴展樣式
    SetWindowLongPtrW(hWnd_, GWL_EXSTYLE, originExStyle);

    // 使新的樣式生效
    SetWindowPos(hWnd_, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);

    // 重繪窗口
    RedrawWindow(hWnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);

    // 使窗口可見
    ShowWindow(hWnd_, SW_SHOW);

    // 更新窗口
    UpdateWindow(hWnd_);

    // 使窗口獲取焦點
    SetFocus(hWnd_);

    // 使窗口處於活動狀態
    SetActiveWindow(hWnd_);

    // 使窗口處於前景
    SetForegroundWindow(hWnd_);

    // 使窗口處於輸入焦點
    SetFocus(hWnd_);
}
#endif