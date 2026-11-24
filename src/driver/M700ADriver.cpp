#include "M700ADriver.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <linux/hidraw.h>
#include <sstream>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>

namespace fs = std::filesystem;

namespace {
constexpr unsigned long kHidSetFeature8 = HIDIOCSFEATURE(8);
constexpr unsigned long kHidGetFeature8 = HIDIOCGFEATURE(8);

void sleepMs(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void report(const M700ADriver::ProgressCallback &cb, int pct, const std::string &msg) {
    if (cb) {
        cb(std::clamp(pct, 0, 100), msg);
    }
}

std::string hexAddr(uint16_t addr) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "0x%04X", addr);
    return std::string(buf);
}

} // namespace

int M700ADriver::dpiCodeToIndex(uint8_t code) {
    for (size_t i = 0; i < kDpiCodeTable.size(); ++i) {
        if (kDpiCodeTable[i] == code) {
            return static_cast<int>(i);
        }
    }
    if (code == 0x0B) return 10;
    return (code < 0x01) ? 0 : 16;
}

std::mutex &M700ADriver::hardwareIoMutex() {
    static std::mutex s_mutex;
    return s_mutex;
}

namespace {
struct ScopedFd {
    int fd = -1;
    explicit ScopedFd(int f) : fd(f) {}
    ~ScopedFd() { if (fd >= 0) ::close(fd); }
    ScopedFd(const ScopedFd &) = delete;
    ScopedFd &operator=(const ScopedFd &) = delete;
    int get() const { return fd; }
    bool valid() const { return fd >= 0; }
};
} // namespace

std::optional<std::string> M700ADriver::findDevicePath() {
    const fs::path base("/sys/class/hidraw");
    std::error_code ec;
    if (!fs::exists(base, ec) || ec) {
        return std::nullopt;
    }

    fs::directory_iterator it(base, fs::directory_options::skip_permission_denied, ec);
    if (ec) {
        return std::nullopt;
    }

    for (fs::directory_iterator end; it != end; it.increment(ec)) {
        if (ec) { ec.clear(); continue; }
        const std::string fname = it->path().filename().string();
        if (fname.size() <= 6 || fname.compare(0, 6, "hidraw") != 0) continue;
        bool digitsOnly = true;
        for (size_t k = 6; k < fname.size(); ++k) {
            if (fname[k] < '0' || fname[k] > '9') { digitsOnly = false; break; }
        }
        if (!digitsOnly) continue;

        const fs::path ueventPath = it->path() / "device" / "uevent";
        std::ifstream file(ueventPath, std::ios::in | std::ios::binary);
        if (!file.is_open()) continue;
        char buf[4096];
        file.read(buf, static_cast<std::streamsize>(sizeof(buf) - 1));
        const std::streamsize n = file.gcount();
        if (n <= 0) continue;
        const std::string content(buf, static_cast<size_t>(n));

        if (content.find("000030FA:00001040") != std::string::npos &&
            content.find("/input1") != std::string::npos) {
            return "/dev/" + fname;
        }
    }
    return std::nullopt;
}

bool M700ADriver::sendFeature(int fd, const std::array<uint8_t, 8> &data) {
    std::array<uint8_t, 8> buf = data;
    int rc = -1;
    for (int attempt = 0; attempt < 3; ++attempt) {
        rc = ioctl(fd, kHidSetFeature8, buf.data());
        if (rc >= 0 || errno != EINTR) break;
    }
    if (rc < 0) {
        return false;
    }
    sleepMs(6);
    return true;
}

bool M700ADriver::getFeature(int fd, std::array<uint8_t, 8> &outData) {
    int rc = -1;
    for (int attempt = 0; attempt < 3; ++attempt) {
        outData.fill(0);
        outData[0] = 0x07;
        rc = ioctl(fd, kHidGetFeature8, outData.data());
        if (rc >= 0 || errno != EINTR) break;
    }
    return rc >= 0;
}

bool M700ADriver::readByte(int fd, uint16_t addr, uint8_t &outByte) {
    const uint8_t lo = static_cast<uint8_t>(addr & 0xFF);
    const uint8_t hi = static_cast<uint8_t>((addr >> 8) & 0xFF);

    for (int attempt = 0; attempt < 2; ++attempt) {
        if (!sendFeature(fd, {0x07, 0x18, 0x05, 0x00, lo, hi, 0x00, 0x00})) {
            return false;
        }
        sleepMs(8);
        std::array<uint8_t, 8> resp{};
        if (!getFeature(fd, resp)) {
            return false;
        }
        sendFeature(fd, {0x07, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
        if (resp[0] == 0x07) {
            outByte = resp[1];
            return true;
        }
        sleepMs(10);
    }
    return false;
}

bool M700ADriver::readPage(int fd, uint16_t baseAddr, std::array<uint8_t, 8> &outPage) {
    for (uint16_t i = 0; i < 8; ++i) {
        if (!readByte(fd, static_cast<uint16_t>(baseAddr + i), outPage[i])) {
            return false;
        }
    }
    return true;
}

bool M700ADriver::writeBytes(int fd, uint16_t addr, const uint8_t *data, size_t len) {
    if (len == 0 || len > 8) {
        return false;
    }
    const uint8_t lo = static_cast<uint8_t>(addr & 0xFF);
    const uint8_t hi = static_cast<uint8_t>((addr >> 8) & 0xFF);
    const uint8_t lenCode = static_cast<uint8_t>(len - 1);

    for (size_t i = 0; i < len; ++i) {
        if (!sendFeature(fd, {0x07, 0x18, 0x03, static_cast<uint8_t>(i), lo, hi, data[i], lenCode})) {
            return false;
        }
    }
    sleepMs(12);
    if (!sendFeature(fd, {0x07, 0x18, 0x09, 0x00, lo, hi, 0x00, lenCode})) {
        return false;
    }
    sleepMs(18);
    return sendFeature(fd, {0x07, 0x18, 0x00, 0x00, lo, hi, 0x00, 0x00});
}

bool M700ADriver::applyChanges(int fd) {
    if (!sendFeature(fd, {0x07, 0x18, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00})) {
        return false;
    }
    sleepMs(18);
    return sendFeature(fd, {0x07, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
}

uint8_t M700ADriver::rgbToNibbleInv(int channel255) {
    int clamped = std::clamp(channel255, 0, 255);
    int level4 = (clamped + 8) / 17;
    return static_cast<uint8_t>(0x0F - std::clamp(level4, 0, 15));
}

int M700ADriver::nibbleInvToRgb(uint8_t nibble) {
    int level4 = 0x0F - (nibble & 0x0F);
    return std::clamp(level4 * 17, 0, 255);
}

bool M700ADriver::readState(MouseState &outState, DriverError &errCode, std::string &detailOut,
                            const ProgressCallback &progressCb) const {
    std::lock_guard<std::mutex> ioLock(hardwareIoMutex());
    errCode = DriverError::None;
    detailOut.clear();

    report(progressCb, 5, "[HID] Searching for Delux M700A (30FA:1040)...");
    auto path = findDevicePath();
    if (!path.has_value()) {
        errCode = DriverError::NoDevice;
        report(progressCb, 100, "[ERROR] Device not found.");
        return false;
    }

    report(progressCb, 12, "[HID] Opening " + *path + "...");
    ScopedFd fdGuard(::open(path->c_str(), O_RDWR | O_CLOEXEC));
    if (!fdGuard.valid()) {
        errCode = DriverError::OpenFailed;
        detailOut = *path + ": " + std::strerror(errno);
        report(progressCb, 100, "[ERROR] Open failed: " + detailOut);
        return false;
    }
    const int fd = fdGuard.get();

    uint8_t activeProfileByte = 0;
    std::array<uint8_t, 8> btnPage{}, page10{}, page18{}, page20{}, page28{}, page30{}, page38{};

    bool ok = true;
    report(progressCb, 18, "[EEPROM] Reading active profile index (0x0076)...");
    ok = ok && readByte(fd, 0x0076, activeProfileByte);

    if (ok) { report(progressCb, 26, "[EEPROM] Reading DPI table (0x0010)..."); ok = readPage(fd, 0x0010, page10); }
    if (ok) { report(progressCb, 36, "[EEPROM] Reading LED & Fire config (0x0018)..."); ok = readPage(fd, 0x0018, page18); }
    if (ok) { report(progressCb, 46, "[EEPROM] Reading color palette 1/3 (0x0020)..."); ok = readPage(fd, 0x0020, page20); }
    if (ok) { report(progressCb, 56, "[EEPROM] Reading color palette 2/3 (0x0028)..."); ok = readPage(fd, 0x0028, page28); }
    if (ok) { report(progressCb, 64, "[EEPROM] Reading color palette 3/3 (0x0030)..."); ok = readPage(fd, 0x0030, page30); }
    if (ok) { report(progressCb, 72, "[EEPROM] Reading brightness & direction (0x0038)..."); ok = readPage(fd, 0x0038, page38); }

    if (ok) {
        int profile = activeProfileByte & 0x03;
        report(progressCb, 80, "[EEPROM] Reading button map (Profile " + std::to_string(profile) + ")...");
        if (profile == 1) {
            for (uint16_t i = 0; i < 8 && ok; ++i) {
                ok = readByte(fd, static_cast<uint16_t>(0x0044 + i), btnPage[i]);
            }
        } else if (profile == 2) {
            ok = readPage(fd, 0x0050, btnPage);
        } else if (profile == 3) {
            ok = readPage(fd, 0x0060, btnPage);
        } else {
            ok = readPage(fd, 0x0008, btnPage);
        }
    }

    if (!ok) {
        errCode = DriverError::ReadFailed;
        report(progressCb, 100, "[ERROR] EEPROM read failed.");
        return false;
    }

    outState.devicePath = *path;
    outState.reactiveEffect = (btnPage[0] >> 6) & 0x03;
    for (size_t i = 0; i < 8; ++i) {
        outState.buttonCodes[i] = btnPage[i] & 0x1F;
        outState.reactiveButtons[i] = ((btnPage[i] >> 5) & 0x01) != 0;
    }

    // Read onboard macros (0x0D / 0x0E) and multimedia descriptors (0x0F) from 0x00A0 + 0x0400+
    for (size_t i = 0; i < 8; ++i) {
        const int code = outState.buttonCodes[i];
        if (code != 0x0F) {
            outState.buttonMediaUsages[i] = 0;
        }
        if (code == 0x0D || code == 0x0E || code == 0x0F) {
            std::array<uint8_t, 10> desc{};
            bool descOk = true;
            const uint16_t descAddr = static_cast<uint16_t>(0x00A0 + i * 10);
            for (uint16_t b = 0; b < 10 && descOk; ++b) {
                descOk = readByte(fd, static_cast<uint16_t>(descAddr + b), desc[b]);
            }
            if (!descOk) continue;

            if (code == 0x0F) {
                outState.buttonMediaUsages[i] = static_cast<uint16_t>(desc[3] | (desc[4] << 8));
                report(progressCb, 86, "[MEDIA] Slot " + std::to_string(i) + " usage=" + hexAddr(outState.buttonMediaUsages[i]));
            } else {
                const int stepCount = std::clamp<int>((desc[1] & 0x7F) | ((desc[7] & 0x3F) << 7), 0, 120);
                const bool holdMode = ((desc[1] & 0x80) != 0) || (code == 0x0E);
                const bool toggleMode = (desc[6] & 0x80) != 0;
                const uint16_t startAddr = static_cast<uint16_t>(desc[5] | ((desc[6] & 0x3F) << 8));

                if (stepCount > 0 && startAddr >= 0x0400 && startAddr + 2 + stepCount * 2 <= 0x0800) {
                    report(progressCb, 90, "[MACRO] Reading onboard macro slot " + std::to_string(i) +
                                           " from " + hexAddr(startAddr) + " (" + std::to_string(stepCount) + " steps)...");
                    uint8_t repLo = 1, repHi = 0;
                    bool payOk = readByte(fd, startAddr, repLo) && readByte(fd, static_cast<uint16_t>(startAddr + 1), repHi);
                    ButtonMacro mac;
                    mac.repeatMode = holdMode ? 2 : (toggleMode ? 1 : 0);
                    mac.repeatCount = std::clamp<int>(repLo | (repHi << 8), 1, 255);
                    mac.steps.reserve(static_cast<size_t>(stepCount));

                    for (int s = 0; s < stepCount && payOk; ++s) {
                        uint8_t tByte = 0, hCode = 0;
                        const uint16_t sAddr = static_cast<uint16_t>(startAddr + 2 + s * 2);
                        payOk = readByte(fd, sAddr, tByte) && readByte(fd, static_cast<uint16_t>(sAddr + 1), hCode);
                        if (payOk && hCode != 0) {
                            MacroStep step;
                            step.isDown = (tByte & 0x80) != 0;
                            step.delayMs = std::clamp<int>((tByte & 0x7F) * 8, 8, 1016);
                            step.hidCode = hCode;
                            mac.steps.push_back(step);
                        }
                    }
                    if (payOk && !mac.steps.empty()) {
                        outState.buttonMacros[i] = std::move(mac);
                    }
                }
            }
        }
    }



    uint16_t rawFireUnits = static_cast<uint16_t>(page18[2] | (page18[3] << 8));
    outState.fireTimeMs = std::clamp<int>(rawFireUnits * 8, 8, 400);

    const uint8_t msbMask = page10[3];
    const std::array<uint8_t, 6> lowNibbles = {
        static_cast<uint8_t>((page10[5] >> 4) & 0x0F),
        static_cast<uint8_t>(page10[5] & 0x0F),
        static_cast<uint8_t>((page10[6] >> 4) & 0x0F),
        static_cast<uint8_t>(page10[6] & 0x0F),
        static_cast<uint8_t>((page10[7] >> 4) & 0x0F),
        static_cast<uint8_t>(page10[7] & 0x0F)
    };
    for (size_t i = 0; i < 6; ++i) {
        uint8_t code = static_cast<uint8_t>((((msbMask >> i) & 0x01) << 4) | lowNibbles[i]);
        outState.dpiIndices[i] = dpiCodeToIndex(code);
        outState.dpiEnabled[i] = ((page18[1] >> i) & 0x01) != 0;
    }

    outState.activeDpiLevel = std::clamp(static_cast<int>(page18[0] & 0x07) + 1, 1, 6);

    const uint8_t b1c = page18[4];
    const uint8_t b33 = page30[3];
    if ((b1c & 0x07) == 0x07) {
        outState.ledMode = 0x0D;
        outState.ledSpeed = 0;
    } else if ((b1c & 0xF8) == 0x98 && (b33 & 0x80) == 0) {
        outState.ledMode = 0x0C;
        outState.ledSpeed = std::clamp(static_cast<int>(b1c & 0x07), 0, 5);
    } else {
        int low3 = (((b1c >> 4) - 8) & 0x07);
        int bit3 = (b33 & 0x80) ? 8 : 0;
        outState.ledMode = bit3 | low3;
        outState.ledSpeed = std::clamp(static_cast<int>(b1c & 0x07), 0, 5);
    }

    const uint8_t b1d = page18[5];
    if (outState.ledMode != 0x09) {
        outState.userColorEnabled = ((b1d >> 7) & 0x01) != 0;
        for (size_t i = 0; i < 7; ++i) {
            outState.userColorSlotsEnabled[i] = ((b1d >> i) & 0x01) != 0;
        }
    }

    outState.ledSingleColor = ((page38[2] >> 7) & 0x01) != 0;
    outState.ledSymmetry = ((page38[2] >> 6) & 0x01) != 0;
    outState.ledDirection = (page38[2] >> 4) & 0x03;
    const int invBright = (page38[7] >> 4) & 0x07;
    outState.ledBrightness = std::clamp(6 - invBright, 0, 6);

    std::array<uint8_t, 24> colorBuf{};
    std::copy(page20.begin(), page20.end(), colorBuf.begin());
    std::copy(page28.begin(), page28.end(), colorBuf.begin() + 8);
    std::copy(page30.begin(), page30.end(), colorBuf.begin() + 16);

    for (size_t i = 0; i < 6; ++i) {
        const size_t off = i * 3;
        outState.dpiColors[i] = {
            nibbleInvToRgb(colorBuf[off + 0] & 0x0F),
            nibbleInvToRgb(colorBuf[off + 1] & 0x0F),
            nibbleInvToRgb(colorBuf[off + 2] & 0x0F)
        };
        RgbColor upperCol = {
            nibbleInvToRgb((colorBuf[off + 0] >> 4) & 0x0F),
            nibbleInvToRgb((colorBuf[off + 1] >> 4) & 0x0F),
            nibbleInvToRgb((colorBuf[off + 2] >> 4) & 0x0F)
        };
        if (outState.ledMode == 0x09) {
            outState.extraModeColors[i] = upperCol;
        } else {
            outState.userColors[i] = upperCol;
        }
    }

    if (outState.ledMode != 0x09) {
        outState.userColors[6] = {
            nibbleInvToRgb((page38[6] >> 4) & 0x0F),
            nibbleInvToRgb(page38[6] & 0x0F),
            nibbleInvToRgb(page38[7] & 0x0F)
        };
    }

    report(progressCb, 100, "[OK] Mouse settings read successfully.");
    return true;
}


bool M700ADriver::readThemeDpiSnapshot(ThemeDpiSnapshot &out) {
    std::unique_lock<std::mutex> ioLock(hardwareIoMutex(), std::try_to_lock);
    if (!ioLock.owns_lock()) {
        // Hardware is currently being read/written by the editor (e.g. applying Extra Mode) — never interrupt!
        return false;
    }

    auto path = findDevicePath();
    if (!path.has_value()) {
        return false;
    }

    ScopedFd fdGuard(::open(path->c_str(), O_RDWR | O_CLOEXEC));
    if (!fdGuard.valid()) {
        return false;
    }
    const int fd = fdGuard.get();

    for (int attempt = 0; attempt < 2; ++attempt) {
        sendFeature(fd, {0x07, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
        sleepMs(10);

        uint8_t syncByte = 0;
        if (!readByte(fd, 0x0076, syncByte)) {
            sleepMs(20);
            continue;
        }

        std::array<uint8_t, 8> page10{}, page18{}, page20{}, page28{}, page30{};
        if (!readPage(fd, 0x0010, page10) ||
            !readPage(fd, 0x0018, page18) ||
            !readPage(fd, 0x0020, page20) ||
            !readPage(fd, 0x0028, page28) ||
            !readPage(fd, 0x0030, page30)) {
            sleepMs(20);
            continue;
        }

        // Strict integrity validation:
        // 1) page18[0] MUST be 0x50..0x55 (active DPI index 0..5)
        // 2) page30[2] (0x0032) MUST equal magic signature 0xED (zero byte-shift guarantee)
        const bool validDpiByte = ((page18[0] & 0xF8) == 0x50) && ((page18[0] & 0x07) <= 5);
        const bool validPaletteMagic = (page30[2] == 0xED);
        if (!validDpiByte || !validPaletteMagic) {
            sleepMs(25);
            continue;
        }

        // Detect whether hardware LED controller is currently running Extra Mode (0x09)
        const uint8_t b1c = page18[4];
        const uint8_t b33 = page30[3];
        int hwLedMode = 0x02;
        if ((b1c & 0x07) == 0x07) {
            hwLedMode = 0x0D;
        } else if ((b1c & 0xF8) == 0x98 && (b33 & 0x80) == 0) {
            hwLedMode = 0x0C;
        } else {
            const int low3 = (((b1c >> 4) - 8) & 0x07);
            const int bit3 = (b33 & 0x80) ? 8 : 0;
            hwLedMode = bit3 | low3;
        }
        out.inExtraMode = (hwLedMode == 0x09);

        // Decode hardware DPI stages table (page10)
        const uint8_t msbMask = page10[3];
        const std::array<uint8_t, 6> lowNibbles = {
            static_cast<uint8_t>((page10[5] >> 4) & 0x0F),
            static_cast<uint8_t>(page10[5] & 0x0F),
            static_cast<uint8_t>((page10[6] >> 4) & 0x0F),
            static_cast<uint8_t>(page10[6] & 0x0F),
            static_cast<uint8_t>((page10[7] >> 4) & 0x0F),
            static_cast<uint8_t>(page10[7] & 0x0F)
        };
        for (size_t i = 0; i < 6; ++i) {
            const uint8_t code = static_cast<uint8_t>((((msbMask >> i) & 0x01) << 4) | lowNibbles[i]);
            out.dpiIndices[i] = dpiCodeToIndex(code);
        }

        // Decode hardware DPI indicator colors (lower nibbles of 0x0020..0x0031 — independent of Extra Mode upper nibbles)
        const int rawIdx = static_cast<int>(page18[0] & 0x07);
        std::array<uint8_t, 24> colorBuf{};
        std::copy(page20.begin(), page20.end(), colorBuf.begin());
        std::copy(page28.begin(), page28.end(), colorBuf.begin() + 8);
        std::copy(page30.begin(), page30.end(), colorBuf.begin() + 16);

        for (size_t i = 0; i < 6; ++i) {
            const size_t off = i * 3;
            out.dpiColors[i] = {
                nibbleInvToRgb(colorBuf[off + 0] & 0x0F),
                nibbleInvToRgb(colorBuf[off + 1] & 0x0F),
                nibbleInvToRgb(colorBuf[off + 2] & 0x0F)
            };
        }

        out.activeDpiLevel = rawIdx + 1;
        out.activeDpiColor = out.dpiColors[static_cast<size_t>(rawIdx)];
        return true;
    }
    return false;
}


bool M700ADriver::readActiveDpiLevelOnly(int &outLevel) {
    std::unique_lock<std::mutex> ioLock(hardwareIoMutex(), std::try_to_lock);
    if (!ioLock.owns_lock()) {
        return false;
    }

    auto path = findDevicePath();
    if (!path.has_value()) {
        return false;
    }

    ScopedFd fdGuard(::open(path->c_str(), O_RDWR | O_CLOEXEC));
    if (!fdGuard.valid()) {
        return false;
    }
    const int fd = fdGuard.get();

    sendFeature(fd, {0x07, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    sleepMs(8);

    uint8_t syncByte = 0;
    uint8_t b18 = 0;
    if (!readByte(fd, 0x0076, syncByte) || !readByte(fd, 0x0018, b18)) {
        return false;
    }

    // Strict integrity check: Instant A725 active DPI register MUST be 0x50..0x55
    if ((b18 & 0xF8) != 0x50 || (b18 & 0x07) > 5) {
        return false;
    }

    outLevel = static_cast<int>(b18 & 0x07) + 1;
    return true;
}

bool M700ADriver::applySettings(const MouseState &newState, DriverError &errCode, std::string &detailOut,
                                const ProgressCallback &progressCb) const {
    std::lock_guard<std::mutex> ioLock(hardwareIoMutex());
    errCode = DriverError::None;
    detailOut.clear();

    report(progressCb, 2, "[CHECK] Verifying Left Click safety...");
    bool hasLeftClick = false;
    for (size_t slot : {0u, 1u, 2u, 3u, 4u, 5u, 7u}) {
        if ((newState.buttonCodes[slot] & 0x1F) == 0x01) {
            hasLeftClick = true;
            break;
        }
    }
    if (!hasLeftClick) {
        errCode = DriverError::NoLeftClick;
        report(progressCb, 100, "[ERROR] Safety block: at least one button must remain Left Click.");
        return false;
    }

    report(progressCb, 5, "[HID] Locating Delux M700A (30FA:1040)...");
    auto path = findDevicePath();
    if (!path.has_value()) {
        errCode = DriverError::NoDevice;
        report(progressCb, 100, "[ERROR] Device not found.");
        return false;
    }

    report(progressCb, 8, "[HID] Opening " + *path + "...");
    ScopedFd fdGuard(::open(path->c_str(), O_RDWR | O_CLOEXEC));
    if (!fdGuard.valid()) {
        errCode = DriverError::OpenFailed;
        detailOut = *path + ": " + std::strerror(errno);
        report(progressCb, 100, "[ERROR] Open failed: " + detailOut);
        return false;
    }
    const int fd = fdGuard.get();

    std::array<uint8_t, 8> page08{}, page10{}, page18{}, page20{}, page28{}, page30{}, page38{};
    std::array<uint8_t, 8> page40{}, page48{}, page50{}, page60{}, page70{};

    report(progressCb, 14, "[READ] Snapshotting current EEPROM pages (0x0008..0x0070)...");
    if (!readPage(fd, 0x0008, page08) ||
        !readPage(fd, 0x0010, page10) ||
        !readPage(fd, 0x0018, page18) ||
        !readPage(fd, 0x0020, page20) ||
        !readPage(fd, 0x0028, page28) ||
        !readPage(fd, 0x0030, page30) ||
        !readPage(fd, 0x0038, page38) ||
        !readPage(fd, 0x0040, page40) ||
        !readPage(fd, 0x0048, page48) ||
        !readPage(fd, 0x0050, page50) ||
        !readPage(fd, 0x0060, page60) ||
        !readPage(fd, 0x0070, page70)) {
        errCode = DriverError::ReadFailed;
        report(progressCb, 100, "[ERROR] Failed to read base EEPROM pages.");
        return false;
    }

    // 1. Prepare button map + macro/media descriptors (0x00A0..0x00EF) + macro payloads (0x0400+)
    std::array<uint8_t, 80> macroDescBuf{}; // 8 slots * 10 bytes = 80 bytes (10 pages at 0x00A0)
    struct MacroWriteChunk {
        uint16_t addr;
        std::vector<uint8_t> bytes;
        int slot;
        int stepCount;
    };
    std::vector<MacroWriteChunk> macroPayloads;
    uint16_t nextMacroAddr = 0x0400;
    bool hasAnyDescriptor = false;

    const uint8_t reactType = static_cast<uint8_t>(std::clamp(newState.reactiveEffect, 0, 3));
    for (size_t i = 0; i < 8; ++i) {
        uint8_t keyCode = static_cast<uint8_t>(newState.buttonCodes[i] & 0x1F);

        if (keyCode == 0x0F) {
            // Multimedia / Consumer Control Key
            const uint16_t usage = newState.buttonMediaUsages[i];
            if (usage != 0) {
                hasAnyDescriptor = true;
                const size_t dOff = static_cast<size_t>(i * 10);
                macroDescBuf[dOff + 0] = 0x07;
                macroDescBuf[dOff + 1] = 0x00;
                macroDescBuf[dOff + 2] = 0x02; // Consumer Control report type
                macroDescBuf[dOff + 3] = static_cast<uint8_t>(usage & 0xFF);
                macroDescBuf[dOff + 4] = static_cast<uint8_t>((usage >> 8) & 0xFF);
                macroDescBuf[dOff + 5] = 0x00;
                macroDescBuf[dOff + 6] = 0x00;
                macroDescBuf[dOff + 7] = 0x00;
                macroDescBuf[dOff + 8] = 0x00;
                macroDescBuf[dOff + 9] = 0x00;
            } else {
                keyCode = 0x00;
            }
        } else if (keyCode == 0x0D || keyCode == 0x0E) {
            const auto &mac = newState.buttonMacros[i];
            if (!mac.steps.empty()) {
                hasAnyDescriptor = true;
                const int rMode = std::clamp(mac.repeatMode, 0, 2);
                keyCode = (rMode == 2) ? 0x0E : 0x0D;

                const int stepCount = std::min<int>(static_cast<int>(mac.steps.size()), 120);
                const uint16_t startAddr = nextMacroAddr;

                const size_t dOff = static_cast<size_t>(i * 10);
                macroDescBuf[dOff + 0] = 0x07; // 8ms time base
                macroDescBuf[dOff + 1] = static_cast<uint8_t>((stepCount & 0x7F) | ((rMode == 2 ? 1 : 0) << 7));
                macroDescBuf[dOff + 2] = 0x00;
                macroDescBuf[dOff + 3] = 0x00;
                macroDescBuf[dOff + 4] = 0x00;
                macroDescBuf[dOff + 5] = static_cast<uint8_t>(startAddr & 0xFF);
                macroDescBuf[dOff + 6] = static_cast<uint8_t>(((startAddr >> 8) & 0x3F) | ((rMode == 1 ? 1 : 0) << 7));
                macroDescBuf[dOff + 7] = static_cast<uint8_t>((stepCount >> 7) & 0x3F);
                macroDescBuf[dOff + 8] = 0x00;
                macroDescBuf[dOff + 9] = 0x00;

                std::vector<uint8_t> payload;
                payload.reserve(static_cast<size_t>(2 + stepCount * 2));
                const uint16_t rep = (rMode == 0) ? static_cast<uint16_t>(std::clamp(mac.repeatCount, 1, 255)) : 1;
                payload.push_back(static_cast<uint8_t>(rep & 0xFF));
                payload.push_back(static_cast<uint8_t>((rep >> 8) & 0xFF));

                for (int s = 0; s < stepCount; ++s) {
                    const auto &st = mac.steps[static_cast<size_t>(s)];
                    uint8_t units = static_cast<uint8_t>(std::clamp((st.delayMs + 4) / 8, 1, 127));
                    uint8_t timeByte = st.isDown ? static_cast<uint8_t>(units | 0x80) : units;
                    payload.push_back(timeByte);
                    payload.push_back(st.hidCode);
                }

                uint16_t alignedLen = static_cast<uint16_t>(((payload.size() + 7) / 8) * 8);
                if (startAddr + alignedLen <= 0x07F8) {
                    macroPayloads.push_back({startAddr, std::move(payload), static_cast<int>(i), stepCount});
                    nextMacroAddr = static_cast<uint16_t>(startAddr + alignedLen);
                } else {
                    std::fill_n(macroDescBuf.begin() + static_cast<std::ptrdiff_t>(dOff), 10, static_cast<uint8_t>(0));
                    keyCode = 0x00;
                }
            } else {
                keyCode = 0x00;
            }
        }

        if (keyCode == 0x00) {
            page08[i] = 0x00;
        } else {
            uint8_t en = newState.reactiveButtons[i] ? 1 : 0;
            page08[i] = static_cast<uint8_t>(((reactType * 2 + en) << 5) | keyCode);
        }
    }

    for (size_t i = 0; i < 4; ++i) {
        page40[4 + i] = page08[i];
        page48[i] = page08[4 + i];
    }
    page50 = page08;
    page60 = page08;
    page70[6] = static_cast<uint8_t>(page70[6] & 0xFC);

    // 2. DPI table (preserve factory page10[4] untouched)
    uint8_t msbMask = 0;
    uint8_t enableMask = 0;
    std::array<uint8_t, 6> codes{};
    for (size_t i = 0; i < 6; ++i) {
        size_t idx = static_cast<size_t>(std::clamp(newState.dpiIndices[i], 0, 16));
        codes[i] = kDpiCodeTable[idx];
        if ((codes[i] >> 4) & 0x01) msbMask |= static_cast<uint8_t>(1u << i);
        if (newState.dpiEnabled[i]) enableMask |= static_cast<uint8_t>(1u << i);
    }
    if (enableMask == 0) enableMask = 0x01;

    page10[3] = msbMask;
    page10[5] = static_cast<uint8_t>(((codes[0] & 0x0F) << 4) | (codes[1] & 0x0F));
    page10[6] = static_cast<uint8_t>(((codes[2] & 0x0F) << 4) | (codes[3] & 0x0F));
    page10[7] = static_cast<uint8_t>(((codes[4] & 0x0F) << 4) | (codes[5] & 0x0F));

    // 3. Active DPI + Fire Time + LED Mode
    page18[0] = static_cast<uint8_t>(0x50 | (std::clamp(newState.activeDpiLevel - 1, 0, 5) & 0x07));
    page18[1] = enableMask;
    uint16_t fireUnits = static_cast<uint16_t>(std::clamp(newState.fireTimeMs, 8, 400) / 8);
    page18[2] = static_cast<uint8_t>(fireUnits & 0xFF);
    page18[3] = static_cast<uint8_t>((fireUnits >> 8) & 0xFF);
    page18[6] = 0x07;
    page18[7] = 0x00;

    uint8_t b1c = 0xA0;
    uint8_t b33 = 0x2D;
    const uint8_t spd = static_cast<uint8_t>(std::clamp(newState.ledSpeed, 0, 5));

    if (newState.ledMode == 0x0D) {
        b1c = 0xA7;
        b33 = 0x2D;
    } else if (newState.ledMode == 0x0C) {
        b1c = static_cast<uint8_t>(0x98 | (spd & 0x07));
        b33 = 0x2D;
    } else {
        int modeVal = std::clamp(newState.ledMode, 0, 11);
        uint8_t modeSpd = (modeVal == 0x02) ? 0 : spd;
        b1c = static_cast<uint8_t>(((((modeVal + 8) * 2) << 3) & 0xFF) | (modeSpd & 0x07));
        b33 = static_cast<uint8_t>((((modeVal & 0x08) << 4) + 0x2D) & 0xFF);
    }
    page18[4] = b1c;

    const bool isExtraMode = (newState.ledMode == 0x09);
    if (isExtraMode) {
        // Extra Mode uses its own independent 6-zone palette in hardware
        page18[5] = 0xBF; // bit7=1 (user palette enabled) + slots 0..5 enabled
    } else {
        uint8_t slotMask = 0;
        for (size_t i = 0; i < 7; ++i) {
            if (newState.userColorSlotsEnabled[i]) slotMask |= static_cast<uint8_t>(1u << i);
        }
        if (slotMask == 0) slotMask = 0x01;
        page18[5] = static_cast<uint8_t>((newState.userColorEnabled ? 0x80 : 0x00) | (slotMask & 0x7F));
    }

    // 4. Colors (Extra Mode uses independent extraModeColors[0..5], other modes use userColors[0..6])
    std::array<uint8_t, 24> colorBuf{};
    std::copy(page20.begin(), page20.end(), colorBuf.begin());
    std::copy(page28.begin(), page28.end(), colorBuf.begin() + 8);
    std::copy(page30.begin(), page30.end(), colorBuf.begin() + 16);

    for (size_t i = 0; i < 6; ++i) {
        const size_t off = i * 3;
        const RgbColor &activeUpper = isExtraMode ? newState.extraModeColors[i] : newState.userColors[i];
        colorBuf[off + 0] = static_cast<uint8_t>((rgbToNibbleInv(activeUpper.r) << 4) |
                                                 rgbToNibbleInv(newState.dpiColors[i].r));
        colorBuf[off + 1] = static_cast<uint8_t>((rgbToNibbleInv(activeUpper.g) << 4) |
                                                 rgbToNibbleInv(newState.dpiColors[i].g));
        colorBuf[off + 2] = static_cast<uint8_t>((rgbToNibbleInv(activeUpper.b) << 4) |
                                                 rgbToNibbleInv(newState.dpiColors[i].b));
    }
    colorBuf[18] = 0xED;
    colorBuf[19] = b33;

    std::copy(colorBuf.begin(), colorBuf.begin() + 8, page20.begin());
    std::copy(colorBuf.begin() + 8, colorBuf.begin() + 16, page28.begin());
    std::copy(colorBuf.begin() + 16, colorBuf.begin() + 24, page30.begin());

    page38[2] = static_cast<uint8_t>(((newState.ledSingleColor ? 1 : 0) << 7) |
                                     ((newState.ledSymmetry ? 1 : 0) << 6) |
                                     ((std::clamp(newState.ledDirection, 0, 3) & 0x03) << 4) |
                                     (page38[2] & 0x0F));
    page38[6] = static_cast<uint8_t>((rgbToNibbleInv(newState.userColors[6].r) << 4) |
                                     rgbToNibbleInv(newState.userColors[6].g));
    const uint8_t invBright = static_cast<uint8_t>(6 - std::clamp(newState.ledBrightness, 0, 6));
    page38[7] = static_cast<uint8_t>(((invBright & 0x07) << 4) |
                                     rgbToNibbleInv(newState.userColors[6].b));

    // 5. Write pages with live progress & log
    const uint8_t initByte = 0x03;
    bool ok = true;

    auto stepWrite = [&](int pct, uint16_t addr, const uint8_t *ptr, size_t len, const std::string &desc) {
        if (!ok) return;
        report(progressCb, pct, "[WRITE " + hexAddr(addr) + "] " + desc);
        ok = writeBytes(fd, addr, ptr, len);
    };

    stepWrite(24, 0x0001, &initByte, 1, "Header & profile flag");
    stepWrite(30, 0x0008, page08.data(), page08.size(), "Primary button map (Profile 0)");
    stepWrite(36, 0x0010, page10.data(), page10.size(), "DPI stages table");
    stepWrite(42, 0x0018, page18.data(), page18.size(), "Active DPI, RGB mode, speed & Fire Key");
    stepWrite(48, 0x0020, page20.data(), page20.size(), "RGB palette block 1/3");
    stepWrite(54, 0x0028, page28.data(), page28.size(), "RGB palette block 2/3");
    stepWrite(60, 0x0030, page30.data(), page30.size(), "RGB palette block 3/3 & extended mode");
    stepWrite(66, 0x0038, page38.data(), page38.size(), "Brightness, direction, symmetry & color 7");
    stepWrite(70, 0x0040, page40.data(), page40.size(), "Button map mirror (Profile 1a)");
    stepWrite(74, 0x0048, page48.data(), page48.size(), "Button map mirror (Profile 1b)");
    stepWrite(77, 0x0050, page50.data(), page50.size(), "Button map mirror (Profile 2)");
    stepWrite(80, 0x0060, page60.data(), page60.size(), "Button map mirror (Profile 3)");
    stepWrite(83, 0x0070, page70.data(), page70.size(), "Active profile pointer (0x0076)");

    if (ok && hasAnyDescriptor) {
        report(progressCb, 86, "[MACRO/MEDIA] Writing descriptor table (0x00A0..0x00EF)...");
        for (size_t p = 0; p < 10 && ok; ++p) {
            ok = writeBytes(fd, static_cast<uint16_t>(0x00A0 + p * 8), macroDescBuf.data() + p * 8, 8);
        }
        int mIdx = 0;
        const int totalM = std::max<int>(1, static_cast<int>(macroPayloads.size()));
        for (const auto &chunk : macroPayloads) {
            if (!ok) break;
            int pct = 88 + (mIdx * 7) / totalM;
            report(progressCb, pct,
                   "[MACRO] Slot " + std::to_string(chunk.slot) + " -> " + hexAddr(chunk.addr) +
                   " (" + std::to_string(chunk.stepCount) + " steps, " +
                   std::to_string(chunk.bytes.size()) + " bytes)");
            for (size_t off = 0; off < chunk.bytes.size() && ok; off += 8) {
                size_t n = std::min<size_t>(8, chunk.bytes.size() - off);
                ok = writeBytes(fd, static_cast<uint16_t>(chunk.addr + off), chunk.bytes.data() + off, n);
            }
            ++mIdx;
        }
    }

    if (ok) {
        report(progressCb, 96, "[COMMIT] Applying changes to Instant A725 firmware (0x10)...");
        ok = applyChanges(fd);
    }


    if (!ok) {
        errCode = DriverError::WriteFailed;
        report(progressCb, 100, "[ERROR] EEPROM write or commit failed.");
        return false;
    }

    report(progressCb, 100, "[OK] All settings saved and applied to mouse.");
    return true;
}
