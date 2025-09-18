#include "device_discovery.h"
#include "mouse_item.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";

bool DeviceDiscovery::is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string DeviceDiscovery::base64_decode(const std::string& encoded_string) {
    int in_len = static_cast<int>(encoded_string.size());
    int i = 0;
    int j = 0;
    int in = 0;
    unsigned char char_array_4[4], char_array_3[3];

    std::string ret;

    while (in_len-- && (encoded_string[in] != '=') && is_base64(encoded_string[in])) {
        char_array_4[i++] = encoded_string[in];
        in++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = static_cast<unsigned char>(base64_chars.find(char_array_4[i]));

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;

        for (j = 0; j < 4; j++)
            char_array_4[j] = static_cast<unsigned char>(base64_chars.find(char_array_4[j]));

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; (j < i - 1); j++)
            ret += char_array_3[j];
    }

    return ret;
}

bool DeviceDiscovery::extractResourceToDisk(int resourceId, const std::wstring& outputPath) {
    HMODULE hModule = GetModuleHandle(NULL);
    if (!hModule) return false;

    HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hRes) return false;

    HGLOBAL hMem = LoadResource(hModule, hRes);
    if (!hMem) return false;

    DWORD resourceSize = SizeofResource(hModule, hRes);
    void* resourceData = LockResource(hMem);
    if (!resourceData) return false;

    HANDLE hFile = CreateFileW(outputPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD bytesWritten;
    BOOL writeResult = WriteFile(hFile, resourceData, resourceSize, &bytesWritten, NULL);
    CloseHandle(hFile);

    return writeResult && (bytesWritten == resourceSize);
}

std::string DeviceDiscovery::loadResourceAsString(int resourceId) {
    HMODULE hModule = GetModuleHandle(NULL);
    if (!hModule) return "";

    HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hRes) return "";

    HGLOBAL hMem = LoadResource(hModule, hRes);
    if (!hMem) return "";

    DWORD resourceSize = SizeofResource(hModule, hRes);
    void* resourceData = LockResource(hMem);
    if (!resourceData) return "";

    return std::string(static_cast<const char*>(resourceData), resourceSize);
}

DeviceDiscovery::DeviceDiscovery() : hidUsbDll(nullptr), batteryUpdateCallback(nullptr) {
    instance = this;

    CS_GetDeviceBatteryStatus = nullptr;
    CS_UsbServer_ReadBatteryLevel = nullptr;
    CS_UsbServer_Start = nullptr;
    CS_UsbServer_Exit = nullptr;
    CS_UsbFinder_FindHidDevicesByDeviceId = nullptr;
    CS_UsbFinder_GetDeviceOnLine = nullptr;
    CS_UsbFinder_GetDeviceOnLineWithAddress = nullptr;
}

DeviceDiscovery::~DeviceDiscovery() {
    Cleanup();
}

bool DeviceDiscovery::Initialize() {
    if (initialized.load()) return true;

    CoInitialize(NULL);

    if (!loadMonkaConfig()) {
        std::cerr << "Failed to load Monka configuration" << std::endl;
        return false;
    }

    if (!loadHidUsbDll()) {
        std::cerr << "Failed to load HID USB DLL - using mock data" << std::endl;
        usingMockData = true;
        initialized = true;
        return true; 
    }

    initialized = true;
    return true;
}

void DeviceDiscovery::Cleanup() {
    if (hidUsbDll) {
        if (CS_UsbServer_Exit) {
            CS_UsbServer_Exit();
        }
        FreeLibrary(hidUsbDll);
        hidUsbDll = nullptr;
    }

    discoveredDevices.clear();
    lastBatteryUpdate.clear();
    deviceBatteryStatus.clear();
    initialized = false;

    CoUninitialize();
}

bool DeviceDiscovery::loadMonkaConfig() {
    bool configLoaded = false;

    std::ifstream configFile("Config.ini");
    if (configFile.is_open()) {
        configLoaded = parseConfigFromFile(configFile);
        configFile.close();
    }

    if (!configLoaded) {
        std::string configContent = loadResourceAsString(IDR_CONFIG_INI);
        if (!configContent.empty()) {
            std::istringstream configStream(configContent);
            configLoaded = parseConfigFromStream(configStream);
        }
    }

    // DEFAULTS
    if (!configLoaded) {
        config.company = "Monka";
        config.vid = "3554"; // MONKA VID
        config.m_pids = {"b00e", "b00f"}; // MOUSE PIDs
        config.d_pids = {"b012", "b013"}; // DONGLE PIDs
        config.interface_id = 0;
        config.device_id = 0;
    }

    return true;
}

bool DeviceDiscovery::parseConfigFromFile(std::ifstream& configFile) {
    config.company = "Monka";
    std::string line;
    bool inOptionSection = false;
    bool inDevice1Section = false;

    while (std::getline(configFile, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty() || line[0] == '#') continue;

        if (line == "[Option]") {
            inOptionSection = true;
            continue;
        } else if (line[0] == '[') {
            inOptionSection = false;
            continue;
        }

        if (!inOptionSection) continue;

        size_t equalPos = line.find('=');
        if (equalPos == std::string::npos) continue;

        std::string key = line.substr(0, equalPos);
        std::string value = line.substr(equalPos + 1);

        try {
            if (key == "VID") {
                std::string decoded = base64_decode(value);
                size_t pos = decoded.find(config.company);
                if (pos != std::string::npos) {
                    decoded.replace(pos, config.company.length(), "");
                }
                config.vid = decoded;
            } else if (key == "M_PID") {
                std::string decoded = base64_decode(value);
                size_t pos = decoded.find(config.company);
                if (pos != std::string::npos) {
                    decoded.replace(pos, config.company.length(), "");
                }
                std::stringstream ss(decoded);
                std::string pid;
                while (std::getline(ss, pid, ',')) {
                    if (!pid.empty()) {
                        config.m_pids.push_back(pid);
                    }
                }
            } else if (key == "D_PID") {
                std::string decoded = base64_decode(value);
                size_t pos = decoded.find(config.company);
                if (pos != std::string::npos) {
                    decoded.replace(pos, config.company.length(), "");
                }
                std::stringstream ss(decoded);
                std::string pid;
                while (std::getline(ss, pid, ',')) {
                    if (!pid.empty()) {
                        config.d_pids.push_back(pid);
                    }
                }
            } else if (key == "Interfaceid") {
                config.interface_id = std::stoi(value);
            } else if (key == "Deviceid") {
                config.device_id = std::stoi(value);
            } else if (inDevice1Section && key == "BatteryParam") {
                std::stringstream ss(value);
                std::string voltage;
                config.batteryParam.clear();
                while (std::getline(ss, voltage, ',')) {
                    if (!voltage.empty()) {
                        config.batteryParam.push_back(std::stoi(voltage));
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing config key " << key << ": " << e.what() << std::endl;
        }
    }

    configFile.close();

    return !config.vid.empty() && !config.company.empty();
}

bool DeviceDiscovery::parseConfigFromStream(std::istringstream& configStream) {
    config.company = "Monka";
    std::string line;
    bool inOptionSection = false;
    bool inDevice1Section = false;

    while (std::getline(configStream, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty() || line[0] == '#') continue;

        if (line == "[Option]") {
            inOptionSection = true;
            inDevice1Section = false;
            continue;
        } else if (line == "[Device1]") {
            inOptionSection = false;
            inDevice1Section = true;
            continue;
        } else if (line[0] == '[') {
            inOptionSection = false;
            inDevice1Section = false;
            continue;
        }

        if (!inOptionSection && !inDevice1Section) continue;

        size_t equalPos = line.find('=');
        if (equalPos == std::string::npos) continue;

        std::string key = line.substr(0, equalPos);
        std::string value = line.substr(equalPos + 1);

        try {
            if (inOptionSection) {
                if (key == "VID") {
                    std::string decoded = base64_decode(value);
                    size_t pos = decoded.find(config.company);
                    if (pos != std::string::npos) {
                        decoded.replace(pos, config.company.length(), "");
                    }
                    config.vid = decoded;
                } else if (key == "M_PID") {
                    std::string decoded = base64_decode(value);
                    size_t pos = decoded.find(config.company);
                    if (pos != std::string::npos) {
                        decoded.replace(pos, config.company.length(), "");
                    }
                    std::stringstream ss(decoded);
                    std::string pid;
                    while (std::getline(ss, pid, ',')) {
                        if (!pid.empty()) {
                            config.m_pids.push_back(pid);
                        }
                    }
                } else if (key == "D_PID") {
                    std::string decoded = base64_decode(value);
                    size_t pos = decoded.find(config.company);
                    if (pos != std::string::npos) {
                        decoded.replace(pos, config.company.length(), "");
                    }
                    std::stringstream ss(decoded);
                    std::string pid;
                    while (std::getline(ss, pid, ',')) {
                        if (!pid.empty()) {
                            config.d_pids.push_back(pid);
                        }
                    }
                } else if (key == "Interfaceid") {
                    config.interface_id = std::stoi(value);
                } else if (key == "Deviceid") {
                    config.device_id = std::stoi(value);
                }
            } else if (inDevice1Section && key == "BatteryParam") {
                std::stringstream ss(value);
                std::string voltage;
                config.batteryParam.clear();
                while (std::getline(ss, voltage, ',')) {
                    if (!voltage.empty()) {
                        config.batteryParam.push_back(std::stoi(voltage));
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing config key " << key << ": " << e.what() << std::endl;
        }
    }

    return !config.vid.empty() && !config.company.empty();
}

bool DeviceDiscovery::loadHidUsbDll() {
    std::wstring tempDllPath = L"hidusb.dll";
    if (extractResourceToDisk(IDR_HIDUSB_DLL, tempDllPath)) {
        hidUsbDll = LoadLibraryA("hidusb.dll");
    }

    if (!hidUsbDll) {
        hidUsbDll = LoadLibraryA("hidusb.dll");
    }

    if (!hidUsbDll) {
        return false;
    }

    CS_GetDeviceBatteryStatus = (CS_GetDeviceBatteryStatusFunc)GetProcAddress(hidUsbDll, "CS_GetDeviceBatteryStatus");
    CS_UsbServer_ReadBatteryLevel = (CS_UsbServer_ReadBatteryLevelFunc)GetProcAddress(hidUsbDll, "CS_UsbServer_ReadBatteryLevel");
    CS_UsbServer_Start = (CS_UsbServer_StartFunc)GetProcAddress(hidUsbDll, "CS_UsbServer_Start");
    CS_UsbServer_Exit = (CS_UsbServer_ExitFunc)GetProcAddress(hidUsbDll, "CS_UsbServer_Exit");
    CS_UsbFinder_FindHidDevicesByDeviceId = (CS_UsbFinder_FindHidDevicesByDeviceIdFunc)GetProcAddress(hidUsbDll, "CS_UsbFinder_FindHidDevicesByDeviceId");
    CS_UsbFinder_GetDeviceOnLine = (CS_UsbFinder_GetDeviceOnLineFunc)GetProcAddress(hidUsbDll, "CS_UsbFinder_GetDeviceOnLine");
    CS_UsbFinder_GetDeviceOnLineWithAddress = (CS_UsbFinder_GetDeviceOnLineWithAddressFunc)GetProcAddress(hidUsbDll, "CS_UsbFinder_GetDeviceOnLineWithAddress");

    return (CS_UsbFinder_FindHidDevicesByDeviceId != nullptr);
}

std::vector<std::string> DeviceDiscovery::getMonkaVIDPIDCombinations() {
    std::vector<std::string> combinations;

    for (const auto& pid : config.m_pids) {
        combinations.push_back(config.vid + ":" + pid);
    }

    for (const auto& pid : config.d_pids) {
        combinations.push_back(config.vid + ":" + pid);
    }

    return combinations;
}

bool DeviceDiscovery::isMousePID(const std::string& pid) {
    return std::find(config.m_pids.begin(), config.m_pids.end(), pid) != config.m_pids.end();
}

bool DeviceDiscovery::isDonglePID(const std::string& pid) {
    return std::find(config.d_pids.begin(), config.d_pids.end(), pid) != config.d_pids.end();
}

ConnectionType DeviceDiscovery::determineConnectionType(const std::string& pid, const std::string& devicePath) {
    if (isDonglePID(pid)) {
        return ConnectionType::WIRELESS_DONGLE;
    } else if (isMousePID(pid)) {
        std::string lowerPath = devicePath;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);

        if (lowerPath.find("usb") != std::string::npos) {
            return ConnectionType::USB_WIRED;
        } else {
            return ConnectionType::BLUETOOTH;
        }
    }
    return ConnectionType::UNKNOWN;
}

std::string DeviceDiscovery::getDeviceNameFromPID(const std::string& pid) {
    static std::map<std::string, std::string> pidToName = {
        {"b00e", "Monka M1 Pro"},
        {"b00f", "Monka M2 Pro"},
        {"b010", "Monka M3 Pro"},
        {"b012", "Monka Dongle"},
        {"b013", "Monka Dongle Pro"}
    };

    auto it = pidToName.find(pid);
    if (it != pidToName.end()) {
        return it->second;
    }
    return "Monka Device";
}

std::string DeviceDiscovery::getDeviceImagePath(const std::string& pid, ConnectionType connectionType) {
    return "assets/pngs/mouse/m1_pro.png";
}

int DeviceDiscovery::calculateBatteryPercentage(uint16_t voltage) {
    static const int BAT_VOLTAGE_TABLE[21] = {
        3050, 3420, 3480, 3540, 3600, 3660, 3720, 3760, 3800, 3840,
        3880, 3920, 3940, 3960, 3980, 4000, 4020, 4040, 4060, 4080, 4110
    };

    if (voltage >= BAT_VOLTAGE_TABLE[20]) {
        return 100;
    }

    for (int i = 0; i < 20; i++) {
        if (voltage < BAT_VOLTAGE_TABLE[i]) {
            if (i == 0) return 0;

            int prevVoltage = BAT_VOLTAGE_TABLE[i-1];
            int nextVoltage = BAT_VOLTAGE_TABLE[i];
            int prevPercent = (i-1) * 5;
            int nextPercent = i * 5;

            int percentage = prevPercent + ((voltage - prevVoltage) * (nextPercent - prevPercent)) / (nextVoltage - prevVoltage);

            return (percentage < 0) ? 0 : ((percentage > 100) ? 100 : percentage);
        }
    }

    return 0;
}

bool DeviceDiscovery::DiscoverDevices() {
    discoveredDevices.clear();

    if (!hidUsbDll || !CS_UsbFinder_FindHidDevicesByDeviceId) {
        std::cerr << "HID USB DLL not loaded or device discovery function not available" << std::endl;
        return false; 
    }

    auto combinations = getMonkaVIDPIDCombinations();

    for (const auto& combination : combinations) {
        size_t colonPos = combination.find(':');
        if (colonPos == std::string::npos) continue;

        std::string vid = combination.substr(0, colonPos);
        std::string pid = combination.substr(colonPos + 1);

        try {
            char vidStr[64], pidStr[64];
            strncpy_s(vidStr, vid.c_str(), sizeof(vidStr) - 1);
            strncpy_s(pidStr, pid.c_str(), sizeof(pidStr) - 1);

            SAFEARRAY* deviceArray = CS_UsbFinder_FindHidDevicesByDeviceId(
                vidStr, pidStr, config.interface_id, config.device_id);

            if (deviceArray) {
                LONG lbound, ubound;
                if (SUCCEEDED(SafeArrayGetLBound(deviceArray, 1, &lbound)) &&
                    SUCCEEDED(SafeArrayGetUBound(deviceArray, 1, &ubound))) {

                    for (LONG i = lbound; i <= ubound; i++) {
                        BSTR devicePath;
                        if (SUCCEEDED(SafeArrayGetElement(deviceArray, &i, &devicePath))) {
                            std::wstring wPath(devicePath);
                            std::string pathStr(wPath.begin(), wPath.end());

                            DeviceInfo device;
                            device.devicePath = pathStr;
                            device.vid = vid;
                            device.pid = pid;
                            device.name = getDeviceNameFromPID(pid);
                            device.imagePath = getDeviceImagePath(pid, device.connectionType);
                            device.connectionType = determineConnectionType(pid, pathStr);
                            device.batteryLevel = 0;
                            device.isCharging = false;

                            device.isOnline = false;
                            if (CS_UsbFinder_GetDeviceOnLine) {
                                device.isOnline = CS_UsbFinder_GetDeviceOnLine(devicePath);
                            }

                            discoveredDevices.push_back(device);
                            SysFreeString(devicePath);
                        }
                    }
                }
                SafeArrayDestroy(deviceArray);
            }
        } catch (...) {
        }
    }

    return !discoveredDevices.empty();
}

std::vector<MouseItem> DeviceDiscovery::GetMouseItems() {
    std::vector<MouseItem> mouseItems;
    std::map<std::string, std::vector<DeviceInfo>> devicesByName;

    for (const auto& device : discoveredDevices) {
        devicesByName[device.name].push_back(device);
    }

    for (auto it = devicesByName.begin(); it != devicesByName.end(); ++it) {
        const std::string& deviceName = it->first;
        const std::vector<DeviceInfo>& devices = it->second;

        std::vector<ConnectionInfo> connections;

        for (const auto& device : devices) {
            ConnectionInfo connInfo(
                device.connectionType,
                device.devicePath,
                device.isOnline,
                device.batteryLevel,
                device.isCharging,
                device.vid,
                device.pid
            );
            connections.push_back(connInfo);
        }

        if (!devices.empty()) {
            std::wstring wName(deviceName.begin(), deviceName.end());
            std::wstring wImagePath(devices[0].imagePath.begin(), devices[0].imagePath.end());

            MouseItem item(wName, wImagePath, connections);
            mouseItems.push_back(item);
        }
    }

    return mouseItems;
}

bool DeviceDiscovery::IsDeviceOnline(const std::string& devicePath) {
    auto it = lastBatteryUpdate.find(devicePath);
    if (it == lastBatteryUpdate.end()) {
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    auto timeSinceUpdate = std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count();
    return timeSinceUpdate < 10;
}

BatteryStatus DeviceDiscovery::GetBatteryStatus(const std::string& devicePath) {
    auto it = deviceBatteryStatus.find(devicePath);
    if (it != deviceBatteryStatus.end()) {
        return it->second;
    }
    return {0, 0, 0};
}

bool DeviceDiscovery::StartBatteryMonitoring(const std::string& devicePath) {
    if (!hidUsbDll || !CS_UsbServer_Start) {
        return false; 
    }

    char devicePathBuffer[512];
    strncpy_s(devicePathBuffer, devicePath.c_str(), sizeof(devicePathBuffer) - 1);

    CS_UsbServer_Start(devicePathBuffer, devicePathBuffer, reinterpret_cast<void*>(usbDataReceivedCallback));

    return true;
}

void DeviceDiscovery::StopBatteryMonitoring() {
    if (hidUsbDll && CS_UsbServer_Exit) {
        CS_UsbServer_Exit();
    }
}

void DeviceDiscovery::RequestBatteryLevel(const std::string& devicePath) {
    if (!hidUsbDll || !CS_UsbServer_ReadBatteryLevel) {
        return; 
    }

    CS_UsbServer_ReadBatteryLevel();

    lastBatteryUpdate[devicePath] = std::chrono::steady_clock::now();
}

DeviceDiscovery* DeviceDiscovery::instance = nullptr;

void __cdecl DeviceDiscovery::usbDataReceivedCallback(void* pcmd, int cmdLength, void* pdata, int dataLength) {
    if (instance) {
        instance->handleUsbData(pcmd, cmdLength, pdata, dataLength);
    }
}

void DeviceDiscovery::handleUsbData(void* pcmd, int cmdLength, void* pdata, int dataLength) {
    if (cmdLength < 2) return;

    uint8_t* cmdBytes = static_cast<uint8_t*>(pcmd);
    uint8_t commandId = cmdBytes[1];

    if (commandId == 4 && pdata && dataLength > 0) {
        if (!discoveredDevices.empty()) {
            processBatteryData(static_cast<uint8_t*>(pdata), dataLength, discoveredDevices[0].devicePath);
        }
    }
}

void DeviceDiscovery::processBatteryData(uint8_t* data, int dataLength, const std::string& devicePath) {
    BatteryStatus status = {0, 0, 0};

    if (CS_GetDeviceBatteryStatus) {
        CS_GetDeviceBatteryStatus(data, &status);
    } else {
        if (dataLength >= 4) {
            status.level = data[0];
            status.isCharging = data[1];
            status.BatVoltage = (data[3] << 8) | data[2];
        }
    }

    deviceBatteryStatus[devicePath] = status;
    lastBatteryUpdate[devicePath] = std::chrono::steady_clock::now();

    if (batteryUpdateCallback) {
        batteryUpdateCallback(devicePath, status);
    }
}

void DeviceDiscovery::SetBatteryUpdateCallback(BatteryUpdateCallback callback) {
    batteryUpdateCallback = callback;
}

static DeviceDiscovery g_deviceDiscovery;

DeviceDiscovery& GetDeviceDiscovery() {
    return g_deviceDiscovery;
}