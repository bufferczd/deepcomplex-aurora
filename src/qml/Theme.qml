pragma Singleton
import QtQuick

QtObject {
    id: theme

    // Live-synced properties from mouseController in Main.qml
    property string presetId: "oled_pure"
    property string accentMode: "preset" // "autoDpi", "preset", "custom"
    property color customAccent: "#15803d"
    property color dpiAccent: "#15803d"
    property real glassOpacity: 0.84
    property int cornerRadius: 14
    property real glowIntensity: 0.65
    property string backgroundStyle: "oledPure" // "aurora", "waves", "grid", "solid", "oledPure"
    property bool animatedUi: true

    // Deep customization properties
    property bool trueBlackOled: true
    property real borderWidth: 1.4
    property real animationSpeed: 1.0
    property bool compactMode: false
    property bool useCustomSecondary: false
    property color customSecondary: "#166534"
    property string borderStyle: "accent" // "accent", "gradient", "rainbow", "subtle", "custom", "none"
    property color customBorderColor: "#15803d"
    property real borderOpacity: 0.85
    property bool windowEdgeGlow: true
    property string windowButtonStyle: "circle" // "circle", "pill", "traffic"
    property string titlebarStyle: "oled" // "glass", "accent", "transparent", "oled"
    property string buttonShape: "rounded" // "rounded", "pill", "sharp", "squircle", "leaf", "chamfer"
    property string buttonStyle: "glass"   // "glass", "solid", "outline", "neon", "gradient", "soft"
    property string cardEdgeStyle: "glass" // "glass", "neon", "gradient", "chamfer", "flat"
    property string windowShape: "rounded" // "sharp", "rounded", "extra", "chamfer"

    // 20 Curated Themes: 8 True-Black OLED + 10 Modern Dark Glass + 2 Light Glass
    readonly property var presets: ({
        // === OLED Pure Black (#000000) — 8 Themes ===
        oled_pure: {
            id: "oled_pure", category: "oled", isOled: true, isLight: false,
            accent: "#15803d", orb2: "#166534", orb3: "#059669",
            bgTop: "#000000", bgMid: "#000000", bgBottom: "#000000",
            surfaceR: 0.02, surfaceG: 0.03, surfaceB: 0.02,
            sidebarR: 0.00, sidebarG: 0.00, sidebarB: 0.00
        },
        oled_cyan: {
            id: "oled_cyan", category: "oled", isOled: true, isLight: false,
            accent: "#00e5ff", orb2: "#0284c7", orb3: "#38bdf8",
            bgTop: "#000000", bgMid: "#000000", bgBottom: "#000000",
            surfaceR: 0.01, surfaceG: 0.03, surfaceB: 0.04,
            sidebarR: 0.00, sidebarG: 0.00, sidebarB: 0.00
        },
        oled_crimson: {
            id: "oled_crimson", category: "oled", isOled: true, isLight: false,
            accent: "#ff1e42", orb2: "#dc2626", orb3: "#f97316",
            bgTop: "#000000", bgMid: "#000000", bgBottom: "#000000",
            surfaceR: 0.04, surfaceG: 0.01, surfaceB: 0.02,
            sidebarR: 0.00, sidebarG: 0.00, sidebarB: 0.00
        },
        oled_violet: {
            id: "oled_violet", category: "oled", isOled: true, isLight: false,
            accent: "#a855f7", orb2: "#7c3aed", orb3: "#ec4899",
            bgTop: "#000000", bgMid: "#000000", bgBottom: "#000000",
            surfaceR: 0.03, surfaceG: 0.01, surfaceB: 0.05,
            sidebarR: 0.00, sidebarG: 0.00, sidebarB: 0.00
        },
        oled_gold: {
            id: "oled_gold", category: "oled", isOled: true, isLight: false,
            accent: "#f59e0b", orb2: "#d97706", orb3: "#fbbf24",
            bgTop: "#000000", bgMid: "#000000", bgBottom: "#000000",
            surfaceR: 0.04, surfaceG: 0.03, surfaceB: 0.01,
            sidebarR: 0.00, sidebarG: 0.00, sidebarB: 0.00
        },
        oled_ice: {
            id: "oled_ice", category: "oled", isOled: true, isLight: false,
            accent: "#3b82f6", orb2: "#6366f1", orb3: "#38bdf8",
            bgTop: "#000000", bgMid: "#000000", bgBottom: "#000000",
            surfaceR: 0.01, surfaceG: 0.02, surfaceB: 0.05,
            sidebarR: 0.00, sidebarG: 0.00, sidebarB: 0.00
        },
        oled_magenta: {
            id: "oled_magenta", category: "oled", isOled: true, isLight: false,
            accent: "#f43f5e", orb2: "#d946ef", orb3: "#8b5cf6",
            bgTop: "#000000", bgMid: "#000000", bgBottom: "#000000",
            surfaceR: 0.04, surfaceG: 0.01, surfaceB: 0.04,
            sidebarR: 0.00, sidebarG: 0.00, sidebarB: 0.00
        },
        oled_mono: {
            id: "oled_mono", category: "oled", isOled: true, isLight: false,
            accent: "#f8fafc", orb2: "#94a3b8", orb3: "#cbd5e1",
            bgTop: "#000000", bgMid: "#000000", bgBottom: "#000000",
            surfaceR: 0.03, surfaceG: 0.03, surfaceB: 0.03,
            sidebarR: 0.00, sidebarG: 0.00, sidebarB: 0.00
        },

        // === Modern Dark Glass — 10 Themes ===
        aurora: {
            id: "aurora", category: "dark", isOled: false, isLight: false,
            accent: "#38bdf8", orb2: "#7c3aed", orb3: "#06b6d4",
            bgTop: "#070B14", bgMid: "#0B1120", bgBottom: "#060912",
            surfaceR: 0.08, surfaceG: 0.12, surfaceB: 0.21,
            sidebarR: 0.05, sidebarG: 0.08, sidebarB: 0.14
        },
        cyber: {
            id: "cyber", category: "dark", isOled: false, isLight: false,
            accent: "#f43f5e", orb2: "#ec4899", orb3: "#06b6d4",
            bgTop: "#09060F", bgMid: "#11091C", bgBottom: "#07040C",
            surfaceR: 0.12, surfaceG: 0.08, surfaceB: 0.19,
            sidebarR: 0.07, sidebarG: 0.05, sidebarB: 0.12
        },
        emerald: {
            id: "emerald", category: "dark", isOled: false, isLight: false,
            accent: "#10b981", orb2: "#14b8a6", orb3: "#22c55e",
            bgTop: "#040F0D", bgMid: "#071A16", bgBottom: "#030B09",
            surfaceR: 0.06, surfaceG: 0.15, surfaceB: 0.13,
            sidebarR: 0.04, sidebarG: 0.10, sidebarB: 0.09
        },
        crimson: {
            id: "crimson", category: "dark", isOled: false, isLight: false,
            accent: "#fb7185", orb2: "#ef4444", orb3: "#f59e0b",
            bgTop: "#120709", bgMid: "#1C0B0E", bgBottom: "#0D0506",
            surfaceR: 0.17, surfaceG: 0.09, surfaceB: 0.11,
            sidebarR: 0.11, sidebarG: 0.05, sidebarB: 0.07
        },
        amethyst: {
            id: "amethyst", category: "dark", isOled: false, isLight: false,
            accent: "#a78bfa", orb2: "#8b5cf6", orb3: "#e879f9",
            bgTop: "#0B0716", bgMid: "#130D24", bgBottom: "#080511",
            surfaceR: 0.12, surfaceG: 0.09, surfaceB: 0.22,
            sidebarR: 0.08, sidebarG: 0.06, sidebarB: 0.15
        },
        ocean: {
            id: "ocean", category: "dark", isOled: false, isLight: false,
            accent: "#0ea5e9", orb2: "#0284c7", orb3: "#2dd4bf",
            bgTop: "#04101A", bgMid: "#061826", bgBottom: "#030C14",
            surfaceR: 0.05, surfaceG: 0.14, surfaceB: 0.22,
            sidebarR: 0.03, sidebarG: 0.09, sidebarB: 0.15
        },
        solar: {
            id: "solar", category: "dark", isOled: false, isLight: false,
            accent: "#f97316", orb2: "#f59e0b", orb3: "#ef4444",
            bgTop: "#130B04", bgMid: "#1C1006", bgBottom: "#0E0803",
            surfaceR: 0.17, surfaceG: 0.11, surfaceB: 0.06,
            sidebarR: 0.11, sidebarG: 0.07, sidebarB: 0.04
        },
        sakura: {
            id: "sakura", category: "dark", isOled: false, isLight: false,
            accent: "#f472b6", orb2: "#ec4899", orb3: "#a855f7",
            bgTop: "#140912", bgMid: "#1E0E1B", bgBottom: "#0F060D",
            surfaceR: 0.17, surfaceG: 0.09, surfaceB: 0.15,
            sidebarR: 0.11, sidebarG: 0.06, sidebarB: 0.10
        },
        nord: {
            id: "nord", category: "dark", isOled: false, isLight: false,
            accent: "#88c0d0", orb2: "#81a1c1", orb3: "#5e81ac",
            bgTop: "#191E28", bgMid: "#202736", bgBottom: "#151922",
            surfaceR: 0.16, surfaceG: 0.20, surfaceB: 0.27,
            sidebarR: 0.11, sidebarG: 0.14, sidebarB: 0.20
        },
        dracula: {
            id: "dracula", category: "dark", isOled: false, isLight: false,
            accent: "#bd93f9", orb2: "#ff79c6", orb3: "#50fa7b",
            bgTop: "#171822", bgMid: "#1F212E", bgBottom: "#13141C",
            surfaceR: 0.16, surfaceG: 0.17, surfaceB: 0.24,
            sidebarR: 0.11, sidebarG: 0.12, sidebarB: 0.17
        },

        // === Modern Light Glass — 2 Themes ===
        frost: {
            id: "frost", category: "light", isOled: false, isLight: true,
            accent: "#0284c7", orb2: "#6366f1", orb3: "#06b6d4",
            bgTop: "#EEF4FB", bgMid: "#E4EDF8", bgBottom: "#DCE7F5",
            surfaceR: 0.98, surfaceG: 0.99, surfaceB: 1.00,
            sidebarR: 0.94, sidebarG: 0.96, sidebarB: 0.99
        },
        sand: {
            id: "sand", category: "light", isOled: false, isLight: true,
            accent: "#d97706", orb2: "#ea580c", orb3: "#0d9488",
            bgTop: "#F9F5EE", bgMid: "#F3ECE1", bgBottom: "#EAE0D0",
            surfaceR: 0.99, surfaceG: 0.98, surfaceB: 0.96,
            sidebarR: 0.96, sidebarG: 0.93, sidebarB: 0.88
        }
    })

    readonly property var currentPreset: presets[presetId] ? presets[presetId] : presets.aurora
    readonly property bool effectiveOled: trueBlackOled || currentPreset.isOled
    readonly property bool isLight: !trueBlackOled && currentPreset.isLight

    readonly property color accent: {
        if (accentMode === "custom") return customAccent
        if (accentMode === "preset") return currentPreset.accent
        return dpiAccent
    }
    readonly property color orbSecondary: useCustomSecondary ? customSecondary : currentPreset.orb2
    readonly property color orbTertiary: currentPreset.orb3
    readonly property color bgTop: effectiveOled ? "#000000" : currentPreset.bgTop
    readonly property color bgMid: effectiveOled ? "#000000" : currentPreset.bgMid
    readonly property color bgBottom: effectiveOled ? "#000000" : currentPreset.bgBottom

    // Radii & Density (pure reactive bindings — no singleton JS helper that breaks AOT tracking!)
    readonly property int cardRadius: windowShape === "sharp" ? 0 : cornerRadius
    readonly property int buttonRadius: {
        if (buttonShape === "sharp") return 0
        if (buttonShape === "squircle") return 11
        if (buttonShape === "pill") return 22
        return cornerRadius === 0 ? 0 : Math.max(4, Math.round(cornerRadius * 0.72))
    }
    readonly property int dialogRadius: {
        if (windowShape === "sharp") return 0
        if (windowShape === "extra") return Math.min(28, Math.max(22, cornerRadius + 8))
        return cornerRadius === 0 ? 0 : Math.min(24, cornerRadius + 2)
    }
    readonly property int windowRadius: {
        if (windowShape === "sharp") return 0
        if (windowShape === "extra") return Math.min(30, Math.max(24, cornerRadius + 10))
        return cornerRadius === 0 ? 0 : Math.min(24, cornerRadius + 2)
    }
    readonly property int windowChamferCut: Math.min(28, Math.max(14, cornerRadius + 4))
    readonly property int cardPad: compactMode ? 11 : 16
    readonly property int sectionSpacing: compactMode ? 8 : 12
    readonly property real speedFactor: Math.max(0.4, Math.min(2.0, animationSpeed))

    // Frosted Glass & True-Black OLED Surfaces
    readonly property color sidebarBg: effectiveOled
        ? Qt.rgba(0.0, 0.0, 0.0, Math.min(0.98, glassOpacity + 0.14))
        : Qt.rgba(currentPreset.sidebarR, currentPreset.sidebarG, currentPreset.sidebarB, Math.min(0.96, glassOpacity + 0.10))

    readonly property color headerBg: effectiveOled
        ? Qt.rgba(0.0, 0.0, 0.0, Math.min(0.96, glassOpacity + 0.10))
        : Qt.rgba(currentPreset.sidebarR, currentPreset.sidebarG, currentPreset.sidebarB, Math.min(0.92, glassOpacity + 0.02))

    readonly property color cardBg: effectiveOled
        ? Qt.rgba(0.02, 0.02, 0.02, Math.min(0.95, glassOpacity + 0.08))
        : Qt.rgba(currentPreset.surfaceR, currentPreset.surfaceG, currentPreset.surfaceB, glassOpacity)

    readonly property color cardHoverBg: effectiveOled
        ? Qt.rgba(0.06, 0.06, 0.06, Math.min(0.96, glassOpacity + 0.10))
        : (isLight
           ? Qt.rgba(1.0, 1.0, 1.0, Math.min(0.98, glassOpacity + 0.12))
           : Qt.rgba(
               Math.min(1.0, currentPreset.surfaceR + 0.045),
               Math.min(1.0, currentPreset.surfaceG + 0.045),
               Math.min(1.0, currentPreset.surfaceB + 0.055),
               Math.min(0.92, glassOpacity + 0.08)
             ))

    readonly property color dialogBg: effectiveOled
        ? Qt.rgba(0.01, 0.01, 0.01, 0.96)
        : Qt.rgba(currentPreset.sidebarR, currentPreset.sidebarG, currentPreset.sidebarB, isLight ? 0.95 : 0.93)

    readonly property color inputBg: effectiveOled
        ? Qt.rgba(0.0, 0.0, 0.0, 0.95)
        : (isLight ? Qt.rgba(0.92, 0.95, 0.99, 0.92) : Qt.rgba(0.04, 0.06, 0.12, 0.85))

    // Modern Button Surfaces
    readonly property color buttonBg: effectiveOled
        ? Qt.rgba(0.05, 0.05, 0.05, 0.92)
        : (isLight
           ? Qt.rgba(0.94, 0.96, 0.99, 0.92)
           : Qt.rgba(
               Math.min(1.0, currentPreset.surfaceR + 0.06),
               Math.min(1.0, currentPreset.surfaceG + 0.07),
               Math.min(1.0, currentPreset.surfaceB + 0.09),
               0.86
             ))

    readonly property color buttonHoverBg: effectiveOled
        ? Qt.rgba(0.11, 0.11, 0.11, 0.96)
        : (isLight
           ? Qt.rgba(0.88, 0.93, 0.99, 0.98)
           : Qt.rgba(
               Math.min(1.0, currentPreset.surfaceR + 0.12),
               Math.min(1.0, currentPreset.surfaceG + 0.13),
               Math.min(1.0, currentPreset.surfaceB + 0.16),
               0.94
             ))

    readonly property color buttonPressedBg: effectiveOled
        ? Qt.rgba(0.02, 0.02, 0.02, 1.0)
        : (isLight
           ? Qt.rgba(0.82, 0.89, 0.97, 1.0)
           : Qt.rgba(
               Math.min(1.0, currentPreset.surfaceR + 0.03),
               Math.min(1.0, currentPreset.surfaceG + 0.04),
               Math.min(1.0, currentPreset.surfaceB + 0.06),
               0.98
             ))

    // Borders & Text
    readonly property color borderSubtle: effectiveOled
        ? Qt.rgba(1, 1, 1, 0.12)
        : (isLight ? Qt.rgba(0.06, 0.12, 0.24, 0.14) : Qt.rgba(1, 1, 1, 0.10))
    readonly property color borderHover: isLight ? Qt.rgba(0.06, 0.12, 0.24, 0.28) : Qt.rgba(1, 1, 1, 0.26)
    readonly property color windowBorder: {
        if (borderStyle === "none") return "transparent"
        if (borderStyle === "subtle") return isLight ? Qt.rgba(0.08, 0.14, 0.28, 0.24 * borderOpacity) : Qt.rgba(1, 1, 1, 0.26 * borderOpacity)
        let c = borderStyle === "custom" ? customBorderColor : accent
        return Qt.rgba(c.r, c.g, c.b, borderOpacity)
    }
    readonly property color effectiveTitlebarBg: {
        if (titlebarStyle === "transparent") return "transparent"
        if (titlebarStyle === "oled") return "#000000"
        if (titlebarStyle === "accent") return Qt.rgba(accent.r, accent.g, accent.b, isLight ? 0.16 : 0.22)
        return headerBg
    }
    readonly property color specularTop: effectiveOled
        ? Qt.rgba(1, 1, 1, 0.10)
        : (isLight ? Qt.rgba(1, 1, 1, 0.85) : Qt.rgba(1, 1, 1, 0.16))

    readonly property color textPrimary: isLight ? "#0F172A" : "#F8FAFC"
    readonly property color textSecondary: isLight ? "#475569" : "#94A3B8"
    readonly property color textMuted: "#64748B"
    readonly property color accentContrastText: {
        let lum = 0.299 * accent.r + 0.587 * accent.g + 0.114 * accent.b
        return lum > 0.58 ? "#040812" : "#FFFFFF"
    }
}
