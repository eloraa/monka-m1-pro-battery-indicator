#include "ui_renderer.h"
#include "font_loader.h"
#include "device_discovery.h"
#include "colors.h"
#include "color_picker_dialog.h"

ULONG_PTR UIRenderer::gdiplusToken = 0;
bool UIRenderer::gdiplusInitialized = false;
Gdiplus::Font *UIRenderer::interFont = nullptr;
Gdiplus::Font *UIRenderer::geistMonoFont = nullptr;
std::vector<MouseItem> UIRenderer::mouseList;
bool UIRenderer::animationTimerRunning = false;
int UIRenderer::hoveredItemIndex = -1;
DWORD UIRenderer::lastHoverUpdate = 0;
bool UIRenderer::isTrackingMouse = false;
DWORD UIRenderer::lastBatteryUpdate = 0;
DWORD UIRenderer::lastDeviceDiscovery = 0;
std::map<std::string, DWORD> UIRenderer::deviceLastResponse;
std::vector<std::string> UIRenderer::activeDevicePaths;
SettingsView UIRenderer::settingsView;
HWND UIRenderer::mainWindowHandle = nullptr;

bool UIRenderer::Initialize() {
  if (gdiplusInitialized)
    return true;

  Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
  gdiplusInitialized = true;

  FontLoader::Initialize();
  FontLoader::PreloadCommonWeights();

  extern BOOL IsDarkMode();
  Colors::SetTheme(IsDarkMode() ? ThemeMode::DARK : ThemeMode::LIGHT);

  ColorPickerDialog::LoadColorSettings();

  DeviceDiscovery& discovery = GetDeviceDiscovery();
  discovery.SetBatteryUpdateCallback(OnBatteryDataReceived);

  return LoadFonts() && InitializeMouseList();
}

void UIRenderer::Cleanup() {
  interFont = nullptr;
  geistMonoFont = nullptr;

  mouseList.clear();

  FontLoader::Cleanup();

  if (gdiplusInitialized) {
    Gdiplus::GdiplusShutdown(gdiplusToken);
    gdiplusInitialized = false;
  }
}

bool UIRenderer::LoadFonts() {
  interFont = FontLoader::GetInterFont(FontWeight::Medium, 14.0f);

  geistMonoFont = FontLoader::GetGeistMonoFont(FontWeight::Medium, 14.0f);

  if (!interFont) {
    interFont = new Gdiplus::Font(L"Segoe UI", 14.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
  }

  if (!geistMonoFont) {
    geistMonoFont = new Gdiplus::Font(L"Consolas", 14.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
  }

  return true;
}

Gdiplus::Font* UIRenderer::GetInterFont() {
  return interFont;
}

Gdiplus::Font* UIRenderer::GetGeistMonoFont() {
  return geistMonoFont;
}

bool UIRenderer::InitializeMouseList() {
  mouseList.clear();

  DeviceDiscovery& discovery = GetDeviceDiscovery();
  if (!discovery.IsInitialized()) {
    if (!discovery.Initialize()) {
      mouseList.push_back(MouseItem(L"Monka M1 Pro (Mock)",
                                    L"assets/pngs/mouse/m1_pro.png", 90, false,
                                    Color(255, 255, 255, 255) // WHITE
                                    ));
      
      extern bool g_isUsingMockData;
      g_isUsingMockData = true;
      
      if (mouseList.size() == 1) {
        settingsView.Show(&mouseList[0]);
      }
      return true;
    }
  }
  
  extern bool g_isUsingMockData;
  g_isUsingMockData = discovery.IsUsingMockData();

  if (discovery.DiscoverDevices()) {
    auto newMouseItems = discovery.GetMouseItems();

    for (size_t i = 0; i < newMouseItems.size() && i < mouseList.size(); i++) {
      newMouseItems[i].isHovered = mouseList[i].isHovered;
      newMouseItems[i].hoverProgress = mouseList[i].hoverProgress;
    }

    mouseList = newMouseItems;

    const auto& devices = discovery.GetDiscoveredDevices();
    if (!devices.empty()) {
      activeDevicePaths.clear();
      deviceLastResponse.clear();

      std::string bestDevice;
      for (const auto& device : devices) {
        if (device.isOnline) {
          if (device.connectionType == ConnectionType::USB_WIRED) {
            bestDevice = device.devicePath;
            break;
          } else if (bestDevice.empty() || device.connectionType == ConnectionType::WIRELESS_DONGLE) {
            bestDevice = device.devicePath;
          }
        }
      }

      if (bestDevice.empty()) {
        bestDevice = devices[0].devicePath;
      }

      if (discovery.StartBatteryMonitoring(bestDevice)) {
        activeDevicePaths.push_back(bestDevice);
        UpdateDeviceResponseTime(bestDevice);
      }
    }

    if (mouseList.size() == 1) {
      settingsView.Show(&mouseList[0]);
    }
  }

  return true;
}

void UIRenderer::RenderUI(HDC hdc, const RECT &clientRect) {
  if (!gdiplusInitialized)
    return;

  Gdiplus::Graphics g(hdc);
  g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
  g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

  int width = clientRect.right - clientRect.left;
  int height = clientRect.bottom - clientRect.top;

  if (settingsView.IsVisible()) {
    settingsView.Render(&g, width, height);
  } else {
    Gdiplus::SolidBrush bgBrush(Colors::GetBackground());
    g.FillRectangle(&bgBrush, 0, 0, width, height);

    extern bool g_isUsingMockData;
    if (g_isUsingMockData) {
      RenderMockDataIndicator(&g, width, height);
    }

    MouseListRenderer::RenderMouseList(&g, mouseList, 8, 8, width - 16,
                                       height - 16);
  }
}

void UIRenderer::UpdateMouseHover(HWND hWnd, int mouseX, int mouseY, int width, int height) {
    DWORD currentTime = GetTickCount();
    if (currentTime - lastHoverUpdate < 33) { // ~30fps
        return;
    }
    lastHoverUpdate = currentTime;
    
    int newHoveredIndex = -1;
    
    int containerWidth = width - 18; 
    int itemWidth = (containerWidth - 20) / 2;
    int itemHeight = 220;
    int spacing = 20;
    int startX = 8;
    int startY = 20;
    int itemsPerRow = 2;
    
    for (size_t i = 0; i < mouseList.size(); i++) {
        int col = i % itemsPerRow;
        int row = i / itemsPerRow;
        
        int itemX = startX + col * (itemWidth + spacing);
        int itemY = startY + row * (itemHeight + spacing);
        
        if (mouseX >= itemX && mouseX < itemX + itemWidth && 
            mouseY >= itemY && mouseY < itemY + itemHeight) {
            newHoveredIndex = static_cast<int>(i);
            break;
        }
    }
    
    if (newHoveredIndex != hoveredItemIndex) {
        if (hoveredItemIndex >= 0 && hoveredItemIndex < static_cast<int>(mouseList.size())) {
            mouseList[hoveredItemIndex].isHovered = false;
        }
        
        if (newHoveredIndex >= 0 && newHoveredIndex < static_cast<int>(mouseList.size())) {
            mouseList[newHoveredIndex].isHovered = true;
        }
        
        hoveredItemIndex = newHoveredIndex;

        StartAnimationTimer(hWnd);

        InvalidateRect(hWnd, NULL, FALSE);
    }
}

bool UIRenderer::UpdateHoverAnimation(HWND hWnd) {
    bool needsRedraw = false;
    bool hasActiveAnimations = false;
    const float animationSpeed = 0.15f; 

    for (auto& item : mouseList) {
        float targetProgress = item.isHovered ? 1.0f : 0.0f;
        float currentProgress = item.hoverProgress;

        if (abs(targetProgress - currentProgress) > 0.01f) {
            item.hoverProgress = currentProgress + (targetProgress - currentProgress) * animationSpeed;
            needsRedraw = true;
            hasActiveAnimations = true;
        } else {
            item.hoverProgress = targetProgress;
        }
    }

    if (needsRedraw) {
        InvalidateRect(hWnd, NULL, FALSE);
    }

    return hasActiveAnimations;
}

void UIRenderer::StartAnimationTimer(HWND hWnd) {
    if (!animationTimerRunning) {
        SetTimer(hWnd, 1, 16, NULL); // 60fps TIMER
        animationTimerRunning = true;
    }
}

void UIRenderer::StopAnimationTimer(HWND hWnd) {
    if (animationTimerRunning) {
        KillTimer(hWnd, 1);
        animationTimerRunning = false;
    }
}

void UIRenderer::ClearMouseHover(HWND hWnd) {
    if (hoveredItemIndex >= 0 && hoveredItemIndex < static_cast<int>(mouseList.size())) {
        mouseList[hoveredItemIndex].isHovered = false;
        hoveredItemIndex = -1;

        StartAnimationTimer(hWnd);

        InvalidateRect(hWnd, NULL, FALSE);
    }
}

void UIRenderer::UpdateBatteryStatus(HWND hWnd) {
    DWORD currentTime = GetTickCount();

    if (currentTime - lastBatteryUpdate < 5000) {
        return;
    }
    lastBatteryUpdate = currentTime;

    ThemeMode oldTheme = Colors::GetCurrentTheme();
    UpdateThemeFromSystem();

    if (oldTheme != Colors::GetCurrentTheme()) {
        extern void ApplyWindowDecor(HWND hWnd, COLORREF borderColor);
        ApplyWindowDecor(hWnd, RGB(139, 195, 74));

        InvalidateRect(hWnd, NULL, FALSE);
    }

    DeviceDiscovery& discovery = GetDeviceDiscovery();
    if (!discovery.IsInitialized()) {
        return;
    }

    if (currentTime - lastDeviceDiscovery > 30000) {
        PerformDeviceDiscovery(hWnd);
        lastDeviceDiscovery = currentTime;
    }

    bool statusChanged = false;

    const auto& devices = discovery.GetDiscoveredDevices();
    for (const auto& device : devices) {
        discovery.RequestBatteryLevel(device.devicePath);

        bool isOnline = discovery.IsDeviceOnline(device.devicePath);

        for (auto& mouseItem : mouseList) {
            std::string deviceName(mouseItem.name.begin(), mouseItem.name.end());
            if (deviceName == device.name) {
                if (mouseItem.isOnline != isOnline) {
                    mouseItem.isOnline = isOnline;
                    statusChanged = true;
                }

                auto status = discovery.GetBatteryStatus(device.devicePath);

                int calculatedLevel = 0;
                if (status.level > 0) {
                    calculatedLevel = status.level;
                } else if (status.BatVoltage > 0) {
                    calculatedLevel = discovery.calculateBatteryPercentage(status.BatVoltage);
                }

                if (calculatedLevel != mouseItem.batteryLevel && calculatedLevel > 0) {
                    mouseItem.batteryLevel = calculatedLevel;
                    mouseItem.isCharging = (status.isCharging != 0);
                    statusChanged = true;

                    UpdateDeviceResponseTime(device.devicePath);
                }
                break;
            }
        }
    }

    if (statusChanged) {
        InvalidateRect(hWnd, NULL, FALSE);
    }
}

void UIRenderer::RefreshDeviceList(HWND hWnd) {
    InitializeMouseList();
    
    if (!settingsView.IsVisible() && mouseList.size() == 1) {
        settingsView.Show(&mouseList[0]);
    }
    
    InvalidateRect(hWnd, NULL, FALSE);
}

int UIRenderer::HandleMouseClick(HWND hWnd, int mouseX, int mouseY, int width, int height) {
    int clickedItemIndex = -1;

    int containerWidth = width - 18; 
    int itemWidth = (containerWidth - 20) / 2; 
    int itemHeight = 220;
    int spacing = 20;
    int startX = 8;
    int startY = 20;
    int itemsPerRow = 2;

    for (size_t i = 0; i < mouseList.size(); i++) {
        int col = i % itemsPerRow;
        int row = i / itemsPerRow;

        int itemX = startX + col * (itemWidth + spacing);
        int itemY = startY + row * (itemHeight + spacing);

        if (mouseX >= itemX && mouseX < itemX + itemWidth &&
            mouseY >= itemY && mouseY < itemY + itemHeight) {
            clickedItemIndex = static_cast<int>(i);
            break;
        }
    }

    return clickedItemIndex; 
}

void UIRenderer::ShowDeviceSettings(const MouseItem* device) {
    settingsView.Show(device);
}

bool UIRenderer::IsSettingsViewVisible() {
    return settingsView.IsVisible();
}

bool UIRenderer::HandleSettingsClick(int mouseX, int mouseY, int width, int height) {
    return settingsView.HandleClick(mouseX, mouseY, width, height);
}

void UIRenderer::HandleSettingsHover(int mouseX, int mouseY, int width, int height) {
    settingsView.HandleHover(mouseX, mouseY, width, height);
}

const std::vector<MouseItem>& UIRenderer::GetMouseList() {
    return mouseList;
}

void UIRenderer::UpdateThemeFromSystem() {
    extern BOOL IsDarkMode();
    ThemeMode systemTheme = IsDarkMode() ? ThemeMode::DARK : ThemeMode::LIGHT;

    if (Colors::GetCurrentTheme() != systemTheme) {
        Colors::SetTheme(systemTheme);
    }
}

void UIRenderer::CheckDeviceHealth(HWND hWnd) {
    DWORD currentTime = GetTickCount();
    const DWORD DEVICE_TIMEOUT = 5000;

    bool anyDeviceResponding = false;
    bool needsSwitching = false;

    for (const auto& devicePath : activeDevicePaths) {
        auto it = deviceLastResponse.find(devicePath);
        if (it != deviceLastResponse.end() && (currentTime - it->second) < DEVICE_TIMEOUT) {
            anyDeviceResponding = true;
            break;
        }
    }

    if (!anyDeviceResponding && !activeDevicePaths.empty()) {
        needsSwitching = true;
    }

    if (needsSwitching) {
        for (auto& mouseItem : mouseList) {
            mouseItem.isUpdating = true;
        }

        SwitchToAvailableDevice(hWnd);

        InvalidateRect(hWnd, NULL, FALSE);
    }
}

void UIRenderer::PerformDeviceDiscovery(HWND hWnd) {
    DeviceDiscovery& discovery = GetDeviceDiscovery();
    if (!discovery.IsInitialized()) {
        return;
    }

    for (auto& mouseItem : mouseList) {
        mouseItem.isUpdating = true;
    }
    InvalidateRect(hWnd, NULL, FALSE);

    if (discovery.DiscoverDevices()) {
        auto newMouseItems = discovery.GetMouseItems();

        for (size_t i = 0; i < newMouseItems.size() && i < mouseList.size(); i++) {
            newMouseItems[i].isHovered = mouseList[i].isHovered;
            newMouseItems[i].hoverProgress = mouseList[i].hoverProgress;
            newMouseItems[i].isUpdating = false;
        }

        mouseList = newMouseItems;

        SwitchToAvailableDevice(hWnd);
    } else {
        for (auto& mouseItem : mouseList) {
            mouseItem.isUpdating = false;
        }
    }

    InvalidateRect(hWnd, NULL, FALSE);
}

void UIRenderer::SwitchToAvailableDevice(HWND hWnd) {
    DeviceDiscovery& discovery = GetDeviceDiscovery();
    if (!discovery.IsInitialized()) {
        return;
    }

    const auto& devices = discovery.GetDiscoveredDevices();
    std::string bestDevice;

    for (const auto& device : devices) {
        if (device.isOnline) {
            if (device.connectionType == ConnectionType::USB_WIRED) {
                bestDevice = device.devicePath;
                break;
            } else if (bestDevice.empty() || device.connectionType == ConnectionType::WIRELESS_DONGLE) {
                bestDevice = device.devicePath;
            }
        }
    }

    if (bestDevice.empty() && !devices.empty()) {
        bestDevice = devices[0].devicePath;
    }

    if (!bestDevice.empty()) {
        discovery.StopBatteryMonitoring();
        if (discovery.StartBatteryMonitoring(bestDevice)) {
            activeDevicePaths.clear();
            activeDevicePaths.push_back(bestDevice);

            UpdateDeviceResponseTime(bestDevice);
        }
    }

    for (auto& mouseItem : mouseList) {
        mouseItem.isUpdating = false;
    }
}

void UIRenderer::UpdateDeviceResponseTime(const std::string& devicePath) {
    deviceLastResponse[devicePath] = GetTickCount();
}

void UIRenderer::SetMainWindow(HWND hWnd) {
    mainWindowHandle = hWnd;
    settingsView.SetParentWindow(hWnd);
}

void UIRenderer::OnBatteryDataReceived(const std::string& devicePath, const BatteryStatus& status) {
    bool statusChanged = false;

    for (auto& mouseItem : mouseList) {
        int newLevel = status.level;
        bool newCharging = (status.isCharging != 0);

        if (mouseItem.batteryLevel != newLevel || mouseItem.isCharging != newCharging) {
            mouseItem.batteryLevel = newLevel;
            mouseItem.isCharging = newCharging;
            mouseItem.isOnline = true;
            mouseItem.isUpdating = false; 
            statusChanged = true;

            UpdateDeviceResponseTime(devicePath);
        }
        break; 
    }

    if (statusChanged && mainWindowHandle) {
        InvalidateRect(mainWindowHandle, NULL, FALSE);
        UpdateSystemTrayIcon();
    }
}

void UIRenderer::OnDeviceChange() {
    for (auto& mouseItem : mouseList) {
        mouseItem.isUpdating = true;
    }

    if (mainWindowHandle) {
        PerformDeviceDiscovery(mainWindowHandle);
        InvalidateRect(mainWindowHandle, NULL, FALSE);
        UpdateSystemTrayIcon();
    }
}

extern void UpdateTrayIcon();

void UIRenderer::UpdateSystemTrayIcon() {
    UpdateTrayIcon();
}

void UIRenderer::RenderMockDataIndicator(Gdiplus::Graphics* g, int width, int height) {
    Gdiplus::SolidBrush overlayBrush(Gdiplus::Color(128, 255, 193, 7)); // ORANGE - (50%)
    g->FillRectangle(&overlayBrush, 0, 0, width, 40);
    
    Gdiplus::Pen borderPen(Gdiplus::Color(255, 255, 152, 0), 2.0f); // ORAANGE
    g->DrawRectangle(&borderPen, 1, 1, width - 2, 38);
    
    Gdiplus::SolidBrush iconBrush(Gdiplus::Color(255, 255, 255, 255)); // WHITE
    Gdiplus::PointF trianglePoints[3] = {
        Gdiplus::PointF(15, 10),
        Gdiplus::PointF(25, 10),
        Gdiplus::PointF(20, 25)
    };
    g->FillPolygon(&iconBrush, trianglePoints, 3);
    
    Gdiplus::Font* font = FontLoader::GetInterFont(FontWeight::Medium, 12.0f);
    if (!font) {
        font = new Gdiplus::Font(L"Segoe UI", 12.0f, Gdiplus::FontStyleBold);
    }
    
    Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 255, 255, 255)); // WHITE
    Gdiplus::StringFormat format;
    format.SetAlignment(Gdiplus::StringAlignmentNear);
    format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    
    std::wstring warningText = L"⚠️ Using Mock Data - HID USB DLL not available";
    g->DrawString(warningText.c_str(), -1, font, Gdiplus::RectF(35, 0, width - 40, 40), &format, &textBrush);
    
    if (font && font != FontLoader::GetInterFont(FontWeight::Medium, 12.0f)) {
        delete font;
    }
}
