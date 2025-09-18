#include "colors.h"
#include "color_picker_dialog.h"

namespace Colors {
    ThemeMode currentTheme = ThemeMode::LIGHT;

    Gdiplus::Color GetBatteryColor(int batteryLevel) {
        // 90%+ = PRIMARY (blue) - for full/near-full battery
        // 50-89% = SUCCESS (green) - for good battery level
        // 20-49% = WARNING (yellow) - for medium battery
        // 0-19% = CRITICAL (red) - for low battery

        if (batteryLevel >= 90) {
            return ColorPickerDialog::GetColor(0); // PRIMARY
        } else if (batteryLevel >= 50) {
            return ColorPickerDialog::GetColor(1); // SUCCESS
        } else if (batteryLevel >= 20) {
            return ColorPickerDialog::GetColor(2); // WARNING
        } else {
            return ColorPickerDialog::GetColor(3); // CRITICAL
        }
    }
}