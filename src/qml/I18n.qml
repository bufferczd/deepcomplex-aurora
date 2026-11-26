pragma Singleton
import QtQuick
import "Translations.js" as T

QtObject {
    function tr(lang, key) { return T.tr(lang, key) }
    function hex2(v) { return "0x" + (v < 16 ? "0" : "") + v.toString(16).toUpperCase() }

    function isOffMode(mode) { return mode === 0x0D }
    function isStaticMode(mode) { return mode === 0x02 || mode === 0x0D }
    function isReactiveMode(mode) { return mode === 0x0C }
    function isExtraMode(mode) { return mode === 0x09 }
    function isDpiColorUnsupportedMode(mode) { return mode === 0x09 || mode === 0x0D }
    function isMacroCode(code) { return code === 0x0D || code === 0x0E }

    function buildLedModeModel(lang, showHidden, showHex) {
        let all = [
            { key: "m00", mode: 0x00, hidden: false },
            { key: "m01", mode: 0x01, hidden: true  },
            { key: "m02", mode: 0x02, hidden: false },
            { key: "m03", mode: 0x03, hidden: false },
            { key: "m04", mode: 0x04, hidden: true  },
            { key: "m05", mode: 0x05, hidden: false },
            { key: "m06", mode: 0x06, hidden: false },
            { key: "m07", mode: 0x07, hidden: false },
            { key: "m08", mode: 0x08, hidden: false },
            { key: "m09", mode: 0x09, hidden: true  },
            { key: "m0A", mode: 0x0A, hidden: false },
            { key: "m0B", mode: 0x0B, hidden: false },
            { key: "m0C", mode: 0x0C, hidden: false },
            { key: "m0D", mode: 0x0D, hidden: false }
        ]
        let out = []
        for (let i = 0; i < all.length; ++i) {
            let item = all[i]
            if (!item.hidden || showHidden) {
                let txt = T.tr(lang, item.key)
                out.push({ label: showHex ? (hex2(item.mode) + " • " + txt) : txt, mode: item.mode })
            }
        }
        return out
    }

    function buildButtonActionModel(lang, showHex, enableMacro) {
        let all = [
            { key: "act01", code: 0x01, mediaUsage: 0, macroOnly: false },
            { key: "act03", code: 0x03, mediaUsage: 0, macroOnly: false },
            { key: "act02", code: 0x02, mediaUsage: 0, macroOnly: false },
            { key: "act04", code: 0x04, mediaUsage: 0, macroOnly: false },
            { key: "act05", code: 0x05, mediaUsage: 0, macroOnly: false },
            { key: "act06", code: 0x06, mediaUsage: 0, macroOnly: false },
            { key: "act0B", code: 0x0B, mediaUsage: 0, macroOnly: false },
            { key: "act0C", code: 0x0C, mediaUsage: 0, macroOnly: false },
            { key: "act09", code: 0x09, mediaUsage: 0, macroOnly: false },
            { key: "act14", code: 0x14, mediaUsage: 0, macroOnly: false },
            { key: "act15", code: 0x15, mediaUsage: 0, macroOnly: false },
            { key: "act07", code: 0x07, mediaUsage: 0, macroOnly: false },
            { key: "actVolUp",     code: 0x0F, mediaUsage: 0x00E9, macroOnly: false },
            { key: "actVolDown",   code: 0x0F, mediaUsage: 0x00EA, macroOnly: false },
            { key: "actMute",      code: 0x0F, mediaUsage: 0x00E2, macroOnly: false },
            { key: "actPlayPause", code: 0x0F, mediaUsage: 0x00CD, macroOnly: false },
            { key: "actNextTrack", code: 0x0F, mediaUsage: 0x00B5, macroOnly: false },
            { key: "actPrevTrack", code: 0x0F, mediaUsage: 0x00B6, macroOnly: false },
            { key: "actStop",      code: 0x0F, mediaUsage: 0x00B7, macroOnly: false },
            { key: "actCalc",      code: 0x0F, mediaUsage: 0x0192, macroOnly: false },
            { key: "actBrowser",   code: 0x0F, mediaUsage: 0x0223, macroOnly: false },
            { key: "actMacro",     code: 0x0D, mediaUsage: 0, macroOnly: true },
            { key: "act00",        code: 0x00, mediaUsage: 0, macroOnly: false }
        ]
        let out = []
        for (let i = 0; i < all.length; ++i) {
            if (all[i].macroOnly && !enableMacro) continue
            let txt = T.tr(lang, all[i].key)
            let hexTag = all[i].mediaUsage > 0
                         ? (" (" + hex2(all[i].code) + ":0x" + all[i].mediaUsage.toString(16).toUpperCase() + ")")
                         : (" (" + hex2(all[i].code) + ")")
            out.push({ label: showHex ? (txt + hexTag) : txt, code: all[i].code, mediaUsage: all[i].mediaUsage })
        }
        return out
    }

    function indexForLedMode(model, mode) {
        for (let i = 0; i < model.length; ++i) { if (model[i].mode === mode) return i }
        return 0
    }

    function indexForButtonAction(model, code, mediaUsage) {
        let normalized = (code === 0x0E) ? 0x0D : code
        for (let i = 0; i < model.length; ++i) {
            if (model[i].code === normalized && (normalized !== 0x0F || model[i].mediaUsage === mediaUsage)) return i
        }
        return model.length - 1
    }
}
