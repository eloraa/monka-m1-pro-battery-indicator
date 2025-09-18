#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <chrono>
#include <functional>
#include "resource.h"

struct MouseItem;

enum class ConnectionType {
    USB_WIRED,
    WIRELESS_DONGLE,
    BLUETOOTH,
    UNKNOWN
};

struct DeviceInfo {
    std::string name;
    std::string imagePath;
    int batteryLevel;
    bool isCharging;
    bool isOnline;
    ConnectionType connectionType;
    std::string devicePath;
    std::string vid;
    std::string pid;
    std::vector<uint8_t> deviceAddress;
};

struct MonkaConfig {
    std::string vid;
    std::vector<std::string> m_pids;
    std::vector<std::string> d_pids;
    int interface_id;
    int device_id;
    std::string company;
    std::vector<int> batteryParam; 
};

struct BatteryStatus {
    uint8_t level;
    uint8_t isCharging;
    uint16_t BatVoltage;
};

typedef void(*BatteryUpdateCallback)(const std::string& devicePath, const BatteryStatus& status);

class DeviceDiscovery {
private:
    HMODULE hidUsbDll;
    MonkaConfig config;
    std::vector<DeviceInfo> discoveredDevices;
    std::atomic<bool> initialized{false};
    std::atomic<bool> usingMockData{false};

    typedef bool(__cdecl *CS_GetDeviceBatteryStatusFunc)(void* data, BatteryStatus* status);
    typedef void(__cdecl *CS_UsbServer_ReadBatteryLevelFunc)();
    typedef void(__cdecl *CS_UsbServer_StartFunc)(char* devicePath1, char* devicePath2, void* callback);
    typedef void(__cdecl *CS_UsbServer_ExitFunc)();
    typedef SAFEARRAY*(__cdecl *CS_UsbFinder_FindHidDevicesByDeviceIdFunc)(char* vid, char* pid, int interfaceId, int deviceId);
    typedef bool(__cdecl *CS_UsbFinder_GetDeviceOnLineFunc)(void* endpoint);
    typedef SAFEARRAY*(__cdecl *CS_UsbFinder_GetDeviceOnLineWithAddressFunc)(void* endpoint);

    CS_GetDeviceBatteryStatusFunc CS_GetDeviceBatteryStatus;
    CS_UsbServer_ReadBatteryLevelFunc CS_UsbServer_ReadBatteryLevel;
    CS_UsbServer_StartFunc CS_UsbServer_Start;
    CS_UsbServer_ExitFunc CS_UsbServer_Exit;
    CS_UsbFinder_FindHidDevicesByDeviceIdFunc CS_UsbFinder_FindHidDevicesByDeviceId;
    CS_UsbFinder_GetDeviceOnLineFunc CS_UsbFinder_GetDeviceOnLine;
    CS_UsbFinder_GetDeviceOnLineWithAddressFunc CS_UsbFinder_GetDeviceOnLineWithAddress;

    std::map<std::string, std::chrono::steady_clock::time_point> lastBatteryUpdate;
    std::map<std::string, BatteryStatus> deviceBatteryStatus;
    BatteryUpdateCallback batteryUpdateCallback; 

    static DeviceDiscovery* instance;
    static void __cdecl usbDataReceivedCallback(void* pcmd, int cmdLength, void* pdata, int dataLength);

    std::string base64_decode(const std::string& encoded_string);
    bool is_base64(unsigned char c);

    bool loadMonkaConfig();
    bool parseConfigFromFile(std::ifstream& configFile);
    bool parseConfigFromStream(std::istringstream& configStream);
    bool loadHidUsbDll();

    bool extractResourceToDisk(int resourceId, const std::wstring& outputPath);
    std::string loadResourceAsString(int resourceId);

    std::vector<std::string> getMonkaVIDPIDCombinations();
    ConnectionType determineConnectionType(const std::string& pid, const std::string& devicePath);
    bool isMousePID(const std::string& pid);
    bool isDonglePID(const std::string& pid);

    std::string getDeviceNameFromPID(const std::string& pid);
    std::string getDeviceImagePath(const std::string& pid, ConnectionType connectionType);

public:
    DeviceDiscovery();
    ~DeviceDiscovery();

    bool Initialize();
    void Cleanup();
    bool DiscoverDevices();
    std::vector<MouseItem> GetMouseItems();

    bool StartBatteryMonitoring(const std::string& devicePath);
    void StopBatteryMonitoring();
    void RequestBatteryLevel(const std::string& devicePath);
    void SetBatteryUpdateCallback(BatteryUpdateCallback callback);

    bool IsDeviceOnline(const std::string& devicePath);
    BatteryStatus GetBatteryStatus(const std::string& devicePath);

    int calculateBatteryPercentage(uint16_t voltage);

    void handleUsbData(void* pcmd, int cmdLength, void* pdata, int dataLength);
    void processBatteryData(uint8_t* data, int dataLength, const std::string& devicePath);

    const std::vector<DeviceInfo>& GetDiscoveredDevices() const { return discoveredDevices; }
    bool IsInitialized() const { return initialized.load(); }
    bool IsUsingMockData() const { return usingMockData; }
};

DeviceDiscovery& GetDeviceDiscovery();