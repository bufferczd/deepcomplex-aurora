#include "ProfileStorage.h"
#include "../driver/HidKeyMap.h"
#include "../system/AsyncProcess.h"

#include <QColor>
#include <QDir>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>
#include <QVariantMap>
#include <algorithm>

namespace ProfileStorage {

namespace {
constexpr qint64 kMaxBackupBytes = 4 * 1024 * 1024;
constexpr qint64 kMaxSingleMacroBytes = 512 * 1024;
constexpr int kMaxMacroLibraryEntries = 500;
constexpr int kMaxMacroSteps = 120;

bool writeJsonAtomic(const QString &path, const QJsonObject &obj, bool allowDirectFallback) {
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Indented);
    QSaveFile file(path);
    if (allowDirectFallback) file.setDirectWriteFallback(true);
    if (!file.open(QIODevice::WriteOnly)) { file.cancelWriting(); return false; }
    if (!allowDirectFallback) file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    if (file.write(payload) != payload.size() || !file.commit()) {
        file.cancelWriting();
        return false;
    }
    return true;
}
} // namespace

QString configFilePath() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/profiles.json");
}

MouseProfile defaultProfileTemplate(int index, const QString &language) {
    MouseProfile p;
    MouseState st;

    switch (index) {
    case 0:
        p.name = SystemUtils::locText(language, QStringLiteral("افتراضي"), QStringLiteral("Default"), QStringLiteral("По умолчанию"), QStringLiteral("默认"));
        st.activeDpiLevel = 2;
        st.dpiIndices = {3, 5, 10, 13, 15, 16};
        st.ledMode = 0x02;
        st.ledSpeed = 3;
        st.ledBrightness = 6;
        break;
    case 1:
        p.name = SystemUtils::locText(language, QStringLiteral("ألعاب FPS"), QStringLiteral("FPS Gaming"), QStringLiteral("FPS Игры"), QStringLiteral("FPS 游戏"));
        st.activeDpiLevel = 2;
        st.dpiIndices = {1, 3, 5, 7, 10, 11};
        st.dpiColors = {{{0, 255, 102}, {255, 0, 68}, {0, 204, 255}, {255, 221, 0}, {170, 0, 255}, {255, 255, 255}}};
        st.ledMode = 0x02;
        st.ledBrightness = 6;
        st.fireTimeMs = 24;
        break;
    case 2:
        p.name = SystemUtils::locText(language, QStringLiteral("مكتبي"), QStringLiteral("Office"), QStringLiteral("Офис"), QStringLiteral("办公"));
        st.activeDpiLevel = 3;
        st.dpiIndices = {3, 5, 7, 10, 11, 13};
        st.dpiColors = {{{0, 255, 102}, {0, 204, 255}, {255, 255, 255}, {0, 255, 204}, {102, 170, 255}, {170, 255, 0}}};
        st.ledMode = 0x00;
        st.ledSpeed = 1;
        st.ledBrightness = 3;
        break;
    case 3:
        p.name = SystemUtils::locText(language, QStringLiteral("تفاعلي"), QStringLiteral("Reactive"), QStringLiteral("Реактивный"), QStringLiteral("按键触发"));
        st.activeDpiLevel = 2;
        st.dpiIndices = {3, 5, 10, 13, 15, 16};
        st.ledMode = 0x0C;
        st.reactiveEffect = 1;
        st.ledSpeed = 4;
        st.ledBrightness = 6;
        break;
    case 4:
    default:
        p.name = SystemUtils::locText(language, QStringLiteral("ليلي / هادئ"), QStringLiteral("Night"), QStringLiteral("Ночной"), QStringLiteral("夜间静音"));
        st.activeDpiLevel = 2;
        st.dpiIndices = {3, 5, 7, 10, 13, 15};
        st.ledMode = 0x06;
        st.ledSpeed = 1;
        st.ledBrightness = 1;
        break;
    }

    p.state = st;
    return p;
}

std::vector<NamedMacro> defaultMacroLibrary() {
    std::vector<NamedMacro> lib;
    auto addPreset = [&](const QString &name, uint8_t mod, uint8_t key) {
        NamedMacro nm;
        nm.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        nm.name = name;
        nm.macro.repeatMode = 0;
        nm.macro.repeatCount = 1;
        if (mod != 0) nm.macro.steps.push_back({mod, true, 24, HidKeyMap::hidUsageName(mod).toStdString()});
        nm.macro.steps.push_back({key, true, 24, HidKeyMap::hidUsageName(key).toStdString()});
        nm.macro.steps.push_back({key, false, 24, HidKeyMap::hidUsageName(key).toStdString()});
        if (mod != 0) nm.macro.steps.push_back({mod, false, 24, HidKeyMap::hidUsageName(mod).toStdString()});
        lib.push_back(std::move(nm));
    };

    addPreset(QStringLiteral("Copy (Ctrl+C)"), 0xE0, 0x06);
    addPreset(QStringLiteral("Paste (Ctrl+V)"), 0xE0, 0x19);
    addPreset(QStringLiteral("Undo (Ctrl+Z)"), 0xE0, 0x1D);
    addPreset(QStringLiteral("Switch Window (Alt+Tab)"), 0xE2, 0x2B);
    addPreset(QStringLiteral("Show Desktop (Super+D)"), 0xE3, 0x07);
    addPreset(QStringLiteral("Screenshot (PrtSc)"), 0x00, 0x46);
    return lib;
}

std::vector<MacroStep> parseMacroStepsFromJson(const QJsonArray &stepsArr) {
    std::vector<MacroStep> out;
    const int count = std::min<int>(kMaxMacroSteps, stepsArr.size());
    out.reserve(static_cast<size_t>(count));
    for (int s = 0; s < count; ++s) {
        const QJsonObject sObj = stepsArr[s].toObject();
        MacroStep step;
        step.hidCode = static_cast<uint8_t>(std::clamp(sObj.value(QStringLiteral("hidCode")).toInt(0), 0, 255));
        step.isDown = sObj.value(QStringLiteral("isDown")).toBool(true);
        step.delayMs = std::clamp(sObj.value(QStringLiteral("delayMs")).toInt(24), 8, 1016);
        const QString lbl = sObj.value(QStringLiteral("label")).toString();
        step.label = (lbl.isEmpty() ? HidKeyMap::hidUsageName(step.hidCode) : lbl).toStdString();
        if (step.hidCode != 0) out.push_back(std::move(step));
    }
    return out;
}

std::vector<MacroStep> parseMacroStepsFromVariant(const QVariantList &steps) {
    std::vector<MacroStep> out;
    const int count = std::min<int>(kMaxMacroSteps, steps.size());
    out.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        const QVariantMap m = steps[i].toMap();
        MacroStep step;
        step.hidCode = static_cast<uint8_t>(std::clamp(m.value(QStringLiteral("hidCode")).toInt(), 0, 255));
        step.isDown = m.value(QStringLiteral("isDown"), true).toBool();
        step.delayMs = std::clamp(m.value(QStringLiteral("delayMs"), 24).toInt(), 8, 1016);
        const QString lbl = m.value(QStringLiteral("label")).toString();
        step.label = (lbl.isEmpty() ? HidKeyMap::hidUsageName(step.hidCode) : lbl).toStdString();
        if (step.hidCode != 0) out.push_back(std::move(step));
    }
    return out;
}

QJsonArray macroStepsToJson(const std::vector<MacroStep> &steps) {
    QJsonArray arr;
    for (const auto &step : steps) {
        QJsonObject sObj;
        sObj[QStringLiteral("hidCode")] = static_cast<int>(step.hidCode);
        sObj[QStringLiteral("isDown")] = step.isDown;
        sObj[QStringLiteral("delayMs")] = step.delayMs;
        sObj[QStringLiteral("label")] = QString::fromStdString(step.label);
        arr.append(sObj);
    }
    return arr;
}

QJsonObject stateToJson(const MouseState &st) {
    QJsonObject obj;
    obj[QStringLiteral("activeDpiLevel")] = st.activeDpiLevel;
    obj[QStringLiteral("fireTimeMs")] = st.fireTimeMs;
    obj[QStringLiteral("ledMode")] = st.ledMode;
    obj[QStringLiteral("ledSpeed")] = st.ledSpeed;
    obj[QStringLiteral("ledBrightness")] = st.ledBrightness;
    obj[QStringLiteral("ledDirection")] = st.ledDirection;
    obj[QStringLiteral("ledSymmetry")] = st.ledSymmetry;
    obj[QStringLiteral("ledSingleColor")] = st.ledSingleColor;
    obj[QStringLiteral("userColorEnabled")] = st.userColorEnabled;
    obj[QStringLiteral("reactiveEffect")] = st.reactiveEffect;

    QJsonArray btnArr, reactBtnArr, macrosArr, mediaArr;
    for (int i = 0; i < 8; ++i) {
        btnArr.append(st.buttonCodes[i]);
        reactBtnArr.append(st.reactiveButtons[i]);
        mediaArr.append(static_cast<int>(st.buttonMediaUsages[i]));

        QJsonObject mObj;
        mObj[QStringLiteral("repeatMode")] = st.buttonMacros[i].repeatMode;
        mObj[QStringLiteral("repeatCount")] = st.buttonMacros[i].repeatCount;
        mObj[QStringLiteral("steps")] = macroStepsToJson(st.buttonMacros[i].steps);
        macrosArr.append(mObj);
    }
    obj[QStringLiteral("buttonCodes")] = btnArr;
    obj[QStringLiteral("buttonMediaUsages")] = mediaArr;
    obj[QStringLiteral("reactiveButtons")] = reactBtnArr;
    obj[QStringLiteral("buttonMacros")] = macrosArr;

    QJsonArray dpiIdxArr, dpiEnArr, dpiColArr;
    for (int i = 0; i < 6; ++i) {
        dpiIdxArr.append(st.dpiIndices[i]);
        dpiEnArr.append(st.dpiEnabled[i]);
        dpiColArr.append(QColor(st.dpiColors[i].r, st.dpiColors[i].g, st.dpiColors[i].b).name(QColor::HexRgb));
    }
    obj[QStringLiteral("dpiIndices")] = dpiIdxArr;
    obj[QStringLiteral("dpiEnabled")] = dpiEnArr;
    obj[QStringLiteral("dpiColors")] = dpiColArr;

    QJsonArray userColArr, userSlotEnArr;
    for (int i = 0; i < 7; ++i) {
        userColArr.append(QColor(st.userColors[i].r, st.userColors[i].g, st.userColors[i].b).name(QColor::HexRgb));
        userSlotEnArr.append(st.userColorSlotsEnabled[i]);
    }
    obj[QStringLiteral("userColors")] = userColArr;
    obj[QStringLiteral("userColorSlotsEnabled")] = userSlotEnArr;

    QJsonArray extraColArr;
    for (int i = 0; i < 6; ++i) {
        extraColArr.append(QColor(st.extraModeColors[i].r, st.extraModeColors[i].g, st.extraModeColors[i].b).name(QColor::HexRgb));
    }
    obj[QStringLiteral("extraModeColors")] = extraColArr;
    return obj;
}

MouseState jsonToState(const QJsonObject &obj, const MouseState &fallback) {
    MouseState st = fallback;
    st.activeDpiLevel = std::clamp(obj.value(QStringLiteral("activeDpiLevel")).toInt(st.activeDpiLevel), 1, 6);
    st.fireTimeMs = std::clamp(obj.value(QStringLiteral("fireTimeMs")).toInt(st.fireTimeMs), 8, 400);
    st.ledMode = std::clamp(obj.value(QStringLiteral("ledMode")).toInt(st.ledMode), 0, 13);
    st.ledSpeed = std::clamp(obj.value(QStringLiteral("ledSpeed")).toInt(st.ledSpeed), 0, 5);
    st.ledBrightness = std::clamp(obj.value(QStringLiteral("ledBrightness")).toInt(st.ledBrightness), 0, 6);
    st.ledDirection = std::clamp(obj.value(QStringLiteral("ledDirection")).toInt(st.ledDirection), 0, 3);
    st.ledSymmetry = obj.value(QStringLiteral("ledSymmetry")).toBool(st.ledSymmetry);
    st.ledSingleColor = obj.value(QStringLiteral("ledSingleColor")).toBool(st.ledSingleColor);
    st.userColorEnabled = obj.value(QStringLiteral("userColorEnabled")).toBool(st.userColorEnabled);
    st.reactiveEffect = std::clamp(obj.value(QStringLiteral("reactiveEffect")).toInt(st.reactiveEffect), 0, 3);

    const QJsonArray btnArr = obj.value(QStringLiteral("buttonCodes")).toArray();
    for (int i = 0; i < std::min<int>(8, btnArr.size()); ++i) {
        st.buttonCodes[i] = std::clamp(btnArr[i].toInt(st.buttonCodes[i]), 0, 31);
    }

    const QJsonArray mediaArr = obj.value(QStringLiteral("buttonMediaUsages")).toArray();
    for (int i = 0; i < std::min<int>(8, mediaArr.size()); ++i) {
        st.buttonMediaUsages[i] = static_cast<uint16_t>(std::clamp(mediaArr[i].toInt(0), 0, 65535));
    }

    const QJsonArray reactBtnArr = obj.value(QStringLiteral("reactiveButtons")).toArray();
    for (int i = 0; i < std::min<int>(8, reactBtnArr.size()); ++i) {
        st.reactiveButtons[i] = reactBtnArr[i].toBool(st.reactiveButtons[i]);
    }

    const QJsonArray macrosArr = obj.value(QStringLiteral("buttonMacros")).toArray();
    for (int i = 0; i < std::min<int>(8, macrosArr.size()); ++i) {
        const QJsonObject mObj = macrosArr[i].toObject();
        ButtonMacro mac;
        mac.repeatMode = std::clamp(mObj.value(QStringLiteral("repeatMode")).toInt(0), 0, 2);
        mac.repeatCount = std::clamp(mObj.value(QStringLiteral("repeatCount")).toInt(1), 1, 255);
        mac.steps = parseMacroStepsFromJson(mObj.value(QStringLiteral("steps")).toArray());
        st.buttonMacros[i] = std::move(mac);
    }

    const QJsonArray dpiIdxArr = obj.value(QStringLiteral("dpiIndices")).toArray();
    for (int i = 0; i < std::min<int>(6, dpiIdxArr.size()); ++i) {
        st.dpiIndices[i] = std::clamp(dpiIdxArr[i].toInt(st.dpiIndices[i]), 0, 16);
    }

    const QJsonArray dpiEnArr = obj.value(QStringLiteral("dpiEnabled")).toArray();
    for (int i = 0; i < std::min<int>(6, dpiEnArr.size()); ++i) {
        st.dpiEnabled[i] = dpiEnArr[i].toBool(st.dpiEnabled[i]);
    }

    const QJsonArray dpiColArr = obj.value(QStringLiteral("dpiColors")).toArray();
    for (int i = 0; i < std::min<int>(6, dpiColArr.size()); ++i) {
        QColor c(dpiColArr[i].toString());
        if (c.isValid()) st.dpiColors[i] = {c.red(), c.green(), c.blue()};
    }

    const QJsonArray userColArr = obj.value(QStringLiteral("userColors")).toArray();
    for (int i = 0; i < std::min<int>(7, userColArr.size()); ++i) {
        QColor c(userColArr[i].toString());
        if (c.isValid()) st.userColors[i] = {c.red(), c.green(), c.blue()};
    }

    const QJsonArray userSlotEnArr = obj.value(QStringLiteral("userColorSlotsEnabled")).toArray();
    for (int i = 0; i < std::min<int>(7, userSlotEnArr.size()); ++i) {
        st.userColorSlotsEnabled[i] = userSlotEnArr[i].toBool(st.userColorSlotsEnabled[i]);
    }

    const QJsonArray extraColArr = obj.value(QStringLiteral("extraModeColors")).toArray();
    for (int i = 0; i < std::min<int>(6, extraColArr.size()); ++i) {
        QColor c(extraColArr[i].toString());
        if (c.isValid()) st.extraModeColors[i] = {c.red(), c.green(), c.blue()};
    }

    return st;
}

QJsonObject buildFullJson(const AppConfigData &cfg) {
    QJsonObject root;
    root[QStringLiteral("version")] = 4;
    root[QStringLiteral("language")] = cfg.language;
    root[QStringLiteral("activeProfile")] = cfg.activeProfile;
    root[QStringLiteral("autoApplyOnSwitch")] = cfg.autoApplyOnSwitch;

    QJsonObject settings;
    settings[QStringLiteral("autostartEnabled")] = cfg.autostartEnabled;
    settings[QStringLiteral("startMinimized")] = cfg.startMinimized;
    settings[QStringLiteral("minimizeToTray")] = cfg.minimizeToTray;
    settings[QStringLiteral("autoReadOnStartup")] = cfg.autoReadOnStartup;
    settings[QStringLiteral("applyOnStartup")] = cfg.applyOnStartup;
    settings[QStringLiteral("autoDetectDevice")] = cfg.autoDetectDevice;
    settings[QStringLiteral("showHiddenEffects")] = cfg.showHiddenEffects;
    settings[QStringLiteral("showHexCodes")] = cfg.showHexCodes;
    settings[QStringLiteral("enableKeyCapture")] = cfg.enableKeyCapture;
    settings[QStringLiteral("showProgressAndLog")] = cfg.showProgressAndLog;
    settings[QStringLiteral("showStartupSplash")] = cfg.showStartupSplash;
    settings[QStringLiteral("showRgbPreviewStrip")] = cfg.showRgbPreviewStrip;
    settings[QStringLiteral("animatedUi")] = cfg.animatedUi;
    settings[QStringLiteral("uiThemePreset")] = cfg.uiThemePreset;
    settings[QStringLiteral("uiAccentMode")] = cfg.uiAccentMode;
    settings[QStringLiteral("uiCustomAccent")] = cfg.uiCustomAccent;
    settings[QStringLiteral("uiGlassOpacity")] = cfg.uiGlassOpacity;
    settings[QStringLiteral("uiCornerRadius")] = cfg.uiCornerRadius;
    settings[QStringLiteral("uiGlowIntensity")] = cfg.uiGlowIntensity;
    settings[QStringLiteral("uiBackgroundStyle")] = cfg.uiBackgroundStyle;
    settings[QStringLiteral("uiTrueBlackOled")] = cfg.uiTrueBlackOled;
    settings[QStringLiteral("uiBorderWidth")] = cfg.uiBorderWidth;
    settings[QStringLiteral("uiAnimationSpeed")] = cfg.uiAnimationSpeed;
    settings[QStringLiteral("uiCompactMode")] = cfg.uiCompactMode;
    settings[QStringLiteral("uiUseCustomSecondary")] = cfg.uiUseCustomSecondary;
    settings[QStringLiteral("uiCustomSecondary")] = cfg.uiCustomSecondary;
    settings[QStringLiteral("uiBorderStyle")] = cfg.uiBorderStyle;
    settings[QStringLiteral("uiCustomBorderColor")] = cfg.uiCustomBorderColor;
    settings[QStringLiteral("uiBorderOpacity")] = cfg.uiBorderOpacity;
    settings[QStringLiteral("uiWindowEdgeGlow")] = cfg.uiWindowEdgeGlow;
    settings[QStringLiteral("uiWindowButtonStyle")] = cfg.uiWindowButtonStyle;
    settings[QStringLiteral("uiTitlebarStyle")] = cfg.uiTitlebarStyle;
    settings[QStringLiteral("uiButtonShape")] = cfg.uiButtonShape;
    settings[QStringLiteral("uiButtonStyle")] = cfg.uiButtonStyle;
    settings[QStringLiteral("uiCardEdgeStyle")] = cfg.uiCardEdgeStyle;
    settings[QStringLiteral("uiWindowShape")] = cfg.uiWindowShape;
    settings[QStringLiteral("firstRunCompleted")] = cfg.firstRunCompleted;
    settings[QStringLiteral("autoSwitchByApp")] = cfg.autoSwitchByApp;
    settings[QStringLiteral("fallbackProfileIndex")] = cfg.fallbackProfileIndex;
    root[QStringLiteral("appSettings")] = settings;

    QJsonArray arr;
    for (int i = 0; i < AppConfigData::kProfileCount; ++i) {
        QJsonObject pObj;
        pObj[QStringLiteral("name")] = cfg.profiles[i].name;
        pObj[QStringLiteral("linkedApps")] = cfg.profiles[i].linkedApps;
        pObj[QStringLiteral("state")] = stateToJson(cfg.profiles[i].state);
        QJsonArray midArr;
        for (int s = 0; s < 8; ++s) midArr.append(cfg.profiles[i].buttonMacroIds[s]);
        pObj[QStringLiteral("buttonMacroIds")] = midArr;
        arr.append(pObj);
    }
    root[QStringLiteral("profiles")] = arr;

    QJsonArray libArr;
    for (const auto &nm : cfg.macroLibrary) {
        QJsonObject mObj;
        mObj[QStringLiteral("id")] = nm.id;
        mObj[QStringLiteral("name")] = nm.name;
        mObj[QStringLiteral("repeatMode")] = nm.macro.repeatMode;
        mObj[QStringLiteral("repeatCount")] = nm.macro.repeatCount;
        mObj[QStringLiteral("steps")] = macroStepsToJson(nm.macro.steps);
        libArr.append(mObj);
    }
    root[QStringLiteral("macroLibrary")] = libArr;
    return root;
}

bool applyFullJson(const QJsonObject &root, AppConfigData &cfg) {
    if (!root.contains(QStringLiteral("profiles")) || !root.value(QStringLiteral("profiles")).isArray()) return false;

    const QString savedLang = root.value(QStringLiteral("language")).toString();
    if (savedLang == QStringLiteral("ar") || savedLang == QStringLiteral("en") ||
        savedLang == QStringLiteral("ru") || savedLang == QStringLiteral("zh")) {
        cfg.language = savedLang;
    }

    cfg.activeProfile = std::clamp(root.value(QStringLiteral("activeProfile")).toInt(0), 0, AppConfigData::kProfileCount - 1);
    cfg.autoApplyOnSwitch = root.value(QStringLiteral("autoApplyOnSwitch")).toBool(false);

    if (root.contains(QStringLiteral("appSettings")) && root.value(QStringLiteral("appSettings")).isObject()) {
        const QJsonObject s = root.value(QStringLiteral("appSettings")).toObject();
        cfg.autostartEnabled = s.value(QStringLiteral("autostartEnabled")).toBool(cfg.autostartEnabled);
        cfg.startMinimized = s.value(QStringLiteral("startMinimized")).toBool(cfg.startMinimized);
        cfg.minimizeToTray = s.value(QStringLiteral("minimizeToTray")).toBool(cfg.minimizeToTray);
        cfg.autoReadOnStartup = s.value(QStringLiteral("autoReadOnStartup")).toBool(cfg.autoReadOnStartup);
        cfg.applyOnStartup = s.value(QStringLiteral("applyOnStartup")).toBool(cfg.applyOnStartup);
        if (cfg.autoReadOnStartup && cfg.applyOnStartup) cfg.applyOnStartup = false;
        cfg.autoDetectDevice = s.value(QStringLiteral("autoDetectDevice")).toBool(cfg.autoDetectDevice);
        cfg.showHiddenEffects = s.value(QStringLiteral("showHiddenEffects")).toBool(cfg.showHiddenEffects);
        cfg.showHexCodes = s.value(QStringLiteral("showHexCodes")).toBool(cfg.showHexCodes);
        cfg.enableKeyCapture = s.value(QStringLiteral("enableKeyCapture")).toBool(cfg.enableKeyCapture);
        cfg.showProgressAndLog = s.value(QStringLiteral("showProgressAndLog")).toBool(cfg.showProgressAndLog);
        cfg.showStartupSplash = s.value(QStringLiteral("showStartupSplash")).toBool(cfg.showStartupSplash);
        cfg.showRgbPreviewStrip = s.value(QStringLiteral("showRgbPreviewStrip")).toBool(cfg.showRgbPreviewStrip);
        cfg.animatedUi = s.value(QStringLiteral("animatedUi")).toBool(cfg.animatedUi);
        {
            static const QStringList kValidPresets = {
                QStringLiteral("oled_pure"), QStringLiteral("oled_cyan"), QStringLiteral("oled_crimson"),
                QStringLiteral("oled_violet"), QStringLiteral("oled_gold"), QStringLiteral("oled_ice"),
                QStringLiteral("oled_magenta"), QStringLiteral("oled_mono"),
                QStringLiteral("aurora"), QStringLiteral("cyber"), QStringLiteral("emerald"),
                QStringLiteral("crimson"), QStringLiteral("amethyst"), QStringLiteral("ocean"),
                QStringLiteral("solar"), QStringLiteral("sakura"), QStringLiteral("nord"),
                QStringLiteral("dracula"), QStringLiteral("frost"), QStringLiteral("sand")
            };
            const QString preset = s.value(QStringLiteral("uiThemePreset")).toString(cfg.uiThemePreset);
            if (kValidPresets.contains(preset)) {
                cfg.uiThemePreset = preset;
            }
            const QString aMode = s.value(QStringLiteral("uiAccentMode")).toString(cfg.uiAccentMode);
            if (aMode == QStringLiteral("autoDpi") || aMode == QStringLiteral("preset") || aMode == QStringLiteral("custom")) {
                cfg.uiAccentMode = aMode;
            }
            const QColor customCol(s.value(QStringLiteral("uiCustomAccent")).toString(cfg.uiCustomAccent));
            if (customCol.isValid()) cfg.uiCustomAccent = customCol.name(QColor::HexRgb);
            cfg.uiGlassOpacity = std::clamp(s.value(QStringLiteral("uiGlassOpacity")).toDouble(cfg.uiGlassOpacity), 0.35, 0.98);
            cfg.uiCornerRadius = std::clamp(s.value(QStringLiteral("uiCornerRadius")).toInt(cfg.uiCornerRadius), 0, 24);
            cfg.uiGlowIntensity = std::clamp(s.value(QStringLiteral("uiGlowIntensity")).toDouble(cfg.uiGlowIntensity), 0.0, 1.0);
            const QString bgStyle = s.value(QStringLiteral("uiBackgroundStyle")).toString(cfg.uiBackgroundStyle);
            if (bgStyle == QStringLiteral("aurora") || bgStyle == QStringLiteral("waves") ||
                bgStyle == QStringLiteral("grid") || bgStyle == QStringLiteral("solid") ||
                bgStyle == QStringLiteral("oledPure")) {
                cfg.uiBackgroundStyle = bgStyle;
            }
            cfg.uiTrueBlackOled = s.value(QStringLiteral("uiTrueBlackOled")).toBool(cfg.uiTrueBlackOled);
            cfg.uiBorderWidth = std::clamp(s.value(QStringLiteral("uiBorderWidth")).toDouble(cfg.uiBorderWidth), 0.0, 3.0);
            cfg.uiAnimationSpeed = std::clamp(s.value(QStringLiteral("uiAnimationSpeed")).toDouble(cfg.uiAnimationSpeed), 0.4, 2.0);
            cfg.uiCompactMode = s.value(QStringLiteral("uiCompactMode")).toBool(cfg.uiCompactMode);
            cfg.uiUseCustomSecondary = s.value(QStringLiteral("uiUseCustomSecondary")).toBool(cfg.uiUseCustomSecondary);
            const QColor secCol(s.value(QStringLiteral("uiCustomSecondary")).toString(cfg.uiCustomSecondary));
            if (secCol.isValid()) cfg.uiCustomSecondary = secCol.name(QColor::HexRgb);
            const QString bStyle = s.value(QStringLiteral("uiBorderStyle")).toString(cfg.uiBorderStyle);
            if (bStyle == QStringLiteral("accent") || bStyle == QStringLiteral("gradient") ||
                bStyle == QStringLiteral("rainbow") || bStyle == QStringLiteral("subtle") ||
                bStyle == QStringLiteral("custom") || bStyle == QStringLiteral("none")) {
                cfg.uiBorderStyle = bStyle;
            }
            const QColor bCol(s.value(QStringLiteral("uiCustomBorderColor")).toString(cfg.uiCustomBorderColor));
            if (bCol.isValid()) cfg.uiCustomBorderColor = bCol.name(QColor::HexRgb);
            cfg.uiBorderOpacity = std::clamp(s.value(QStringLiteral("uiBorderOpacity")).toDouble(cfg.uiBorderOpacity), 0.10, 1.0);
            cfg.uiWindowEdgeGlow = s.value(QStringLiteral("uiWindowEdgeGlow")).toBool(cfg.uiWindowEdgeGlow);
            const QString wbStyle = s.value(QStringLiteral("uiWindowButtonStyle")).toString(cfg.uiWindowButtonStyle);
            if (wbStyle == QStringLiteral("circle") || wbStyle == QStringLiteral("pill") || wbStyle == QStringLiteral("traffic")) {
                cfg.uiWindowButtonStyle = wbStyle;
            }
            const QString tbStyle = s.value(QStringLiteral("uiTitlebarStyle")).toString(cfg.uiTitlebarStyle);
            if (tbStyle == QStringLiteral("glass") || tbStyle == QStringLiteral("accent") ||
                tbStyle == QStringLiteral("transparent") || tbStyle == QStringLiteral("oled")) {
                cfg.uiTitlebarStyle = tbStyle;
            }
            const QString btnShape = s.value(QStringLiteral("uiButtonShape")).toString(cfg.uiButtonShape);
            if (btnShape == QStringLiteral("rounded") || btnShape == QStringLiteral("pill") ||
                btnShape == QStringLiteral("sharp") || btnShape == QStringLiteral("squircle") ||
                btnShape == QStringLiteral("leaf") || btnShape == QStringLiteral("chamfer")) {
                cfg.uiButtonShape = btnShape;
            }
            const QString btnStyle = s.value(QStringLiteral("uiButtonStyle")).toString(cfg.uiButtonStyle);
            if (btnStyle == QStringLiteral("glass") || btnStyle == QStringLiteral("solid") ||
                btnStyle == QStringLiteral("outline") || btnStyle == QStringLiteral("neon") ||
                btnStyle == QStringLiteral("gradient") || btnStyle == QStringLiteral("soft")) {
                cfg.uiButtonStyle = btnStyle;
            }
            const QString ceStyle = s.value(QStringLiteral("uiCardEdgeStyle")).toString(cfg.uiCardEdgeStyle);
            if (ceStyle == QStringLiteral("glass") || ceStyle == QStringLiteral("neon") ||
                ceStyle == QStringLiteral("gradient") || ceStyle == QStringLiteral("chamfer") ||
                ceStyle == QStringLiteral("flat")) {
                cfg.uiCardEdgeStyle = ceStyle;
            }
            const QString wShape = s.value(QStringLiteral("uiWindowShape")).toString(cfg.uiWindowShape);
            if (wShape == QStringLiteral("sharp") || wShape == QStringLiteral("rounded") ||
                wShape == QStringLiteral("extra") || wShape == QStringLiteral("chamfer")) {
                cfg.uiWindowShape = wShape;
            }
        }
        cfg.firstRunCompleted = s.value(QStringLiteral("firstRunCompleted")).toBool(cfg.firstRunCompleted);
        cfg.autoSwitchByApp = s.value(QStringLiteral("autoSwitchByApp")).toBool(cfg.autoSwitchByApp);
        cfg.fallbackProfileIndex = std::clamp(s.value(QStringLiteral("fallbackProfileIndex")).toInt(0), 0, AppConfigData::kProfileCount - 1);
    }

    const QJsonArray arr = root.value(QStringLiteral("profiles")).toArray();
    for (int i = 0; i < std::min<int>(AppConfigData::kProfileCount, arr.size()); ++i) {
        const QJsonObject pObj = arr[i].toObject();
        const QString savedName = pObj.value(QStringLiteral("name")).toString().trimmed();
        if (!savedName.isEmpty()) cfg.profiles[i].name = savedName;
        cfg.profiles[i].linkedApps = pObj.value(QStringLiteral("linkedApps")).toString().trimmed();
        if (pObj.contains(QStringLiteral("state")) && pObj.value(QStringLiteral("state")).isObject()) {
            cfg.profiles[i].state = jsonToState(pObj.value(QStringLiteral("state")).toObject(), cfg.profiles[i].state);
        }
        const QJsonArray midArr = pObj.value(QStringLiteral("buttonMacroIds")).toArray();
        for (int s = 0; s < std::min<int>(8, midArr.size()); ++s) {
            cfg.profiles[i].buttonMacroIds[s] = midArr[s].toString();
        }
    }

    if (root.contains(QStringLiteral("macroLibrary")) && root.value(QStringLiteral("macroLibrary")).isArray()) {
        cfg.macroLibrary.clear();
        const QJsonArray libArr = root.value(QStringLiteral("macroLibrary")).toArray();
        int importedCount = 0;
        for (const auto &val : libArr) {
            if (++importedCount > kMaxMacroLibraryEntries) break;
            const QJsonObject mObj = val.toObject();
            NamedMacro nm;
            nm.id = mObj.value(QStringLiteral("id")).toString();
            if (nm.id.isEmpty()) nm.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            nm.name = mObj.value(QStringLiteral("name")).toString().trimmed();
            if (nm.name.isEmpty()) nm.name = QStringLiteral("Macro");
            nm.macro.repeatMode = std::clamp(mObj.value(QStringLiteral("repeatMode")).toInt(0), 0, 2);
            nm.macro.repeatCount = std::clamp(mObj.value(QStringLiteral("repeatCount")).toInt(1), 1, 255);
            nm.macro.steps = parseMacroStepsFromJson(mObj.value(QStringLiteral("steps")).toArray());
            cfg.macroLibrary.push_back(std::move(nm));
        }
    }
    return true;
}

bool loadFromDisk(AppConfigData &cfg) {
    QByteArray raw;
    if (!SystemUtils::readRegularFileBounded(configFilePath(), kMaxBackupBytes, raw)) return false;
    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    return doc.isObject() && applyFullJson(doc.object(), cfg);
}

bool saveToDisk(const AppConfigData &cfg) {
    return writeJsonAtomic(configFilePath(), buildFullJson(cfg), false);
}

bool exportBackup(const QString &localPath, const AppConfigData &cfg) {
    return writeJsonAtomic(localPath, buildFullJson(cfg), true);
}

bool importBackup(const QString &localPath, AppConfigData &cfg) {
    QByteArray raw;
    if (!SystemUtils::readRegularFileBounded(localPath, kMaxBackupBytes, raw)) return false;
    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    return doc.isObject() && applyFullJson(doc.object(), cfg);
}

bool exportSingleMacro(const QString &localPath, const NamedMacro &macro) {
    QJsonObject obj;
    obj[QStringLiteral("deepcomplexMacro")] = 1;
    obj[QStringLiteral("name")] = macro.name;
    obj[QStringLiteral("repeatMode")] = macro.macro.repeatMode;
    obj[QStringLiteral("repeatCount")] = macro.macro.repeatCount;
    obj[QStringLiteral("steps")] = macroStepsToJson(macro.macro.steps);
    return writeJsonAtomic(localPath, obj, true);
}

bool importSingleMacro(const QString &localPath, NamedMacro &outMacro) {
    QByteArray raw;
    if (!SystemUtils::readRegularFileBounded(localPath, kMaxSingleMacroBytes, raw)) return false;
    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    if (!doc.isObject()) return false;

    const QJsonObject obj = doc.object();
    const QJsonArray stepsArr = obj.value(QStringLiteral("steps")).toArray();
    if (stepsArr.isEmpty()) return false;

    NamedMacro nm;
    nm.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    nm.name = obj.value(QStringLiteral("name")).toString().trimmed().left(80);
    if (nm.name.isEmpty()) nm.name = QStringLiteral("Imported Macro");
    nm.macro.repeatMode = std::clamp(obj.value(QStringLiteral("repeatMode")).toInt(0), 0, 2);
    nm.macro.repeatCount = std::clamp(obj.value(QStringLiteral("repeatCount")).toInt(1), 1, 255);
    nm.macro.steps = parseMacroStepsFromJson(stepsArr);
    if (nm.macro.steps.empty()) return false;
    outMacro = std::move(nm);
    return true;
}

} // namespace ProfileStorage
