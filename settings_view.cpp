#include "settings_view.h"
#include "battery_icon_renderer.h"
#include "color_picker_dialog.h"
#include "colors.h"
#include "font_loader.h"
#include "resource_loader.h"
#include "settings_mouse_renderer.h"
#include <commdlg.h>
#include <fstream>

bool SettingsView::s_iconModeColored = true;

SettingsView::SettingsView()
    : deviceInfo(nullptr), isVisible(false), parentHwnd(NULL),
      hoveredButton(-1), selectedTheme(1), selectedColor(0), selectedMode(0),
      showingColorDropdown(false), activeColorIndex(-1), dropdownX(0),
      dropdownY(0), hoveredDropdownItem(-1) {
  static bool settingsLoaded = false;
  if (!settingsLoaded) {
    LoadUISettings();
    settingsLoaded = true;
  }

  settings.isThemeColored = s_iconModeColored;
  settings.selectedColor = 0;
  settings.themeMode = ThemeMode::DARK;

  selectedTheme = s_iconModeColored ? 1 : 0;
}

SettingsView::~SettingsView() { HideColorDropdown(); }

void SettingsView::Show(const MouseItem *device) {
  deviceInfo = device;
  isVisible = true;

  selectedTheme = s_iconModeColored ? 1 : 0;
  settings.isThemeColored = s_iconModeColored;
  selectedColor = settings.selectedColor;
  selectedMode = (settings.themeMode == ThemeMode::DARK) ? 0 : 1;
}

void SettingsView::Hide() {
  isVisible = false;
  hoveredButton = -1;
  HideColorDropdown();
}

void SettingsView::Render(Gdiplus::Graphics *g, int width, int height) {
  if (!isVisible || !deviceInfo)
    return;

  Gdiplus::SolidBrush bgBrush(Colors::GetBackground());
  g->FillRectangle(&bgBrush, 0, 0, width, height);

  extern bool g_isUsingMockData;
  if (g_isUsingMockData) {
    RenderMockDataIndicator(g, width);
  }

  int currentY = g_isUsingMockData ? 60 : 40;

  RenderDeviceInfoSection(g, currentY, width);
  currentY += 180;

  RenderThemeSection(g, currentY, width);
  currentY += 140;

  if (selectedTheme == 1) {
    RenderColorsSection(g, currentY, width);
  }

  if (showingColorDropdown) {
    RenderColorDropdown(g);
  }
}

void SettingsView::RenderDeviceInfoSection(Gdiplus::Graphics *g, int startY,
                                           int width) {
  int imageSize = 180;
  int imageX = -8;
  int imageY = startY - 20;

  std::string imagePathStr(deviceInfo->imagePath.begin(),
                           deviceInfo->imagePath.end());
  Gdiplus::Bitmap *mouseImage = ResourceLoader::LoadPNGAsBitmap(imagePathStr);
  if (mouseImage && mouseImage->GetLastStatus() == Gdiplus::Ok) {
    g->DrawImage(mouseImage, imageX, imageY, imageSize, imageSize);
    delete mouseImage;
  } else {
    Gdiplus::SolidBrush imageBrush(Gdiplus::Color(255, 200, 200, 200));
    g->FillEllipse(&imageBrush, imageX, imageY, imageSize, imageSize);
  }

  Gdiplus::Font *nameFont = FontLoader::GetInterFont(FontWeight::Medium, 18.0f);
  if (!nameFont) {
    nameFont = new Gdiplus::Font(L"Segoe UI", 18.0f, Gdiplus::FontStyleBold,
                                 Gdiplus::UnitPixel);
  }

  Gdiplus::SolidBrush textBrush(Colors::GetForeground());
  Gdiplus::RectF nameRect(140, static_cast<Gdiplus::REAL>(startY + 30),
                          static_cast<Gdiplus::REAL>(width - 160), 40);
  g->DrawString(deviceInfo->name.c_str(), -1, nameFont, nameRect, nullptr,
                &textBrush);

  Gdiplus::Font *batteryFont =
      FontLoader::GetGeistMonoFont(FontWeight::Medium, 14.0f);
  if (!batteryFont) {
    batteryFont = new Gdiplus::Font(
        L"Consolas", 14.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
  }

  int batteryIconSize = 24;
  int batteryY = startY + 65;

  Gdiplus::Color foregroundColor = Colors::GetForeground();
  Gdiplus::Color batteryDisplayColor(
      static_cast<BYTE>(255 * 0.8), foregroundColor.GetR(),
      foregroundColor.GetG(), foregroundColor.GetB());

  int displayLevel = deviceInfo->batteryLevel;
  bool displayCharging = deviceInfo->isCharging;
  Gdiplus::Color iconColor = batteryDisplayColor;

  if (deviceInfo->isUpdating) {
    iconColor = Gdiplus::Color(static_cast<BYTE>(255 * 0.8), 255, 165, 0);
    displayLevel = 50;
    displayCharging = false;
  } else if (!deviceInfo->isOnline) {
    iconColor = Gdiplus::Color(static_cast<BYTE>(255 * 0.8), 128, 128, 128);
    displayLevel = 0;
    displayCharging = false;
  }

  BatteryIconRenderer::RenderBatteryIcon(g, 142, batteryY, batteryIconSize,
                                         displayLevel, displayCharging,
                                         iconColor);

  std::wstring batteryText;
  if (deviceInfo->isUpdating) {
    batteryText = L"UPDATING";
  } else if (!deviceInfo->isOnline) {
    batteryText = L"OFFLINE";
  } else {
    batteryText = std::to_wstring(deviceInfo->batteryLevel);
  }
  Gdiplus::SolidBrush batteryTextBrush(batteryDisplayColor);

  Gdiplus::StringFormat textFormat;
  textFormat.SetAlignment(Gdiplus::StringAlignmentNear);
  textFormat.SetLineAlignment(Gdiplus::StringAlignmentCenter);

  Gdiplus::RectF measureRect(0, 0, 100, 25);
  Gdiplus::RectF actualRect;
  g->MeasureString(batteryText.c_str(), -1, batteryFont, measureRect,
                   &textFormat, &actualRect);

  int textY =
      batteryY + (batteryIconSize - static_cast<int>(actualRect.Height)) / 2;
  Gdiplus::RectF batteryRect(170, static_cast<Gdiplus::REAL>(textY), 100,
                             static_cast<Gdiplus::REAL>(actualRect.Height));
  g->DrawString(batteryText.c_str(), -1, batteryFont, batteryRect, &textFormat,
                &batteryTextBrush);

  if (deviceInfo->isCharging) {
    Gdiplus::Font *chargingFont =
        FontLoader::GetGeistMonoFont(FontWeight::Medium, 14.0f);
    if (!chargingFont) {
      chargingFont = new Gdiplus::Font(
          L"Consolas", 14.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    }

    Gdiplus::SolidBrush chargingTextBrush(Gdiplus::Color(255, 100, 200, 100));
    std::wstring chargingText = L"CHARGING";
    int chargingY =
        batteryY + batteryIconSize + 12;
    Gdiplus::RectF chargingRect(140, static_cast<Gdiplus::REAL>(chargingY), 200,
                                20);
    g->DrawString(chargingText.c_str(), -1, chargingFont, chargingRect, nullptr,
                  &chargingTextBrush);
  }
}

void SettingsView::RenderThemeSection(Gdiplus::Graphics *g, int startY,
                                      int width) {
  int buttonSize = 80;
  int buttonSpacing = 40;
  int startX = 40;

  bool isFlatHovered = (hoveredButton == 1);
  bool isColoredHovered = (hoveredButton == 2);

  Gdiplus::Color flatBg = Colors::GetBackground();
  Gdiplus::SolidBrush flatBrush(flatBg);
  g->FillRectangle(&flatBrush, startX, startY, buttonSize, buttonSize);

  if (selectedTheme == 0) {
    Gdiplus::Color flatBorder = Gdiplus::Color(255, 70, 130, 255);
    Gdiplus::Pen flatPen(flatBorder, 2.0f);
    g->DrawRectangle(&flatPen, startX, startY, buttonSize, buttonSize);
  } else {
    Gdiplus::Color foregroundColor = Colors::GetForeground();
    Gdiplus::Color unselectedBorder(
        static_cast<BYTE>(255 * 0.15), foregroundColor.GetR(),
        foregroundColor.GetG(), foregroundColor.GetB());
    Gdiplus::Pen unselectedPen(unselectedBorder, 1.0f);
    g->DrawRectangle(&unselectedPen, startX, startY, buttonSize, buttonSize);
  }

  extern BOOL IsDarkMode();
  bool isDarkTheme = IsDarkMode();
  SettingsMouseRenderer::RenderMouseImage(g, startX + 10, startY + 10,
                                          buttonSize - 20, false, isDarkTheme);

  Gdiplus::Font *labelFont =
      FontLoader::GetInterFont(FontWeight::Medium, 12.0f);
  if (!labelFont) {
    labelFont = new Gdiplus::Font(L"Segoe UI", 12.0f, Gdiplus::FontStyleBold,
                                  Gdiplus::UnitPixel);
  }

  Gdiplus::SolidBrush textBrush(Colors::GetForeground());
  Gdiplus::StringFormat format;
  format.SetAlignment(Gdiplus::StringAlignmentCenter);

  Gdiplus::RectF flatLabelRect(
      static_cast<Gdiplus::REAL>(startX),
      static_cast<Gdiplus::REAL>(startY + buttonSize + 8),
      static_cast<Gdiplus::REAL>(buttonSize), 20);
  g->DrawString(L"FLAT", -1, labelFont, flatLabelRect, &format, &textBrush);

  int coloredX = startX + buttonSize + buttonSpacing;
  Gdiplus::Color coloredBg = Colors::GetBackground();
  Gdiplus::SolidBrush coloredBrush(coloredBg);
  g->FillRectangle(&coloredBrush, coloredX, startY, buttonSize, buttonSize);

  if (selectedTheme == 1) {
    Gdiplus::Color coloredBorder =
        Gdiplus::Color(255, 70, 130, 255);
    Gdiplus::Pen coloredPen(coloredBorder, 2.0f);
    g->DrawRectangle(&coloredPen, coloredX, startY, buttonSize, buttonSize);
  } else {
    Gdiplus::Color foregroundColor = Colors::GetForeground();
    Gdiplus::Color unselectedBorder(
        static_cast<BYTE>(255 * 0.15), foregroundColor.GetR(),
        foregroundColor.GetG(), foregroundColor.GetB());
    Gdiplus::Pen unselectedPen(unselectedBorder, 1.0f);
    g->DrawRectangle(&unselectedPen, coloredX, startY, buttonSize, buttonSize);
  }

  SettingsMouseRenderer::RenderMouseImage(g, coloredX + 10, startY + 10,
                                          buttonSize - 20, true, isDarkTheme);

  Gdiplus::RectF coloredLabelRect(
      static_cast<Gdiplus::REAL>(coloredX),
      static_cast<Gdiplus::REAL>(startY + buttonSize + 8),
      static_cast<Gdiplus::REAL>(buttonSize), 20);
  g->DrawString(L"COLORED", -1, labelFont, coloredLabelRect, &format,
                &textBrush);
}

void SettingsView::RenderColorsSection(Gdiplus::Graphics *g, int startY,
                                       int width) {
  Gdiplus::Font *sectionFont =
      FontLoader::GetInterFont(FontWeight::Medium, 14.0f);
  if (!sectionFont) {
    sectionFont = new Gdiplus::Font(L"Segoe UI", 14.0f, Gdiplus::FontStyleBold,
                                    Gdiplus::UnitPixel);
  }

  Gdiplus::SolidBrush textBrush(Colors::GetForeground());
  Gdiplus::RectF titleRect(40, static_cast<Gdiplus::REAL>(startY),
                           static_cast<Gdiplus::REAL>(width - 100), 25);
  g->DrawString(L"COLORS", -1, sectionFont, titleRect, nullptr, &textBrush);

  int swatchSize = 20;
  int itemHeight = 30;
  int itemSpacing = 15;
  int startX = 40;
  int currentY = startY + 30;

  struct ColorOption {
    std::wstring name;
    Gdiplus::Color color;
    int hoverId;
  };

  ColorOption colors[] = {{L"PRIMARY", ColorPickerDialog::GetColor(0), 3},
                          {L"SUCCESS", ColorPickerDialog::GetColor(1), 4},
                          {L"WARNING", ColorPickerDialog::GetColor(2), 5},
                          {L"CRITICAL", ColorPickerDialog::GetColor(3), 6}};

  for (int i = 0; i < 4; i++) {
    bool isHovered = (hoveredButton == colors[i].hoverId);
    bool isSelected = (selectedColor == i);

    Gdiplus::SolidBrush colorBrush(colors[i].color);
    g->FillRectangle(&colorBrush, startX + 5, currentY, swatchSize, swatchSize);

    Gdiplus::Pen swatchPen(Gdiplus::Color(255, 100, 100, 100), 1.0f);
    g->DrawRectangle(&swatchPen, startX + 5, currentY, swatchSize, swatchSize);

    Gdiplus::Font *labelFont =
        FontLoader::GetInterFont(FontWeight::Medium, 12.0f);
    if (!labelFont) {
      labelFont = new Gdiplus::Font(
          L"Segoe UI", 12.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    }

    Gdiplus::SolidBrush labelBrush(Colors::GetForeground());
    Gdiplus::RectF labelRect(
        static_cast<Gdiplus::REAL>(startX + swatchSize + 8),
        static_cast<Gdiplus::REAL>(currentY + 2), 80,
        static_cast<Gdiplus::REAL>(swatchSize));

    std::wstring displayName = colors[i].name;
    if (ColorPickerDialog::IsColorCustomized(i)) {
      displayName += L" •"; 
    }

    g->DrawString(displayName.c_str(), -1, labelFont, labelRect, nullptr,
                  &labelBrush);

    currentY += swatchSize + itemSpacing;
  }
}

void SettingsView::RenderButton(Gdiplus::Graphics *g, const std::wstring &text,
                                int x, int y, int width, int height,
                                bool isSelected, bool isHovered,
                                const Gdiplus::Color &accentColor) {
  Gdiplus::Color bgColor =
      isSelected ? accentColor : Colors::GetCardBackground();
  if (isHovered && !isSelected) {
    bgColor = Colors::GetBorderHover();
  }

  Gdiplus::SolidBrush bgBrush(bgColor);
  g->FillRectangle(&bgBrush, x, y, width, height);

  Gdiplus::Color borderColor =
      isSelected ? accentColor : Colors::GetBorderDefault();
  Gdiplus::Pen borderPen(borderColor, 1.0f);
  g->DrawRectangle(&borderPen, x, y, width, height);

  Gdiplus::Color textColor =
      isSelected ? Colors::GetBackground() : Colors::GetForeground();
  Gdiplus::Font *font = FontLoader::GetInterFont(FontWeight::Medium, 12.0f);
  if (!font) {
    font = new Gdiplus::Font(L"Segoe UI", 12.0f, Gdiplus::FontStyleRegular,
                             Gdiplus::UnitPixel);
  }

  Gdiplus::SolidBrush textBrush(textColor);
  Gdiplus::StringFormat format;
  format.SetAlignment(Gdiplus::StringAlignmentCenter);
  format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

  Gdiplus::RectF textRect(
      static_cast<Gdiplus::REAL>(x), static_cast<Gdiplus::REAL>(y),
      static_cast<Gdiplus::REAL>(width), static_cast<Gdiplus::REAL>(height));
  g->DrawString(text.c_str(), -1, font, textRect, &format, &textBrush);
}

bool SettingsView::HandleClick(int mouseX, int mouseY, int width, int height) {
  if (!isVisible)
    return false;

  if (showingColorDropdown) {
    const int dropdownWidth = 140;
    const int itemHeight = 32;
    const int dropdownHeight = itemHeight * 2;

    if (IsPointInRect(mouseX, mouseY, dropdownX, dropdownY, dropdownWidth,
                      dropdownHeight)) {
      int item = (mouseY - dropdownY) / itemHeight;
      if (item >= 0 && item < 2) {
        HandleDropdownSelection(item);
      }
      return true;
    } else {
      HideColorDropdown();
      return true;
    }
  }

  int themeY = 40 + 180;
  int buttonSize = 80;
  int buttonSpacing = 40;
  int startX = 40;

  if (IsPointInRect(mouseX, mouseY, startX, themeY, buttonSize, buttonSize)) {
    selectedTheme = 0;
    settings.isThemeColored = false;
    ApplySettings();
    return true;
  }

  int coloredX = startX + buttonSize + buttonSpacing;
  if (IsPointInRect(mouseX, mouseY, coloredX, themeY, buttonSize, buttonSize)) {
    selectedTheme = 1;
    settings.isThemeColored = true;
    ApplySettings();
    return true;
  }

  int colorsY = 40 + 180 + 140 + 30;
  int swatchSize = 20;
  int itemHeight = 30;
  int itemSpacing = 15;
  int colorStartX = 40;
  int currentY = colorsY;

  for (int i = 0; i < 4; i++) {
    if (IsPointInRect(mouseX, mouseY, colorStartX, currentY, 200, itemHeight)) {
      selectedColor = i;
      settings.selectedColor = i;

      ShowColorDropdown(i, colorStartX + 120, currentY);
      return true;
    }
    currentY += swatchSize + itemSpacing;
  }

  return false;
}

void SettingsView::HandleHover(int mouseX, int mouseY, int width, int height) {
  if (!isVisible) {
    hoveredButton = -1;
    hoveredDropdownItem = -1;
    return;
  }

  if (showingColorDropdown) {
    const int dropdownWidth = 140;
    const int itemHeight = 32;
    const int dropdownHeight = itemHeight * 2;

    if (IsPointInRect(mouseX, mouseY, dropdownX, dropdownY, dropdownWidth,
                      dropdownHeight)) {
      int item = (mouseY - dropdownY) / itemHeight;
      hoveredDropdownItem = (item >= 0 && item < 2) ? item : -1;
    } else {
      hoveredDropdownItem = -1;
    }
  } else {
    hoveredDropdownItem = -1;
  }

  hoveredButton = GetHoveredElement(mouseX, mouseY, width, height);
}

int SettingsView::GetHoveredElement(int mouseX, int mouseY, int width,
                                    int height) {
  int themeY = 40 + 180;
  int buttonSize = 80;
  int buttonSpacing = 40;
  int startX = 40;

  if (IsPointInRect(mouseX, mouseY, startX, themeY, buttonSize, buttonSize))
    return 1;

  int coloredX = startX + buttonSize + buttonSpacing;
  if (IsPointInRect(mouseX, mouseY, coloredX, themeY, buttonSize, buttonSize))
    return 2;
  int colorsY = 40 + 180 + 140 + 30;
  int swatchSize = 20;
  int itemHeight = 30;
  int itemSpacing = 15;
  int colorStartX = 40;
  int currentY = colorsY;

  for (int i = 0; i < 4; i++) {
    if (IsPointInRect(mouseX, mouseY, colorStartX, currentY, 200, itemHeight)) {
      return 3 + i; // PRIMARY=3, SUCCESS=4, WARNING=5, CRITICAL=6
    }
    currentY += swatchSize + itemSpacing;
  }

  return -1;
}

bool SettingsView::IsPointInRect(int x, int y, int rectX, int rectY, int rectW,
                                 int rectH) {
  return x >= rectX && x < rectX + rectW && y >= rectY && y < rectY + rectH;
}

void SettingsView::ApplySettings() {
  Colors::SetTheme(settings.themeMode);

  s_iconModeColored = settings.isThemeColored;

  SaveUISettings();

  extern void UpdateTrayIcon();
  UpdateTrayIcon();
}

void SettingsView::LoadUISettings() {
  std::ifstream configFile("Config.ini");
  if (!configFile.is_open()) {
    std::string configContent =
        ResourceLoader::LoadResourceAsString(IDR_CONFIG_INI);
    if (!configContent.empty()) {
      std::ofstream createFile("Config.ini");
      createFile << configContent;
      createFile.close();

      configFile.open("Config.ini");
    }
  }

  if (!configFile.is_open()) {
    s_iconModeColored = true;
    return;
  }

  std::string line;
  bool inUISettings = false;

  while (std::getline(configFile, line)) {
    line.erase(0, line.find_first_not_of(" \t"));
    line.erase(line.find_last_not_of(" \t") + 1);

    if (line == "[UISettings]") {
      inUISettings = true;
      continue;
    }

    if (line.empty() || line[0] == '[') {
      inUISettings = false;
      continue;
    }

    if (inUISettings) {
      size_t equalPos = line.find('=');
      if (equalPos != std::string::npos) {
        std::string key = line.substr(0, equalPos);
        std::string value = line.substr(equalPos + 1);

        if (key == "IconMode") {
          s_iconModeColored = (value == "COLORED");
        }
      }
    }
  }

  configFile.close();
}

void SettingsView::SaveUISettings() {
  std::ifstream checkFile("Config.ini");
  bool fileExists = checkFile.is_open();
  checkFile.close();

  if (!fileExists) {
    std::string configContent =
        ResourceLoader::LoadResourceAsString(IDR_CONFIG_INI);
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
    lines.push_back("EnableNotifications=1");
    lines.push_back("");
    lines.push_back("[UISettings]");
    lines.push_back("IconMode=" +
                    std::string(s_iconModeColored ? "COLORED" : "FLAT"));
    lines.push_back("");
    lines.push_back("[ColorSettings]");
  } else {
    bool uiSectionFound = false;
    bool iconModeUpdated = false;

    for (size_t i = 0; i < lines.size(); i++) {
      std::string trimmedLine = lines[i];
      trimmedLine.erase(0, trimmedLine.find_first_not_of(" \t"));
      trimmedLine.erase(trimmedLine.find_last_not_of(" \t") + 1);

      if (trimmedLine == "[UISettings]") {
        uiSectionFound = true;
        continue;
      }

      if (uiSectionFound && trimmedLine.find("IconMode=") == 0) {
        lines[i] =
            "IconMode=" + std::string(s_iconModeColored ? "COLORED" : "FLAT");
        iconModeUpdated = true;
        break;
      }

      if (uiSectionFound && (trimmedLine.empty() || trimmedLine[0] == '[')) {
        if (!iconModeUpdated) {
          lines.insert(lines.begin() + i,
                       "IconMode=" +
                           std::string(s_iconModeColored ? "COLORED" : "FLAT"));
        }
        break;
      }
    }

    if (!uiSectionFound) {
      if (!lines.empty() && !lines.back().empty()) {
        lines.push_back("");
      }
      lines.push_back("[UISettings]");
      lines.push_back("IconMode=" +
                      std::string(s_iconModeColored ? "COLORED" : "FLAT"));
    }
  }

  std::ofstream outFile("Config.ini");
  for (const auto &fileLine : lines) {
    outFile << fileLine << "\n";
  }
  outFile.close();
}

bool SettingsView::IsIconModeColored() { return s_iconModeColored; }

void SettingsView::SetIconModeColored(bool colored) {
  s_iconModeColored = colored;
}

void SettingsView::ShowColorDropdown(int colorIndex, int x, int y) {
  activeColorIndex = colorIndex;
  dropdownX = x;
  dropdownY = y;
  showingColorDropdown = true;
  hoveredDropdownItem = -1;
}

void SettingsView::HideColorDropdown() {
  showingColorDropdown = false;
  activeColorIndex = -1;
  hoveredDropdownItem = -1;
}

void SettingsView::HandleDropdownSelection(int selection) {
  if (activeColorIndex < 0 || activeColorIndex >= 4)
    return;

  if (selection == 0) {
    ColorPickerDialog::ResetColorToDefault(activeColorIndex);

    extern void UpdateTrayIcon();
    UpdateTrayIcon();
  } else if (selection == 1) {
    if (parentHwnd) {
      static COLORREF customColors[16] = {0};
      CHOOSECOLOR cc = {0};
      cc.lStructSize = sizeof(CHOOSECOLOR);
      cc.hwndOwner = parentHwnd;
      cc.lpCustColors = customColors;
      cc.rgbResult = RGB(ColorPickerDialog::GetColor(activeColorIndex).GetR(),
                         ColorPickerDialog::GetColor(activeColorIndex).GetG(),
                         ColorPickerDialog::GetColor(activeColorIndex).GetB());
      cc.Flags = CC_FULLOPEN | CC_RGBINIT;

      if (ChooseColor(&cc)) {
        Gdiplus::Color newColor(255, GetRValue(cc.rgbResult),
                                GetGValue(cc.rgbResult),
                                GetBValue(cc.rgbResult));

        ColorSettings settings = ColorPickerDialog::GetColorSettings();
        settings.customColors[activeColorIndex] = newColor;
        settings.useCustomColors[activeColorIndex] = true;
        ColorPickerDialog::SetColorSettings(settings);
        ColorPickerDialog::SaveColorSettings();

        extern void UpdateTrayIcon();
        UpdateTrayIcon();
      }
    }
  }

  HideColorDropdown();
}

void SettingsView::RenderColorDropdown(Gdiplus::Graphics *g) {
  if (!showingColorDropdown)
    return;

  const int dropdownWidth = 140;
  const int itemHeight = 32;
  const int dropdownHeight = itemHeight * 2;
  const int cornerRadius = 8;

  Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(40, 0, 0, 0));
  g->FillRectangle(&shadowBrush, dropdownX + 2, dropdownY + 2, dropdownWidth,
                   dropdownHeight);

  Gdiplus::Color dropdownBg = Colors::GetCardBackground();
  Gdiplus::SolidBrush bgBrush(dropdownBg);
  g->FillRectangle(&bgBrush, dropdownX, dropdownY, dropdownWidth,
                   dropdownHeight);

  Gdiplus::Color borderColor = Colors::GetBorderDefault();
  Gdiplus::Pen borderPen(borderColor, 1.0f);
  g->DrawRectangle(&borderPen, dropdownX, dropdownY, dropdownWidth - 1,
                   dropdownHeight - 1);

  for (int i = 0; i < 2; i++) {
    int itemY = dropdownY + (i * itemHeight);
    bool isHovered = (hoveredDropdownItem == i);

    if (isHovered) {
      Gdiplus::SolidBrush hoverBrush(Colors::GetBorderHover());
      g->FillRectangle(&hoverBrush, dropdownX + 1, itemY, dropdownWidth - 2,
                       itemHeight);
    }

    int swatchX = dropdownX + 8;
    int swatchY = itemY + 8;
    int swatchSize = 16;

    Gdiplus::Color swatchColor;
    std::wstring itemText;

    if (i == 0) {
      itemText = L"Default";
      switch (activeColorIndex) {
      case 0:
        swatchColor = Colors::PRIMARY;
        break;
      case 1:
        swatchColor = Colors::SUCCESS;
        break;
      case 2:
        swatchColor = Colors::WARNING;
        break;
      case 3:
        swatchColor = Colors::CRITICAL;
        break;
      default:
        swatchColor = Colors::PRIMARY;
        break;
      }
    } else {
      itemText = L"Custom...";
      swatchColor = ColorPickerDialog::GetColor(activeColorIndex);
    }

    Gdiplus::SolidBrush swatchBrush(swatchColor);
    g->FillRectangle(&swatchBrush, swatchX, swatchY, swatchSize, swatchSize);

    Gdiplus::Pen swatchPen(Colors::GetForeground(), 1.0f);
    g->DrawRectangle(&swatchPen, swatchX, swatchY, swatchSize, swatchSize);

    Gdiplus::Font *itemFont =
        FontLoader::GetInterFont(FontWeight::Medium, 12.0f);
    if (!itemFont) {
      itemFont = new Gdiplus::Font(
          L"Segoe UI", 12.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    }

    Gdiplus::SolidBrush textBrush(Colors::GetForeground());
    Gdiplus::RectF textRect(
        static_cast<Gdiplus::REAL>(swatchX + swatchSize + 8),
        static_cast<Gdiplus::REAL>(itemY),
        static_cast<Gdiplus::REAL>(dropdownWidth - swatchSize - 24),
        static_cast<Gdiplus::REAL>(itemHeight));

    Gdiplus::StringFormat format;
    format.SetAlignment(Gdiplus::StringAlignmentNear);
    format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

    g->DrawString(itemText.c_str(), -1, itemFont, textRect, &format,
                  &textBrush);

    bool isCurrentSelection =
        (i == 0) ? !ColorPickerDialog::IsColorCustomized(activeColorIndex)
                 : ColorPickerDialog::IsColorCustomized(activeColorIndex);
    if (isCurrentSelection) {
      Gdiplus::SolidBrush checkBrush(Colors::GetForeground());
      int checkX = dropdownX + dropdownWidth - 20;
      int checkY = itemY + itemHeight / 2;
      g->FillEllipse(&checkBrush, checkX, checkY - 2, 4, 4);
    }
  }
}

void SettingsView::RenderMockDataIndicator(Gdiplus::Graphics *g, int width) {
  Gdiplus::SolidBrush overlayBrush(
      Gdiplus::Color(128, 255, 193, 7)); // ORANGE (50%)
  g->FillRectangle(&overlayBrush, 0, 0, width, 30);

  Gdiplus::Pen borderPen(Gdiplus::Color(255, 255, 152, 0),
                         1.0f); // ORANGE
  g->DrawRectangle(&borderPen, 0, 0, width - 1, 29);

  Gdiplus::SolidBrush iconBrush(Gdiplus::Color(255, 255, 255, 255)); // WHITE
  Gdiplus::PointF trianglePoints[3] = {
      Gdiplus::PointF(10, 5), Gdiplus::PointF(18, 5), Gdiplus::PointF(14, 20)};
  g->FillPolygon(&iconBrush, trianglePoints, 3);

  Gdiplus::Font *font = FontLoader::GetInterFont(FontWeight::Medium, 10.0f);
  if (!font) {
    font = new Gdiplus::Font(L"Segoe UI", 10.0f, Gdiplus::FontStyleBold);
  }

  Gdiplus::SolidBrush textBrush(
      Gdiplus::Color(255, 255, 255, 255)); // WHITE
  Gdiplus::StringFormat format;
  format.SetAlignment(Gdiplus::StringAlignmentNear);
  format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

  std::wstring warningText = L"⚠️ Mock Data Mode - HID USB DLL not available";
  g->DrawString(warningText.c_str(), -1, font,
                Gdiplus::RectF(25, 0, width - 30, 30), &format, &textBrush);

  if (font && font != FontLoader::GetInterFont(FontWeight::Medium, 10.0f)) {
    delete font;
  }
}
