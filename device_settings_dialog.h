#pragma once

#include <windows.h>
#include "resource.h"
#include "mouse_item.h"

struct DeviceSettings {
    bool isThemeColored;        // true = COLORED, false = FLAT
    int selectedColor;          // 0 = PRIMARY, 1 = WARNING, 2 = CRITICAL
    bool batteryNotifications;  // TOGGLE
};

class DeviceSettingsDialog {
private:
    HWND hWnd;
    const MouseItem* deviceInfo;
    DeviceSettings settings;

    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL OnInitDialog(HWND hDlg);
    BOOL OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);
    void OnOK(HWND hDlg);
    void OnCancel(HWND hDlg);

    void UpdateDeviceInfo(HWND hDlg);
    void LoadSettings(HWND hDlg);
    void SaveSettings(HWND hDlg);

public:
    DeviceSettingsDialog(const MouseItem* device);
    ~DeviceSettingsDialog();

    INT_PTR ShowDialog(HWND hParent);

    const DeviceSettings& GetSettings() const { return settings; }
};