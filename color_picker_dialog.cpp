#include "color_picker_dialog.h"
#include "resource.h"
#include <fstream>
#include <sstream>
#include <vector>

ColorSettings ColorPickerDialog::s_colorSettings;
COLORREF ColorPickerDialog::customColors[16] = { 0 };
int ColorPickerDialog::currentColorIndex = 0;
Gdiplus::Color ColorPickerDialog::tempColor(255, 255, 255, 255);

ColorPickerDialog::ColorPickerDialog(HWND parent) : parentHwnd(parent) {
    for (int i = 0; i < 16; i++) {
        customColors[i] = RGB(255, 255, 255);
    }
}

ColorPickerDialog::~ColorPickerDialog() {
}

bool ColorPickerDialog::ShowColorPicker(int colorIndex) {
    if (colorIndex < 0 || colorIndex >= 4) {
        return false;
    }

    INT_PTR result = DialogBoxParam(
        GetModuleHandle(NULL),
        MAKEINTRESOURCE(IDD_COLOR_PICKER),
        parentHwnd,
        ColorPickerDialogProc,
        (LPARAM)colorIndex
    );

    return (result == IDOK);
}

Gdiplus::Color ColorPickerDialog::GetColor(int colorIndex) {
    if (colorIndex < 0 || colorIndex >= 4) {
        return Colors::PRIMARY; // Fallback
    }

    if (s_colorSettings.useCustomColors[colorIndex]) {
        return s_colorSettings.customColors[colorIndex];
    }

    switch (colorIndex) {
        case 0: return Colors::PRIMARY;
        case 1: return Colors::SUCCESS;
        case 2: return Colors::WARNING;
        case 3: return Colors::CRITICAL;
        default: return Colors::PRIMARY;
    }
}

void ColorPickerDialog::ResetColorToDefault(int colorIndex) {
    if (colorIndex < 0 || colorIndex >= 4) {
        return;
    }

    s_colorSettings.useCustomColors[colorIndex] = false;

    switch (colorIndex) {
        case 0: s_colorSettings.customColors[colorIndex] = Colors::PRIMARY; break;
        case 1: s_colorSettings.customColors[colorIndex] = Colors::SUCCESS; break;
        case 2: s_colorSettings.customColors[colorIndex] = Colors::WARNING; break;
        case 3: s_colorSettings.customColors[colorIndex] = Colors::CRITICAL; break;
    }

    SaveColorSettings();
}

void ColorPickerDialog::ResetAllColorsToDefault() {
    for (int i = 0; i < 4; i++) {
        ResetColorToDefault(i);
    }
}

bool ColorPickerDialog::IsColorCustomized(int colorIndex) {
    if (colorIndex < 0 || colorIndex >= 4) {
        return false;
    }
    return s_colorSettings.useCustomColors[colorIndex];
}

void ColorPickerDialog::LoadColorSettings() {
    std::ifstream configFile("Config.ini");
    if (!configFile.is_open()) {
        return;
    }

    std::string line;
    bool inColorSettings = false;

    while (std::getline(configFile, line)) {
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line == "[ColorSettings]") {
            inColorSettings = true;
            continue;
        }

        if (line.empty() || line[0] == '[') {
            inColorSettings = false;
            continue;
        }

        if (inColorSettings) {
            size_t equalPos = line.find('=');
            if (equalPos != std::string::npos) {
                std::string key = line.substr(0, equalPos);
                std::string value = line.substr(equalPos + 1);

                if (key == "CustomPrimary") {
                    s_colorSettings.useCustomColors[0] = true;
                    std::stringstream ss(value);
                    std::string token;
                    int rgb[3] = {0};
                    int i = 0;
                    while (std::getline(ss, token, ',') && i < 3) {
                        rgb[i++] = std::stoi(token);
                    }
                    s_colorSettings.customColors[0] = Gdiplus::Color(255, rgb[0], rgb[1], rgb[2]);
                }
                else if (key == "CustomSuccess") {
                    s_colorSettings.useCustomColors[1] = true;
                    std::stringstream ss(value);
                    std::string token;
                    int rgb[3] = {0};
                    int i = 0;
                    while (std::getline(ss, token, ',') && i < 3) {
                        rgb[i++] = std::stoi(token);
                    }
                    s_colorSettings.customColors[1] = Gdiplus::Color(255, rgb[0], rgb[1], rgb[2]);
                }
                else if (key == "CustomWarning") {
                    s_colorSettings.useCustomColors[2] = true;
                    std::stringstream ss(value);
                    std::string token;
                    int rgb[3] = {0};
                    int i = 0;
                    while (std::getline(ss, token, ',') && i < 3) {
                        rgb[i++] = std::stoi(token);
                    }
                    s_colorSettings.customColors[2] = Gdiplus::Color(255, rgb[0], rgb[1], rgb[2]);
                }
                else if (key == "CustomCritical") {
                    s_colorSettings.useCustomColors[3] = true;
                    std::stringstream ss(value);
                    std::string token;
                    int rgb[3] = {0};
                    int i = 0;
                    while (std::getline(ss, token, ',') && i < 3) {
                        rgb[i++] = std::stoi(token);
                    }
                    s_colorSettings.customColors[3] = Gdiplus::Color(255, rgb[0], rgb[1], rgb[2]);
                }
            }
        }
    }

    configFile.close();
}

void ColorPickerDialog::SaveColorSettings() {
    std::ifstream configFile("Config.ini");
    std::vector<std::string> lines;
    std::string line;

    while (configFile.is_open() && std::getline(configFile, line)) {
        lines.push_back(line);
    }
    configFile.close();

    bool colorSectionFound = false;
    int colorSectionEnd = -1;

    for (size_t i = 0; i < lines.size(); i++) {
        std::string trimmedLine = lines[i];
        trimmedLine.erase(0, trimmedLine.find_first_not_of(" \t"));
        trimmedLine.erase(trimmedLine.find_last_not_of(" \t") + 1);

        if (trimmedLine == "[ColorSettings]") {
            colorSectionFound = true;

            size_t j = i + 1;
            while (j < lines.size()) {
                std::string nextLine = lines[j];
                nextLine.erase(0, nextLine.find_first_not_of(" \t"));
                nextLine.erase(nextLine.find_last_not_of(" \t") + 1);

                if (nextLine.empty() || nextLine[0] == '[') {
                    colorSectionEnd = j;
                    break;
                }

                if (nextLine.find("Custom") == 0) {
                    lines.erase(lines.begin() + j);
                } else {
                    j++;
                }
            }

            if (colorSectionEnd == -1) {
                colorSectionEnd = lines.size();
            }
            break;
        }
    }

    if (!colorSectionFound) {
        lines.push_back("[ColorSettings]");
        colorSectionEnd = lines.size();
    }

    std::vector<std::string> colorLines;
    const char* colorNames[] = {"Primary", "Success", "Warning", "Critical"};

    for (int i = 0; i < 4; i++) {
        if (s_colorSettings.useCustomColors[i]) {
            Gdiplus::Color color = s_colorSettings.customColors[i];
            std::stringstream ss;
            ss << "Custom" << colorNames[i] << "="
               << (int)color.GetR() << ","
               << (int)color.GetG() << ","
               << (int)color.GetB();
            colorLines.push_back(ss.str());
        }
    }

    lines.insert(lines.begin() + colorSectionEnd, colorLines.begin(), colorLines.end());

    std::ofstream outFile("Config.ini");
    for (const auto& fileLine : lines) {
        outFile << fileLine << "\n";
    }
    outFile.close();
}

COLORREF ColorPickerDialog::GdiplusColorToColorRef(const Gdiplus::Color& color) {
    return RGB(color.GetR(), color.GetG(), color.GetB());
}

Gdiplus::Color ColorPickerDialog::ColorRefToGdiplusColor(COLORREF colorRef) {
    return Gdiplus::Color(255, GetRValue(colorRef), GetGValue(colorRef), GetBValue(colorRef));
}

static void UpdateColorPreview(HWND hDlg, const Gdiplus::Color& color) {
    HWND hPreview = GetDlgItem(hDlg, IDC_COLOR_PREVIEW);
    if (hPreview) {
        COLORREF colorRef = RGB(color.GetR(), color.GetG(), color.GetB());
        SetWindowLongPtr(hPreview, GWLP_USERDATA, (LONG_PTR)colorRef);
        InvalidateRect(hPreview, NULL, TRUE);
    }
}

static std::wstring GetColorName(int colorIndex) {
    switch (colorIndex) {
        case 0: return L"PRIMARY";
        case 1: return L"SUCCESS";
        case 2: return L"WARNING";
        case 3: return L"CRITICAL";
        default: return L"COLOR";
    }
}

INT_PTR CALLBACK ColorPickerDialog::ColorPickerDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG:
        {
            currentColorIndex = (int)lParam;

            std::wstring title = L"Choose " + GetColorName(currentColorIndex) + L" Color";
            SetWindowTextW(hDlg, title.c_str());

            SetDlgItemTextW(hDlg, IDC_COLOR_NAME_LABEL, GetColorName(currentColorIndex).c_str());

            tempColor = GetColor(currentColorIndex);

            UpdateColorPreview(hDlg, tempColor);

            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) {
                case IDC_COLOR_PICKER_BUTTON:
                {
                    CHOOSECOLOR cc = { 0 };
                    cc.lStructSize = sizeof(CHOOSECOLOR);
                    cc.hwndOwner = hDlg;
                    cc.lpCustColors = customColors;
                    cc.rgbResult = GdiplusColorToColorRef(tempColor);
                    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

                    if (ChooseColor(&cc)) {
                        tempColor = ColorRefToGdiplusColor(cc.rgbResult);
                        UpdateColorPreview(hDlg, tempColor);
                    }
                    return TRUE;
                }

                case IDC_USE_DEFAULT_BUTTON:
                {
                    s_colorSettings.useCustomColors[currentColorIndex] = false;
                    switch (currentColorIndex) {
                        case 0: tempColor = Colors::PRIMARY; break;
                        case 1: tempColor = Colors::SUCCESS; break;
                        case 2: tempColor = Colors::WARNING; break;
                        case 3: tempColor = Colors::CRITICAL; break;
                    }
                    s_colorSettings.customColors[currentColorIndex] = tempColor;
                    SaveColorSettings();
                    UpdateColorPreview(hDlg, tempColor);

                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }

                case IDC_COLOR_OK:
                case IDOK:
                {
                    s_colorSettings.customColors[currentColorIndex] = tempColor;
                    s_colorSettings.useCustomColors[currentColorIndex] = true;
                    SaveColorSettings();
                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }

                case IDC_COLOR_CANCEL:
                case IDCANCEL:
                {
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
                }
            }
            break;
        }

        case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT* pDIS = (DRAWITEMSTRUCT*)lParam;
            if (pDIS->CtlID == IDC_COLOR_PREVIEW) {
                COLORREF color = (COLORREF)GetWindowLongPtr(pDIS->hwndItem, GWLP_USERDATA);
                if (color == 0) {
                    color = GdiplusColorToColorRef(tempColor);
                }

                HBRUSH hBrush = CreateSolidBrush(color);
                FillRect(pDIS->hDC, &pDIS->rcItem, hBrush);
                DeleteObject(hBrush);

                FrameRect(pDIS->hDC, &pDIS->rcItem, (HBRUSH)GetStockObject(BLACK_BRUSH));
                return TRUE;
            }
            break;
        }
    }

    return FALSE;
}