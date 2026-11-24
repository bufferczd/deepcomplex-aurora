#pragma once

#include "../driver/M700ADriver.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVariantList>
#include <array>
#include <vector>

struct NamedMacro {
    QString id;
    QString name;
    ButtonMacro macro;
};

struct MouseProfile {
    QString name;
    QString linkedApps;
    MouseState state;
    std::array<QString, 8> buttonMacroIds;
};

struct AppConfigData {
    static constexpr int kProfileCount = 5;

    QString language = QStringLiteral("ar");
    int activeProfile = 0;
    bool autoApplyOnSwitch = false;

    bool autostartEnabled = false;
    bool startMinimized = false;
    bool minimizeToTray = true;
    bool autoReadOnStartup = true;
    bool applyOnStartup = false;
    bool autoDetectDevice = true;
    bool showHiddenEffects = false;
    bool showHexCodes = false;
    bool enableKeyCapture = true;
    bool showProgressAndLog = true;
    bool showStartupSplash = true;
    bool showRgbPreviewStrip = true;
    bool animatedUi = true;
    QString uiThemePreset = QStringLiteral("oled_pure");
    QString uiAccentMode = QStringLiteral("preset");
    QString uiCustomAccent = QStringLiteral("#15803d");
    double uiGlassOpacity = 0.84;
    int uiCornerRadius = 14;
    double uiGlowIntensity = 0.65;
    QString uiBackgroundStyle = QStringLiteral("oledPure");
    bool uiTrueBlackOled = true;
    double uiBorderWidth = 1.4;
    double uiAnimationSpeed = 1.0;
    bool uiCompactMode = false;
    bool uiUseCustomSecondary = false;
    QString uiCustomSecondary = QStringLiteral("#166534");
    QString uiBorderStyle = QStringLiteral("accent");
    QString uiCustomBorderColor = QStringLiteral("#15803d");
    double uiBorderOpacity = 0.85;
    bool uiWindowEdgeGlow = true;
    QString uiWindowButtonStyle = QStringLiteral("circle");
    QString uiTitlebarStyle = QStringLiteral("oled");
    QString uiButtonShape = QStringLiteral("rounded");
    QString uiButtonStyle = QStringLiteral("glass");
    QString uiCardEdgeStyle = QStringLiteral("glass");
    QString uiWindowShape = QStringLiteral("rounded");
    bool firstRunCompleted = false;
    bool autoSwitchByApp = false;
    int fallbackProfileIndex = 0;

    std::array<MouseProfile, kProfileCount> profiles;
    std::vector<NamedMacro> macroLibrary;
};

namespace ProfileStorage {

constexpr std::array<int, 8> kDefaultButtonCodes = {0x01, 0x02, 0x03, 0x04, 0x05, 0x0C, 0x00, 0x0B};

QString configFilePath();

MouseProfile defaultProfileTemplate(int index, const QString &language);
std::vector<NamedMacro> defaultMacroLibrary();

std::vector<MacroStep> parseMacroStepsFromJson(const QJsonArray &stepsArr);
std::vector<MacroStep> parseMacroStepsFromVariant(const QVariantList &steps);
QJsonArray macroStepsToJson(const std::vector<MacroStep> &steps);

QJsonObject stateToJson(const MouseState &st);
MouseState jsonToState(const QJsonObject &obj, const MouseState &fallback);

QJsonObject buildFullJson(const AppConfigData &cfg);
bool applyFullJson(const QJsonObject &root, AppConfigData &cfg);

bool loadFromDisk(AppConfigData &cfg);
bool saveToDisk(const AppConfigData &cfg);

bool exportBackup(const QString &localPath, const AppConfigData &cfg);
bool importBackup(const QString &localPath, AppConfigData &cfg);

bool exportSingleMacro(const QString &localPath, const NamedMacro &macro);
bool importSingleMacro(const QString &localPath, NamedMacro &outMacro);

} // namespace ProfileStorage
