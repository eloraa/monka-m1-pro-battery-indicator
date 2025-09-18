#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include "device_discovery.h"

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

struct ConnectionInfo {
    ConnectionType type;
    std::string devicePath;
    bool isOnline;
    int batteryLevel;
    bool isCharging;
    std::string vid;
    std::string pid;

    ConnectionInfo(ConnectionType type, const std::string& devicePath, bool isOnline = true,
                   int batteryLevel = 0, bool isCharging = false, const std::string& vid = "", const std::string& pid = "")
        : type(type), devicePath(devicePath), isOnline(isOnline), batteryLevel(batteryLevel),
          isCharging(isCharging), vid(vid), pid(pid) {}
};

struct MouseItem {
    std::wstring name;
    std::wstring imagePath;
    int batteryLevel;
    bool isCharging;
    Color mouseColor;
    bool isHovered;
    float hoverProgress;
    bool isOnline; 
    bool isUpdating; 

    std::vector<ConnectionInfo> connections;

    MouseItem(const std::wstring& name, const std::wstring& imagePath, int batteryLevel, bool isCharging = false, Color mouseColor = Color(255, 255, 255, 255), bool isHovered = false, bool isOnline = true)
        : name(name), imagePath(imagePath), batteryLevel(batteryLevel), isCharging(isCharging), mouseColor(mouseColor), isHovered(isHovered), hoverProgress(0.0f), isOnline(isOnline), isUpdating(false) {}

    MouseItem(const std::wstring& name, const std::wstring& imagePath, const std::vector<ConnectionInfo>& connections)
        : name(name), imagePath(imagePath), connections(connections), isHovered(false), hoverProgress(0.0f), isUpdating(false) {
        UpdateStatusFromConnections();
    }

    void UpdateStatusFromConnections() {
        isOnline = false;
        batteryLevel = 0;
        isCharging = false;

        const ConnectionInfo* bestConnection = nullptr;

        for (const auto& conn : connections) {
            if (conn.isOnline) {
                isOnline = true;
                if (!bestConnection ||
                    (conn.batteryLevel > bestConnection->batteryLevel) ||
                    (conn.isCharging && !bestConnection->isCharging)) {
                    bestConnection = &conn;
                }
            }
        }

        if (!bestConnection && !connections.empty()) {
            bestConnection = &connections[0];
        }

        if (bestConnection) {
            batteryLevel = bestConnection->batteryLevel;
            isCharging = bestConnection->isCharging;
        }

        if (!connections.empty()) {
            mouseColor = GetConnectionColor(connections[0].type);
        } else {
            mouseColor = Color(255, 255, 255, 255); // WHITE
        }
    }

    Color GetConnectionColor(ConnectionType type) const;

    bool HasConnectionType(ConnectionType type) const {
        for (const auto& conn : connections) {
            if (conn.type == type) return true;
        }
        return false;
    }

    const ConnectionInfo* GetConnection(ConnectionType type) const {
        for (const auto& conn : connections) {
            if (conn.type == type) return &conn;
        }
        return nullptr;
    }

    std::vector<ConnectionType> GetOnlineConnectionTypes() const {
        std::vector<ConnectionType> types;
        for (const auto& conn : connections) {
            if (conn.isOnline) {
                types.push_back(conn.type);
            }
        }
        return types;
    }
};

class MouseItemRenderer {
public:
    static void RenderMouseItem(Graphics* g, const MouseItem& item, int x, int y, int width, int height);
    
    static Size GetRecommendedSize();
    
    static Image* LoadMouseImage(const std::wstring& imagePath);
    
private:
    static void RenderBatteryIcon(Graphics* g, int x, int y, int size, int batteryLevel, bool isCharging, Color color);
    
    static Color GetBatteryColor(int batteryLevel);
};
