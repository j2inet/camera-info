#include <windows.h>

#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>

#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

using Microsoft::WRL::ComPtr;

namespace
{
constexpr const wchar_t* kColorReset = L"\x1b[0m";
constexpr const wchar_t* kColorHeader = L"\x1b[36m";
constexpr const wchar_t* kColorCamera = L"\x1b[32m";
constexpr const wchar_t* kColorFormat = L"\x1b[33m";
constexpr const wchar_t* kColorError = L"\x1b[31m";

void EnableVirtualTerminal()
{
    HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (stdoutHandle == INVALID_HANDLE_VALUE)
    {
        return;
    }

    DWORD mode = 0;
    if (!GetConsoleMode(stdoutHandle, &mode))
    {
        return;
    }

    if (!SetConsoleMode(stdoutHandle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
    {
        std::wcout << kColorError << L"Warning: ANSI color output is unavailable in this console." << kColorReset << L'\n';
    }
}

std::wstring GuidToString(REFGUID guid)
{
    if (guid == MFVideoFormat_NV12) return L"NV12";
    if (guid == MFVideoFormat_YUY2) return L"YUY2";
    if (guid == MFVideoFormat_MJPG) return L"MJPG";
    if (guid == MFVideoFormat_RGB32) return L"RGB32";
    if (guid == MFVideoFormat_RGB24) return L"RGB24";
    if (guid == MFVideoFormat_I420) return L"I420";
    if (guid == MFVideoFormat_IYUV) return L"IYUV";
    if (guid == MFVideoFormat_UYVY) return L"UYVY";

    wchar_t buffer[64] = {};
    StringFromGUID2(guid, buffer, static_cast<int>(std::size(buffer)));
    return buffer;
}

void PrintMediaTypes(IMFSourceReader* reader)
{
    std::wcout << kColorHeader << L"  Output formats:" << kColorReset << L'\n';

    for (DWORD typeIndex = 0;; ++typeIndex)
    {
        ComPtr<IMFMediaType> mediaType;
        HRESULT hr = reader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, typeIndex, &mediaType);
        if (hr == MF_E_NO_MORE_TYPES)
        {
            if (typeIndex == 0)
            {
                std::wcout << L"    (none)\n";
            }
            break;
        }

        if (FAILED(hr))
        {
            std::wcout << kColorError << L"    Failed to read media type: 0x" << std::hex << hr << std::dec << kColorReset << L'\n';
            break;
        }

        GUID subtype = GUID_NULL;
        UINT32 width = 0;
        UINT32 height = 0;

        mediaType->GetGUID(MF_MT_SUBTYPE, &subtype);
        MFGetAttributeSize(mediaType.Get(), MF_MT_FRAME_SIZE, &width, &height);

        std::wcout << kColorFormat << L"    - " << GuidToString(subtype) << L": "
                   << width << L"x" << height << kColorReset << L'\n';
    }
}

void EnumerateCameras()
{
    ComPtr<IMFAttributes> attributes;
    HRESULT hr = MFCreateAttributes(&attributes, 1);
    if (FAILED(hr))
    {
        std::wcout << kColorError << L"Failed to create attributes: 0x" << std::hex << hr << std::dec << kColorReset << L'\n';
        return;
    }

    hr = attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED(hr))
    {
        std::wcout << kColorError << L"Failed to set source type: 0x" << std::hex << hr << std::dec << kColorReset << L'\n';
        return;
    }

    IMFActivate** devices = nullptr;
    UINT32 deviceCount = 0;
    hr = MFEnumDeviceSources(attributes.Get(), &devices, &deviceCount);
    if (FAILED(hr))
    {
        std::wcout << kColorError << L"Failed to enumerate camera devices: 0x" << std::hex << hr << std::dec << kColorReset << L'\n';
        return;
    }

    std::wcout << kColorHeader << L"Detected cameras: " << deviceCount << kColorReset << L"\n\n";

    for (UINT32 i = 0; i < deviceCount; ++i)
    {
        ComPtr<IMFActivate> device;
        device.Attach(devices[i]);

        wchar_t* cameraName = nullptr;
        UINT32 cameraNameLength = 0;
        hr = device->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &cameraName, &cameraNameLength);
        std::wstring friendlyName = L"Unknown camera";
        if (SUCCEEDED(hr) && cameraName != nullptr)
        {
            friendlyName = cameraName;
        }
        if (cameraName != nullptr)
        {
            CoTaskMemFree(cameraName);
        }

        std::wcout << kColorCamera << L"Camera " << (i + 1) << L": " << friendlyName << kColorReset << L'\n';

        ComPtr<IMFMediaSource> mediaSource;
        hr = device->ActivateObject(IID_PPV_ARGS(&mediaSource));
        if (FAILED(hr))
        {
            std::wcout << kColorError << L"  Failed to activate camera: 0x" << std::hex << hr << std::dec << kColorReset << L"\n\n";
            continue;
        }

        ComPtr<IMFSourceReader> sourceReader;
        hr = MFCreateSourceReaderFromMediaSource(mediaSource.Get(), nullptr, &sourceReader);
        if (FAILED(hr))
        {
            std::wcout << kColorError << L"  Failed to create source reader: 0x" << std::hex << hr << std::dec << kColorReset << L"\n\n";
            mediaSource->Shutdown();
            continue;
        }

        PrintMediaTypes(sourceReader.Get());
        std::wcout << L'\n';

        mediaSource->Shutdown();
    }

    CoTaskMemFree(devices);
}
} // namespace

int wmain()
{
    EnableVirtualTerminal();

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        std::wcout << kColorError << L"COM initialization failed: 0x" << std::hex << hr << std::dec << kColorReset << L'\n';
        return 1;
    }

    hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
    {
        std::wcout << kColorError << L"Media Foundation startup failed: 0x" << std::hex << hr << std::dec << kColorReset << L'\n';
        CoUninitialize();
        return 1;
    }

    EnumerateCameras();

    MFShutdown();
    CoUninitialize();
    return 0;
}
