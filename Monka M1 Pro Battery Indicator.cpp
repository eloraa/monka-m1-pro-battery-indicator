#include <windows.h>
#include <dwmapi.h>
#include <tchar.h>
#include <random>
#include <commctrl.h>
#include <windowsx.h> 
#include <dbt.h>     
#include <initguid.h> 
#include <devguid.h>  
#include <hidclass.h>
#include <shellapi.h> 
#include <fstream>   
#include <vector>    
#include <string>     
#include "resource.h"
#include "resource_loader.h"
#include "ui_renderer.h"
#include "tray_icon_renderer.h"
#include "device_discovery.h"
#include "settings_view.h"
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")

static TCHAR szClass[] = _T("MonkaM1ProBatteryIndicator");
static TCHAR szTitle[] = _T("Monka M1 Pro Battery Indicator");
HINSTANCE hInst;
HDEVNOTIFY hDeviceNotify = NULL;
HANDLE g_hMutex = NULL;

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAYICON 1001
#define ID_SHOW_WINDOW 1002
#define ID_EXIT 1003
#define ID_LAUNCH_AT_STARTUP 1004
#define ID_ENABLE_NOTIFICATIONS 1005
NOTIFYICONDATA g_notifyIconData = {0};
bool g_isWindowVisible = true;

bool g_notificationsEnabled = true;
int g_fullChargeCounter = 0;
bool g_hasNotifiedFullCharge = false;
bool g_hasNotifiedLowBattery = false;
int g_lastBatteryLevel = -1;
bool g_lastChargingState = false;
bool g_chargingStateInitialized = false;

bool g_isUsingMockData = false;

enum NotificationType {
    NOTIFICATION_FULL_CHARGE,
    NOTIFICATION_LOW_BATTERY,
    NOTIFICATION_CRITICAL_BATTERY
};

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif
#ifndef DWMWA_TEXT_COLOR
#define DWMWA_TEXT_COLOR 36
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif

#define DWMWCP_DEFAULT 0
#define DWMWCP_DONOTROUND 1
#define DWMWCP_ROUND 2
#define DWMWCP_ROUNDSMALL 3

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

bool IsLaunchAtStartupEnabled();
void SetLaunchAtStartup(bool enable);
void LoadStartupSettings();
void SaveStartupSettings();

bool IsNotificationsEnabled();
void SetNotificationsEnabled(bool enable);
void CheckBatteryNotifications();
void ShowNotification(const std::wstring& title, const std::wstring& message, DWORD flags = NIIF_INFO);
void ShowCustomNotification(const std::wstring& title, const std::wstring& message, NotificationType type, int batteryLevel, bool isCharging);
bool IsWindowsVistaOrLater();
HICON CreateNotificationIcon(NotificationType type, int batteryLevel, bool isCharging);

bool InitializeSystemTray(HWND hWnd);
void CleanupSystemTray();
void UpdateTrayIcon();
void ShowContextMenu(HWND hWnd, POINT pt);
void ShowHideWindow(HWND hWnd);
void MinimizeToTray(HWND hWnd);

COLORREF GetRandomColor()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 255);

    return RGB(dis(gen), dis(gen), dis(gen));
}

BOOL IsDarkMode()
{
    HKEY hKey;
    DWORD value = 1;
    DWORD size = sizeof(value);
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr, (LPBYTE)&value, &size) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return (value == 0);
        }
        RegCloseKey(hKey);
    }

    COLORREF windowColor = GetSysColor(COLOR_WINDOW);
    int brightness = GetRValue(windowColor) + GetGValue(windowColor) + GetBValue(windowColor);

    return brightness < 384;
}

void ApplyWindowDecor(HWND hWnd, COLORREF borderColor = RGB(139, 195, 74))
{
    DWORD pref = DWMWCP_DONOTROUND;
    DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &pref, sizeof(pref));

    COLORREF bg = IsDarkMode()
        ? RGB(31, 31, 31)
        : RGB(255, 255, 255);
    COLORREF txt = IsDarkMode()
        ? RGB(255, 255, 255)
        : RGB(0, 0, 0);

    DwmSetWindowAttribute(hWnd, DWMWA_CAPTION_COLOR, &bg, sizeof(bg));
    DwmSetWindowAttribute(hWnd, DWMWA_TEXT_COLOR, &txt, sizeof(txt));

    DwmSetWindowAttribute(hWnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
}

HICON LoadIconFromResource(int resourceId, int size = 32)
{
    HICON hIcon = (HICON)LoadImage(
        GetModuleHandle(NULL),
        MAKEINTRESOURCE(resourceId),
        IMAGE_ICON,
        size, size,
        LR_DEFAULTCOLOR);

    return hIcon;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    hInst = hInstance;

    g_hMutex = CreateMutexA(NULL, TRUE, "MonkaM1ProBatteryIndicator_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (g_hMutex) {
            CloseHandle(g_hMutex);
            g_hMutex = NULL;
        }
        
        HWND hExistingWindow = FindWindowA("MonkaM1ProBatteryIndicator", NULL);
        if (hExistingWindow) {
            ShowWindow(hExistingWindow, SW_RESTORE);
            SetForegroundWindow(hExistingWindow);
            BringWindowToTop(hExistingWindow);
        }
        
        return 0; 
    }

    bool startMinimized = false;
    if (lpCmdLine && strstr(lpCmdLine, "-minimized") != nullptr) {
        startMinimized = true;
        nCmdShow = SW_HIDE;
    }
    WNDCLASSEX wc = { sizeof(wc) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = szClass;

    HICON hIcon = LoadIconFromResource(IDI_MONKAM1PROBATTERYINDICATOR, 32);
    HICON hIconSmall = LoadIconFromResource(IDI_SMALL, 16);

    if (hIcon)
    {
        wc.hIcon = hIcon;
        wc.hIconSm = hIconSmall ? hIconSmall : hIcon;
    }
    else
    {
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    }

    RegisterClassEx(&wc);

    HWND hWnd = CreateWindowEx(
        0,
        szClass,
        _T(""), 
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_BORDER,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 700,
        NULL, NULL, hInst, NULL);

    if (!hWnd)
        return 1;

    if (!UIRenderer::Initialize()) {
        MessageBoxW(NULL, L"Failed to initialize UI renderer", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    SettingsView::LoadUISettings();

    LoadStartupSettings();

    UIRenderer::SetMainWindow(hWnd);

    if (!InitializeSystemTray(hWnd)) {
        MessageBoxW(NULL, L"Failed to initialize system tray", L"Warning", MB_OK | MB_ICONWARNING);
    }
    
    TRACKMOUSEEVENT tme = {};
    tme.cbSize = sizeof(TRACKMOUSEEVENT);
    tme.dwFlags = TME_LEAVE;
    tme.hwndTrack = hWnd;
    TrackMouseEvent(&tme);
    

    SetTimer(hWnd, 2, 5000, NULL);

    SetTimer(hWnd, 3, 2000, NULL); 
    DEV_BROADCAST_DEVICEINTERFACE notificationFilter = {0};
    notificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    notificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    notificationFilter.dbcc_classguid = GUID_DEVINTERFACE_HID; 

    hDeviceNotify = RegisterDeviceNotification(
        hWnd,
        &notificationFilter,
        DEVICE_NOTIFY_WINDOW_HANDLE
    );

    if (!hDeviceNotify) {
        OutputDebugStringA("Warning: Failed to register for device change notifications\n");
    }

    ApplyWindowDecor(hWnd);

    if (startMinimized) {
        g_isWindowVisible = false;
        ShowWindow(hWnd, SW_HIDE);
    } else {
        g_isWindowVisible = true;
        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rc;
        GetClientRect(hWnd, &rc);
        int width = rc.right - rc.left;
        int height = rc.bottom - rc.top;

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

        COLORREF bgColor = IsDarkMode() ? RGB(31, 31, 31) : RGB(255, 255, 255);
        HBRUSH bgBrush = CreateSolidBrush(bgColor);
        FillRect(memDC, &rc, bgBrush);
        DeleteObject(bgBrush);

        UIRenderer::RenderUI(memDC, rc);

        BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);

        EndPaint(hWnd, &ps);
    }
    break;

    case WM_LBUTTONDOWN:
    {
        int mouseX = GET_X_LPARAM(lParam);
        int mouseY = GET_Y_LPARAM(lParam);

        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;

        if (UIRenderer::IsSettingsViewVisible()) {
            if (UIRenderer::HandleSettingsClick(mouseX, mouseY, width, height)) {
                InvalidateRect(hWnd, NULL, FALSE);
            }
        } else {
            int clickedItemIndex = UIRenderer::HandleMouseClick(hWnd, mouseX, mouseY, width, height);
            if (clickedItemIndex >= 0) {
                auto mouseList = UIRenderer::GetMouseList();
                if (clickedItemIndex < static_cast<int>(mouseList.size())) {
                    UIRenderer::ShowDeviceSettings(&mouseList[clickedItemIndex]);
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            } else {
                ApplyWindowDecor(hWnd, GetRandomColor());
            }
        }
        break;
    }
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        ApplyWindowDecor(hWnd, GetRandomColor());
        break;

    case WM_SETFOCUS:
        ApplyWindowDecor(hWnd, GetRandomColor());
        break;

    case WM_KILLFOCUS:
    {
        COLORREF transparentColor = CLR_NONE;
        DwmSetWindowAttribute(hWnd, DWMWA_BORDER_COLOR, &transparentColor, sizeof(COLORREF));
        break;
    }

    case WM_ERASEBKGND:
    {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        
        HBRUSH hBrush = CreateSolidBrush(RGB(31, 31, 31));
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);
        
        return 1;
    }

    case WM_MOUSEMOVE:
    {
        int mouseX = GET_X_LPARAM(lParam);
        int mouseY = GET_Y_LPARAM(lParam);

        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;

        if (UIRenderer::IsSettingsViewVisible()) {
            UIRenderer::HandleSettingsHover(mouseX, mouseY, width, height);
            InvalidateRect(hWnd, NULL, FALSE);
        } else {
            UIRenderer::UpdateMouseHover(hWnd, mouseX, mouseY, width, height);
            UIRenderer::UpdateHoverAnimation(hWnd);
        }
        return 0;
    }

    case WM_MOUSELEAVE:
    {
        UIRenderer::ClearMouseHover(hWnd);
        return 0;
    }

    case WM_TIMER:
    {
        if (wParam == 1) {
            bool hasActiveAnimations = UIRenderer::UpdateHoverAnimation(hWnd);

            if (!hasActiveAnimations) {
                UIRenderer::StopAnimationTimer(hWnd);
            }
        } else if (wParam == 2) {
            UIRenderer::UpdateBatteryStatus(hWnd);
            UpdateTrayIcon();
            CheckBatteryNotifications();
        } else if (wParam == 3) {
            UIRenderer::CheckDeviceHealth(hWnd);
            UpdateTrayIcon();
        }
        return 0;
    }

    case WM_GETMINMAXINFO:
    {
        LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        
        lpMMI->ptMinTrackSize.x = 400; // Minimum width
        lpMMI->ptMaxTrackSize.x = 400; // Maximum width (minimum = fixed width)
        lpMMI->ptMinTrackSize.y = 200; // Minimum height
        lpMMI->ptMaxTrackSize.y = 1000; // Maximum height
        
        return 0;
    }

    case WM_DEVICECHANGE:
    {
        if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE || wParam == DBT_DEVNODES_CHANGED) {
            if (lParam) {
                DEV_BROADCAST_HDR* pHdr = (DEV_BROADCAST_HDR*)lParam;
                if (pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
                    UIRenderer::OnDeviceChange();
                }
            } else {
                UIRenderer::OnDeviceChange();
            }
        }
        break;
    }

    case WM_TRAYICON:
    {
        if (lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            ShowContextMenu(hWnd, pt);
        }
        break;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam)) {
        case ID_SHOW_WINDOW:
            ShowHideWindow(hWnd);
            break;
        case ID_LAUNCH_AT_STARTUP:
            SetLaunchAtStartup(!IsLaunchAtStartupEnabled());
            break;
        case ID_ENABLE_NOTIFICATIONS:
            SetNotificationsEnabled(!IsNotificationsEnabled());
            break;
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, AboutDlgProc);
            break;
        case ID_EXIT:
            PostQuitMessage(0);
            break;
        }
        break;
    }

    case WM_SYSCOMMAND:
    {
        if (wParam == SC_MINIMIZE) {
            MinimizeToTray(hWnd);
            return 0;
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    case WM_SETTINGCHANGE:
    case WM_THEMECHANGED:
        ApplyWindowDecor(hWnd);
        InvalidateRect(hWnd, NULL, TRUE);
        UpdateTrayIcon();
        break;


    case WM_NCHITTEST:
        return DefWindowProc(hWnd, msg, wParam, lParam);

    case WM_CLOSE:
        MinimizeToTray(hWnd);
        return 0;

    case WM_DESTROY:
        KillTimer(hWnd, 1);
        KillTimer(hWnd, 2); 
        KillTimer(hWnd, 3); 

        if (hDeviceNotify) {
            UnregisterDeviceNotification(hDeviceNotify);
            hDeviceNotify = NULL;
        }

        CleanupSystemTray();

        UIRenderer::Cleanup();
        
        if (g_hMutex) {
            CloseHandle(g_hMutex);
            g_hMutex = NULL;
        }
        
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

bool IsLaunchAtStartupEnabled() {
    HKEY hKey;
    bool isEnabled = false;

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        DWORD valueType;
        DWORD dataSize = 0;

        LONG result = RegQueryValueExW(hKey, L"MonkaM1ProBatteryIndicator",
                                      nullptr, &valueType, nullptr, &dataSize);

        isEnabled = (result == ERROR_SUCCESS && dataSize > 0);
        RegCloseKey(hKey);
    }

    return isEnabled;
}

void SetLaunchAtStartup(bool enable) {
    HKEY hKey;

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {

        if (enable) {
            wchar_t exePath[MAX_PATH];
            GetModuleFileNameW(NULL, exePath, MAX_PATH);

            std::wstring commandLine = L"\"" + std::wstring(exePath) + L"\" -minimized";

            RegSetValueExW(hKey, L"MonkaM1ProBatteryIndicator", 0, REG_SZ,
                          (const BYTE*)commandLine.c_str(),
                          static_cast<DWORD>((commandLine.length() + 1) * sizeof(wchar_t)));
        } else {
            RegDeleteValueW(hKey, L"MonkaM1ProBatteryIndicator");
        }

        RegCloseKey(hKey);
    }
}

void LoadStartupSettings() {
    std::ifstream configFile("Config.ini");
    if (!configFile.is_open()) {
        std::string configContent = ResourceLoader::LoadResourceAsString(IDR_CONFIG_INI);
        if (!configContent.empty()) {
            std::ofstream createFile("Config.ini");
            createFile << configContent;
            createFile.close();
            
            configFile.open("Config.ini");
        }
    }
    
    if (!configFile.is_open()) {
        return;
    }

    std::string line;
    bool inOptionSection = false;

    while (std::getline(configFile, line)) {
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line == "[Option]") {
            inOptionSection = true;
            continue;
        }

        if (line.empty() || line[0] == '[') {
            inOptionSection = false;
            continue;
        }

        if (inOptionSection) {
            size_t equalPos = line.find('=');
            if (equalPos != std::string::npos) {
                std::string key = line.substr(0, equalPos);
                std::string value = line.substr(equalPos + 1);

                if (key == "EnableNotifications") {
                    g_notificationsEnabled = (value == "1");
                }
            }
        }
    }

    configFile.close();
}

void SaveStartupSettings() {
    std::ifstream checkFile("Config.ini");
    bool fileExists = checkFile.is_open();
    checkFile.close();
    
    if (!fileExists) {
        std::string configContent = ResourceLoader::LoadResourceAsString(IDR_CONFIG_INI);
        if (!configContent.empty()) {
            std::ofstream createFile("Config.ini");
            createFile << configContent;
            createFile.close();
        }
    }

    std::ifstream configFile("Config.ini");
    std::vector<std::string> lines;
    std::string line;

    while (configFile.is_open() && std::getline(configFile, line)) {
        lines.push_back(line);
    }
    configFile.close();

    if (lines.empty()) {
        lines.push_back("[Option]");
        lines.push_back("EnableNotifications=" + std::string(g_notificationsEnabled ? "1" : "0"));
        lines.push_back("");
        lines.push_back("[UISettings]");
        lines.push_back("IconMode=COLORED");
        lines.push_back("");
        lines.push_back("[ColorSettings]");
    } else {
        bool optionSectionFound = false;
        bool notificationUpdated = false;

        for (size_t i = 0; i < lines.size(); i++) {
            std::string trimmedLine = lines[i];
            trimmedLine.erase(0, trimmedLine.find_first_not_of(" \t"));
            trimmedLine.erase(trimmedLine.find_last_not_of(" \t") + 1);

            if (trimmedLine == "[Option]") {
                optionSectionFound = true;
                continue;
            }

            if (optionSectionFound && trimmedLine.find("EnableNotifications=") == 0) {
                lines[i] = "EnableNotifications=" + std::string(g_notificationsEnabled ? "1" : "0");
                notificationUpdated = true;
                break;
            }

            if (optionSectionFound && (trimmedLine.empty() || trimmedLine[0] == '[')) {
                if (!notificationUpdated) {
                    lines.insert(lines.begin() + i, "EnableNotifications=" + std::string(g_notificationsEnabled ? "1" : "0"));
                }
                break;
            }
        }

        if (!optionSectionFound) {
            if (!lines.empty() && !lines.back().empty()) {
                lines.push_back("");
            }
            lines.push_back("[Option]");
            lines.push_back("EnableNotifications=" + std::string(g_notificationsEnabled ? "1" : "0"));
        }
    }

    std::ofstream outFile("Config.ini");
    for (const auto& fileLine : lines) {
        outFile << fileLine << "\n";
    }
    outFile.close();
}

bool IsNotificationsEnabled() {
    return g_notificationsEnabled;
}

void SetNotificationsEnabled(bool enable) {
    g_notificationsEnabled = enable;
    SaveStartupSettings();
}

void ShowNotification(const std::wstring& title, const std::wstring& message, DWORD flags) {
    if (!g_notificationsEnabled) return;

    g_notifyIconData.uFlags |= NIF_INFO;
    g_notifyIconData.dwInfoFlags = flags;
    wcscpy_s(g_notifyIconData.szInfoTitle, title.c_str());
    wcscpy_s(g_notifyIconData.szInfo, message.c_str());

    Shell_NotifyIcon(NIM_MODIFY, &g_notifyIconData);

    g_notifyIconData.uFlags &= ~NIF_INFO;
}

void CheckBatteryNotifications() {
    if (!g_notificationsEnabled) return;

    extern DeviceDiscovery& GetDeviceDiscovery();
    DeviceDiscovery& discovery = GetDeviceDiscovery();

    const auto& devices = discovery.GetDiscoveredDevices();
    if (devices.empty()) return;

    const auto& device = devices[0];
    bool isOnline = discovery.IsDeviceOnline(device.devicePath);
    if (!isOnline) return;

    auto status = discovery.GetBatteryStatus(device.devicePath);
    int batteryLevel = 0;
    bool isCharging = (status.isCharging != 0);

    if (status.level > 0) {
        batteryLevel = status.level;
    } else if (status.BatVoltage > 0) {
        batteryLevel = discovery.calculateBatteryPercentage(status.BatVoltage);
    }

    if (g_lastBatteryLevel != -1 && abs(batteryLevel - g_lastBatteryLevel) > 5) {
        if (batteryLevel > g_lastBatteryLevel + 5) {
            g_hasNotifiedLowBattery = false;
        }
        if (batteryLevel < g_lastBatteryLevel - 5) {
            g_hasNotifiedFullCharge = false;
            g_fullChargeCounter = 0;
        }
    }

    if (g_chargingStateInitialized && isCharging != g_lastChargingState) {
        if (isCharging && !g_lastChargingState) {
            g_hasNotifiedFullCharge = false;
            g_fullChargeCounter = 0;
        } else if (!isCharging && g_lastChargingState) {
            g_hasNotifiedLowBattery = false;
        }
    }

    g_lastChargingState = isCharging;
    g_chargingStateInitialized = true;
    g_lastBatteryLevel = batteryLevel;

    if (isCharging && batteryLevel == 100) {
        g_fullChargeCounter++;
        if (g_fullChargeCounter >= 8 && !g_hasNotifiedFullCharge) {
            ShowCustomNotification(L"Battery Full", L"Your Monka M1 Pro is fully charged!", NOTIFICATION_FULL_CHARGE, batteryLevel, isCharging);
            g_hasNotifiedFullCharge = true;
        }
    } else {
        g_fullChargeCounter = 0;
    }

    if (!isCharging && !g_hasNotifiedLowBattery) {
        if (batteryLevel <= 10) {
            ShowCustomNotification(L"Critical Battery", L"Battery is critically low (10%). Please charge your Monka M1 Pro.", NOTIFICATION_CRITICAL_BATTERY, batteryLevel, isCharging);
            g_hasNotifiedLowBattery = true;
        } else if (batteryLevel <= 20) {
            ShowCustomNotification(L"Low Battery", L"Battery is low (20%). Consider charging your Monka M1 Pro.", NOTIFICATION_LOW_BATTERY, batteryLevel, isCharging);
            g_hasNotifiedLowBattery = true;
        }
    }
}

bool IsWindowsVistaOrLater() {
    HMODULE hMod = GetModuleHandle(L"kernel32.dll");
    if (hMod) {
        FARPROC proc = GetProcAddress(hMod, "GetTickCount64");
        if (proc != NULL) {
            return true; 
        }
    }

    HMODULE dwmMod = LoadLibrary(L"dwmapi.dll");
    if (dwmMod) {
        FreeLibrary(dwmMod);
        return true; // VISTA | LATER (has DWM)
    }

    return false;
}

HICON CreateNotificationIcon(NotificationType type, int batteryLevel, bool isCharging) {
    const int notificationIconSize = 32; 
    int displayLevel = batteryLevel;
    bool displayCharging = isCharging;

    switch (type) {
        case NOTIFICATION_FULL_CHARGE:
            displayLevel = 100;
            displayCharging = true;
            break;
        case NOTIFICATION_LOW_BATTERY:
            displayLevel = 20;
            displayCharging = false;
            break;
        case NOTIFICATION_CRITICAL_BATTERY:
            displayLevel = 10;
            displayCharging = false;
            break;
    }

    return TrayIconRenderer::CreateBatteryIcon(displayLevel, displayCharging, true, false);
}

void ShowCustomNotification(const std::wstring& title, const std::wstring& message, NotificationType type, int batteryLevel, bool isCharging) {
    if (!g_notificationsEnabled) return;

    HICON customIcon = CreateNotificationIcon(type, batteryLevel, isCharging);

    if (customIcon) {
        g_notifyIconData.uFlags |= NIF_INFO;
        g_notifyIconData.dwInfoFlags = NIIF_USER; 
        if (IsWindowsVistaOrLater()) {
            g_notifyIconData.hBalloonIcon = customIcon;
            g_notifyIconData.dwInfoFlags |= NIIF_LARGE_ICON;
        } else {
            HICON oldIcon = g_notifyIconData.hIcon;
            g_notifyIconData.hIcon = customIcon;

            wcscpy_s(g_notifyIconData.szInfoTitle, title.c_str());
            wcscpy_s(g_notifyIconData.szInfo, message.c_str());
            Shell_NotifyIcon(NIM_MODIFY, &g_notifyIconData);

            g_notifyIconData.hIcon = oldIcon;
            g_notifyIconData.uFlags &= ~NIF_INFO;

            DestroyIcon(customIcon);
            return;
        }

        wcscpy_s(g_notifyIconData.szInfoTitle, title.c_str());
        wcscpy_s(g_notifyIconData.szInfo, message.c_str());

        Shell_NotifyIcon(NIM_MODIFY, &g_notifyIconData);

        g_notifyIconData.uFlags &= ~NIF_INFO;
        g_notifyIconData.hBalloonIcon = NULL;

        DestroyIcon(customIcon);
    } else {
        ShowNotification(title, message, NIIF_INFO);
    }
}

bool InitializeSystemTray(HWND hWnd) {
    ZeroMemory(&g_notifyIconData, sizeof(NOTIFYICONDATA));
    g_notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
    g_notifyIconData.hWnd = hWnd;
    g_notifyIconData.uID = ID_TRAYICON;
    g_notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_notifyIconData.uCallbackMessage = WM_TRAYICON;

    wcscpy_s(g_notifyIconData.szTip, L"Monka M1 Pro Battery Indicator");

    UpdateTrayIcon();

    return Shell_NotifyIcon(NIM_ADD, &g_notifyIconData) != FALSE;
}

void CleanupSystemTray() {
    if (g_notifyIconData.hIcon) {
        DestroyIcon(g_notifyIconData.hIcon);
    }
    Shell_NotifyIcon(NIM_DELETE, &g_notifyIconData);
}

void UpdateTrayIcon() {
    extern DeviceDiscovery& GetDeviceDiscovery();
    DeviceDiscovery& discovery = GetDeviceDiscovery();
    
    int batteryLevel = 0;
    bool isCharging = false;
    bool isOnline = false;
    bool isUpdating = false;

    const auto& devices = discovery.GetDiscoveredDevices();
    if (!devices.empty()) {
        const auto& device = devices[0];
        
        isOnline = discovery.IsDeviceOnline(device.devicePath);
        
        auto status = discovery.GetBatteryStatus(device.devicePath);
        
        if (status.level > 0) {
            batteryLevel = status.level;
        } else if (status.BatVoltage > 0) {
            batteryLevel = discovery.calculateBatteryPercentage(status.BatVoltage);
        }
        
        isCharging = (status.isCharging != 0);
        
        const auto& mouseList = UIRenderer::GetMouseList();
        for (const auto& mouse : mouseList) {
            if (mouse.isUpdating) {
                isUpdating = true;
                break;
            }
        }
    } else {
        const auto& mouseList = UIRenderer::GetMouseList();
        for (const auto& mouse : mouseList) {
            if (mouse.isUpdating) {
                isUpdating = true;
                break;
            }
        }
    }

    HICON newIcon = TrayIconRenderer::CreateBatteryIcon(batteryLevel, isCharging, isOnline, isUpdating);

    if (newIcon) {
        if (g_notifyIconData.hIcon) {
            DestroyIcon(g_notifyIconData.hIcon);
        }

        g_notifyIconData.hIcon = newIcon;

        if (isUpdating) {
            wcscpy_s(g_notifyIconData.szTip, L"Monka M1 Pro - Updating...");
        } else if (!isOnline) {
            wcscpy_s(g_notifyIconData.szTip, L"Monka M1 Pro - Offline");
        } else {
            std::wstring tooltip = L"Monka M1 Pro - " + std::to_wstring(batteryLevel) + L"%";
            if (isCharging) {
                tooltip += L" (Charging)";
            }
            wcscpy_s(g_notifyIconData.szTip, tooltip.c_str());
        }

        Shell_NotifyIcon(NIM_MODIFY, &g_notifyIconData);
    }
}

void ShowContextMenu(HWND hWnd, POINT pt) {
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    if (g_isWindowVisible) {
        AppendMenuW(hMenu, MF_STRING, ID_SHOW_WINDOW, L"Hide Window");
    } else {
        AppendMenuW(hMenu, MF_STRING, ID_SHOW_WINDOW, L"Show Window");
    }

    UINT startupFlags = MF_STRING;
    if (IsLaunchAtStartupEnabled()) {
        startupFlags |= MF_CHECKED;
    }
    AppendMenuW(hMenu, startupFlags, ID_LAUNCH_AT_STARTUP, L"Launch at startup");

    UINT notificationFlags = MF_STRING;
    if (IsNotificationsEnabled()) {
        notificationFlags |= MF_CHECKED;
    }
    AppendMenuW(hMenu, notificationFlags, ID_ENABLE_NOTIFICATIONS, L"Enable Notifications");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, IDM_ABOUT, L"About");
    AppendMenuW(hMenu, MF_STRING, ID_EXIT, L"Exit");

    SetForegroundWindow(hWnd);

    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_RIGHTALIGN,
                   pt.x, pt.y, 0, hWnd, NULL);

    DestroyMenu(hMenu);
}

void ShowHideWindow(HWND hWnd) {
    if (g_isWindowVisible) {
        MinimizeToTray(hWnd);
    } else {
        ShowWindow(hWnd, SW_RESTORE);
        SetForegroundWindow(hWnd);
        g_isWindowVisible = true;
    }
}

void MinimizeToTray(HWND hWnd) {
    ShowWindow(hWnd, SW_HIDE);
    g_isWindowVisible = false;
}

INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_CTLCOLORSTATIC:
        {
            HWND hCtrl = (HWND)lParam;
            if (GetDlgCtrlID(hCtrl) == IDC_ABOUT_GITHUB) { // GitHub link
                HDC hdc = (HDC)wParam;
                SetTextColor(hdc, RGB(0, 102, 204)); // Blue color
                SetBkMode(hdc, TRANSPARENT);
                return (INT_PTR)GetStockObject(NULL_BRUSH);
            }
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        // Handle GitHub link click
        if (LOWORD(wParam) == IDC_ABOUT_GITHUB && HIWORD(wParam) == STN_CLICKED) {
            ShellExecuteW(NULL, L"open", L"https://github.com/eloraa", NULL, NULL, SW_SHOWNORMAL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}