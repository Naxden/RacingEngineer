// Fill out your copyright notice in the Description page of Project Settings.


#include "MapFilePicker.h"

#include "ImageUtils.h"

#pragma region WindowsFileDialogHandler

#include <windows.h>      // For common windows data types and function headers
#include <shlobj.h>
#include <shobjidl.h>     // for IFileDialogEvents and IFileDialogControlEvents
#include <shlwapi.h>
#include <strsafe.h>      // for StringCchPrintfW
#include <shtypes.h>      // for COMDLG_FILTERSPEC
#include <new>

#undef LoadImage
#undef CopyFile
#undef DeleteFile
#undef min
#undef max

const COMDLG_FILTERSPEC c_rgSaveTypes[] =
{
    {L"Image (*.png)",       L"*.png"},
    {L"All Documents (*.*)", L"*.*"}
};

// Indices of file types
#define INDEX_PNG 1

/* File Dialog Event Handler *****************************************************************************************************/

class CDialogEventHandler : public IFileDialogEvents,
    public IFileDialogControlEvents
{
public:
    // IUnknown methods
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        static const QITAB qit[] = {
            QITABENT(CDialogEventHandler, IFileDialogEvents),
            QITABENT(CDialogEventHandler, IFileDialogControlEvents),
            { 0 },
#pragma warning(suppress:4838)
        };
        return QISearch(this, qit, riid, ppv);
    }

    IFACEMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&_cRef);
    }

    IFACEMETHODIMP_(ULONG) Release()
    {
        long cRef = InterlockedDecrement(&_cRef);
        if (!cRef)
            delete this;
        return cRef;
    }

    // IFileDialogEvents methods
    IFACEMETHODIMP OnFileOk(IFileDialog*) { return S_OK; };
    IFACEMETHODIMP OnFolderChange(IFileDialog*) { return S_OK; };
    IFACEMETHODIMP OnFolderChanging(IFileDialog*, IShellItem*) { return S_OK; };
    IFACEMETHODIMP OnHelp(IFileDialog*) { return S_OK; };
    IFACEMETHODIMP OnSelectionChange(IFileDialog*) { return S_OK; };
    IFACEMETHODIMP OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) { return S_OK; };
    IFACEMETHODIMP OnTypeChange(IFileDialog*) { return S_OK; };
    IFACEMETHODIMP OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*) { return S_OK; };

    // IFileDialogControlEvents methods
    IFACEMETHODIMP OnItemSelected(IFileDialogCustomize*, DWORD, DWORD) { return S_OK; };
    IFACEMETHODIMP OnButtonClicked(IFileDialogCustomize*, DWORD) { return S_OK; };
    IFACEMETHODIMP OnCheckButtonToggled(IFileDialogCustomize*, DWORD, BOOL) { return S_OK; };
    IFACEMETHODIMP OnControlActivating(IFileDialogCustomize*, DWORD) { return S_OK; };

    CDialogEventHandler() : _cRef(1) {};
private:
    ~CDialogEventHandler() {};
    long _cRef;
};

// Instance creation helper
HRESULT CDialogEventHandler_CreateInstance(REFIID riid, void** ppv)
{
    *ppv = NULL;
    CDialogEventHandler* pDialogEventHandler = new (std::nothrow) CDialogEventHandler();
    HRESULT hr = pDialogEventHandler ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr))
    {
        hr = pDialogEventHandler->QueryInterface(riid, ppv);
        pDialogEventHandler->Release();
    }
    return hr;
}

/* Utility Functions *************************************************************************************************************/

// A helper function that converts UNICODE data to ANSI and writes it to the given file.
// We write in ANSI format to make it easier to open the output file in Notepad.
HRESULT _WriteDataToFile(HANDLE hFile, PCWSTR pszDataIn)
{
    // First figure out our required buffer size.
    DWORD cbData = WideCharToMultiByte(CP_ACP, 0, pszDataIn, -1, NULL, 0, NULL, NULL);
    HRESULT hr = (cbData == 0) ? HRESULT_FROM_WIN32(GetLastError()) : S_OK;
    if (SUCCEEDED(hr))
    {
        // Now allocate a buffer of the required size, and call WideCharToMultiByte again to do the actual conversion.
        char* pszData = new (std::nothrow) CHAR[cbData];
        hr = pszData ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            hr = WideCharToMultiByte(CP_ACP, 0, pszDataIn, -1, pszData, cbData, NULL, NULL)
                ? S_OK
                : HRESULT_FROM_WIN32(GetLastError());
            if (SUCCEEDED(hr))
            {
                DWORD dwBytesWritten = 0;
                hr = WriteFile(hFile, pszData, cbData - 1, &dwBytesWritten, NULL)
                    ? S_OK
                    : HRESULT_FROM_WIN32(GetLastError());
            }
            delete[] pszData;
        }
    }
    return hr;
}

// Helper function to write property/value into a custom file format.
//
// We are inventing a dummy format here:
// [APPDATA]
// xxxxxx
// [ENDAPPDATA]
// [PROPERTY]foo=bar[ENDPROPERTY]
// [PROPERTY]foo2=bar2[ENDPROPERTY]
HRESULT _WritePropertyToCustomFile(PCWSTR pszFileName, PCWSTR pszPropertyName, PCWSTR pszValue)
{
    HANDLE hFile = CreateFileW(pszFileName,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_ALWAYS, // We will write only if the file exists.
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    HRESULT hr = (hFile == INVALID_HANDLE_VALUE) ? HRESULT_FROM_WIN32(GetLastError()) : S_OK;
    if (SUCCEEDED(hr))
    {
        const WCHAR           wszPropertyStartTag[] = L"[PROPERTY]";
        const WCHAR           wszPropertyEndTag[] = L"[ENDPROPERTY]\r\n";
        const DWORD           cchPropertyStartTag = (DWORD)wcslen(wszPropertyStartTag);
        const static DWORD    cchPropertyEndTag = (DWORD)wcslen(wszPropertyEndTag);
        DWORD const cchPropertyLine = cchPropertyStartTag +
            cchPropertyEndTag +
            (DWORD)wcslen(pszPropertyName) +
            (DWORD)wcslen(pszValue) +
            2; // 1 for '=' + 1 for NULL terminator.
        PWSTR pszPropertyLine = new (std::nothrow) WCHAR[cchPropertyLine];
        hr = pszPropertyLine ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            hr = StringCchPrintfW(pszPropertyLine,
                cchPropertyLine,
                L"%s%s=%s%s",
                wszPropertyStartTag,
                pszPropertyName,
                pszValue,
                wszPropertyEndTag);
            if (SUCCEEDED(hr))
            {
                hr = SetFilePointer(hFile, 0, NULL, FILE_END) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
                if (SUCCEEDED(hr))
                {
                    hr = _WriteDataToFile(hFile, pszPropertyLine);
                }
            }
            delete[] pszPropertyLine;
        }
        CloseHandle(hFile);
    }

    return hr;
}

// Helper function to write dummy content to a custom file format.
//
// We are inventing a dummy format here:
// [APPDATA]
// xxxxxx
// [ENDAPPDATA]
// [PROPERTY]foo=bar[ENDPROPERTY]
// [PROPERTY]foo2=bar2[ENDPROPERTY]
HRESULT _WriteDataToCustomFile(PCWSTR pszFileName)
{
    HANDLE hFile = CreateFileW(pszFileName,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,  // Let's always create this file.
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    HRESULT hr = (hFile == INVALID_HANDLE_VALUE) ? HRESULT_FROM_WIN32(GetLastError()) : S_OK;
    if (SUCCEEDED(hr))
    {
        WCHAR wszDummyContent[] = L"[MYAPPDATA]\r\nThis is an example of how to use the IFileSaveDialog interface.\r\n[ENDMYAPPDATA]\r\n";

        hr = _WriteDataToFile(hFile, wszDummyContent);
        CloseHandle(hFile);
    }
    return hr;
}

/* Common File Dialog Snippets ***************************************************************************************************/

// This code snippet demonstrates how to work with the common file dialog interface
HRESULT BasicFileOpen(FString& outPath)
{
    // CoCreate the File Open Dialog object.
    IFileDialog* pfd = NULL;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
    if (SUCCEEDED(hr))
    {
        // Create an event handling object, and hook it up to the dialog.
        IFileDialogEvents* pfde = NULL;
        hr = CDialogEventHandler_CreateInstance(IID_PPV_ARGS(&pfde));
        if (SUCCEEDED(hr))
        {
            // Hook up the event handler.
            DWORD dwCookie;
            hr = pfd->Advise(pfde, &dwCookie);
            if (SUCCEEDED(hr))
            {
                // Set the options on the dialog.
                DWORD dwFlags;

                // Before setting, always get the options first in order not to override existing options.
                hr = pfd->GetOptions(&dwFlags);
                if (SUCCEEDED(hr))
                {
                    // In this case, get shell items only for file system items.
                    hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM);
                    if (SUCCEEDED(hr))
                    {
                        // Set the file types to display only. Notice that, this is a 1-based array.
                        hr = pfd->SetFileTypes(ARRAYSIZE(c_rgSaveTypes), c_rgSaveTypes);
                        if (SUCCEEDED(hr))
                        {
                            // Set the selected file type index to Png Images for this example.
                            hr = pfd->SetFileTypeIndex(INDEX_PNG);
                            if (SUCCEEDED(hr))
                            {
                                // Set the default extension to be ".png" file.
                                hr = pfd->SetDefaultExtension(L"png");
                                if (SUCCEEDED(hr))
                                {
                                    // Show the dialog
                                    hr = pfd->Show(NULL);
                                    if (SUCCEEDED(hr))
                                    {
                                        // Obtain the result, once the user clicks the 'Open' button.
                                        // The result is an IShellItem object.
                                        IShellItem* psiResult;
                                        hr = pfd->GetResult(&psiResult);
                                        if (SUCCEEDED(hr))
                                        {
                                            // We are just going to print out the name of the file for sample sake.
                                            PWSTR pszFilePath = NULL;
                                            hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                                            if (SUCCEEDED(hr))
                                            {
												outPath = pszFilePath;

                                                CoTaskMemFree(pszFilePath);
                                            }
                                            psiResult->Release();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                // Unhook the event handler.
                pfd->Unadvise(dwCookie);
            }
            pfde->Release();
        }
        pfd->Release();
    }
    return hr;
}

#pragma endregion


UMapFilePicker::UMapFilePicker()
{

}

FString UMapFilePicker::OpenFileDialog()
{
	const UEngine* Engine = GEngine;
	if (Engine != nullptr)
	{
        if (FWindowsPlatformMisc::CoInitialize())
        {
			FString ImagePath;
			HRESULT hr = BasicFileOpen(ImagePath);
			FWindowsPlatformMisc::CoUninitialize();

			if (SUCCEEDED(hr))
			{
                UE_LOG(LogTemp, Log, TEXT("UMapFilePicker::OpenFileDialog() Selected path %s"), *ImagePath);
				return ImagePath;
			}
            else
            {
				UE_LOG(LogTemp, Error, TEXT("UMapFilePicker::OpenFileDialog() BasicFileOpen() failed"));
            }
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UMapFilePicker::OpenFileDialog() CoInitialize() failed"));
        }
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UMapFilePicker::OpenFileDialog() Engine is nullptr"));
	}

	return "";
}


UTexture2D* UMapFilePicker::LoadFileToTexture(const FString& FilePath)
{
	FImage Image;

	if (!FilePath.IsEmpty())
	{
		if (FImageUtils::LoadImage(*FilePath, Image))
		{
			return FImageUtils::CreateTexture2DFromImage(Image);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UMapFilePicker::LoadFileToTexture() Failed to LoadImage"))
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UMapFilePicker::LoadFileToTexture() FilePath is empty string"))
	}


	return nullptr;
}
