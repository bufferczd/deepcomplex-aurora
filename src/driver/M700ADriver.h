#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

struct RgbColor {
    int r = 0;
    int g = 255;
    int b = 0;
};

struct MacroStep {
    // Standard USB HID Keyboard Usage ID (0x04..0xE7)
    // Or Mouse Click Codes: 0xF1=Left, 0xF2=Right, 0xF3=Middle, 0xF4=Back, 0xF5=Forward
    uint8_t hidCode = 0;
    bool isDown = true;    // true = press, false = release
    int delayMs = 24;      // delay in ms (quantized to 8ms units: 8..1016 ms)
    std::string label;     // human-readable key/click name
};

struct ButtonMacro {
    // 0 = Run repeatCount times (Once), 1 = Toggle until pressed again, 2 = Repeat while held
    int repeatMode = 0;
    int repeatCount = 1;
    std::vector<MacroStep> steps;
};

struct MouseState {
    // slot0=Left(0x01), slot1=Middle(0x02), slot2=Right(0x03),
    // slot3=Back(0x04), slot4=Forward(0x05), slot5=DPI-(0x0C), slot6=Unused(0x00), slot7=DPI+(0x0B)
    // Custom macro codes: 0x0D (Once/Toggle), 0x0E (While Held), 0x0F (Multimedia Consumer Key)
    std::array<int, 8> buttonCodes = {0x01, 0x02, 0x03, 0x04, 0x05, 0x0C, 0x00, 0x0B};
    std::array<ButtonMacro, 8> buttonMacros;
    std::array<uint16_t, 8> buttonMediaUsages = {0, 0, 0, 0, 0, 0, 0, 0};
    int fireTimeMs = 48;

    int activeDpiLevel = 2;  // 1..6
    std::array<bool, 6> dpiEnabled = {true, true, true, true, true, true};
    std::array<int, 6> dpiIndices = {3, 5, 10, 13, 15, 16}; // 0..16 (200..7200 DPI)

    int ledMode = 0x02;            // 0x00..0x0D (default Static)
    int ledSpeed = 5;              // 0..5
    int ledBrightness = 6;         // 0..6
    int ledDirection = 0;          // 0..3
    bool ledSymmetry = false;
    bool ledSingleColor = false;
    bool userColorEnabled = true;

    int reactiveEffect = 0;
    std::array<bool, 8> reactiveButtons = {true, true, true, true, true, true, false, true};

    std::array<RgbColor, 6> dpiColors = {{
        {255, 255, 0}, {255, 0, 0}, {0, 255, 0},
        {0, 255, 255}, {0, 0, 255}, {255, 0, 255}
    }};

    std::array<RgbColor, 7> userColors = {{
        {255, 0, 0}, {255, 128, 0}, {255, 255, 0}, {0, 255, 0},
        {0, 255, 255}, {0, 0, 255}, {255, 0, 255}
    }};
    std::array<bool, 7> userColorSlotsEnabled = {true, true, true, true, true, true, true};

    // Independent 6-zone palette for Extra Mode (0x09) so userColors stays untouched
    std::array<RgbColor, 6> extraModeColors = {{
        {255, 0, 255}, {255, 0, 0}, {0, 255, 255},
        {255, 255, 0}, {0, 255, 0}, {0, 68, 255}
    }};

    std::string devicePath;
};

struct ThemeDpiSnapshot {
    int activeDpiLevel = 2;
    RgbColor activeDpiColor{0, 255, 0};
    bool inExtraMode = false;
    std::array<int, 6> dpiIndices{{3, 5, 10, 13, 15, 16}};
    std::array<RgbColor, 6> dpiColors{{
        {255, 255, 0}, {255, 0, 0}, {0, 255, 0},
        {0, 255, 255}, {0, 0, 255}, {255, 0, 255}
    }};
};

enum class DriverError {
    None,
    NoDevice,
    OpenFailed,
    ReadFailed,
    WriteFailed,
    NoLeftClick
};

class M700ADriver {
public:
    using ProgressCallback = std::function<void(int percent, const std::string &logLine)>;

    static constexpr std::array<uint8_t, 17> kDpiCodeTable = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12
    };
    static constexpr std::array<int, 17> kDpiValueTable = {
        200, 400, 600, 800, 1000, 1200, 1400, 1600,
        1800, 2000, 2400, 3200, 4000, 4800, 5600, 6400, 7200
    };

    static std::optional<std::string> findDevicePath();
    static int dpiCodeToIndex(uint8_t code);
    static std::mutex &hardwareIoMutex();
    static bool readThemeDpiSnapshot(ThemeDpiSnapshot &out);
    static bool readActiveDpiLevelOnly(int &outLevel);

    bool readState(MouseState &outState, DriverError &errCode, std::string &detailOut,
                   const ProgressCallback &progressCb = nullptr) const;
    bool applySettings(const MouseState &newState, DriverError &errCode, std::string &detailOut,
                       const ProgressCallback &progressCb = nullptr) const;

private:
    static bool sendFeature(int fd, const std::array<uint8_t, 8> &data);
    static bool getFeature(int fd, std::array<uint8_t, 8> &outData);

    static bool readByte(int fd, uint16_t addr, uint8_t &outByte);
    static bool readPage(int fd, uint16_t baseAddr, std::array<uint8_t, 8> &outPage);
    static bool writeBytes(int fd, uint16_t addr, const uint8_t *data, size_t len);
    static bool applyChanges(int fd);

    static uint8_t rgbToNibbleInv(int channel255);
    static int nibbleInvToRgb(uint8_t nibble);

};
