#include "HidKeyMap.h"

#include <Qt>

namespace HidKeyMap {

int qtKeyToHidUsage(int qtKey) {
    if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z) return 0x04 + (qtKey - Qt::Key_A);
    if (qtKey >= Qt::Key_1 && qtKey <= Qt::Key_9) return 0x1E + (qtKey - Qt::Key_1);
    if (qtKey == Qt::Key_0) return 0x27;
    if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F12) return 0x3A + (qtKey - Qt::Key_F1);

    switch (qtKey) {
    case Qt::Key_Return:
    case Qt::Key_Enter:        return 0x28;
    case Qt::Key_Escape:       return 0x29;
    case Qt::Key_Backspace:    return 0x2A;
    case Qt::Key_Tab:          return 0x2B;
    case Qt::Key_Space:        return 0x2C;
    case Qt::Key_Minus:        return 0x2D;
    case Qt::Key_Equal:        return 0x2E;
    case Qt::Key_BracketLeft:  return 0x2F;
    case Qt::Key_BracketRight: return 0x30;
    case Qt::Key_Backslash:    return 0x31;
    case Qt::Key_Semicolon:    return 0x33;
    case Qt::Key_Apostrophe:   return 0x34;
    case Qt::Key_QuoteLeft:    return 0x35;
    case Qt::Key_Comma:        return 0x36;
    case Qt::Key_Period:       return 0x37;
    case Qt::Key_Slash:        return 0x38;
    case Qt::Key_CapsLock:     return 0x39;
    case Qt::Key_Print:        return 0x46;
    case Qt::Key_ScrollLock:   return 0x47;
    case Qt::Key_Pause:        return 0x48;
    case Qt::Key_Insert:       return 0x49;
    case Qt::Key_Home:         return 0x4A;
    case Qt::Key_PageUp:       return 0x4B;
    case Qt::Key_Delete:       return 0x4C;
    case Qt::Key_End:          return 0x4D;
    case Qt::Key_PageDown:     return 0x4E;
    case Qt::Key_Right:        return 0x4F;
    case Qt::Key_Left:         return 0x50;
    case Qt::Key_Down:         return 0x51;
    case Qt::Key_Up:           return 0x52;
    case Qt::Key_NumLock:      return 0x53;
    case Qt::Key_Control:      return 0xE0;
    case Qt::Key_Shift:        return 0xE1;
    case Qt::Key_Alt:          return 0xE2;
    case Qt::Key_Meta:
    case Qt::Key_Super_L:      return 0xE3;
    case Qt::Key_AltGr:        return 0xE6;
    case Qt::Key_Super_R:      return 0xE7;
    default:                   return 0;
    }
}

int keyEventToHidUsage(int qtKey, int nativeScanCode) {
    if (nativeScanCode > 8) {
        const int ev = nativeScanCode - 8;
        switch (ev) {
        case 1:   return 0x29; // Esc
        case 2:   return 0x1E; // 1
        case 3:   return 0x1F; // 2
        case 4:   return 0x20; // 3
        case 5:   return 0x21; // 4
        case 6:   return 0x22; // 5
        case 7:   return 0x23; // 6
        case 8:   return 0x24; // 7
        case 9:   return 0x25; // 8
        case 10:  return 0x26; // 9
        case 11:  return 0x27; // 0
        case 12:  return 0x2D; // -
        case 13:  return 0x2E; // =
        case 14:  return 0x2A; // Backspace
        case 15:  return 0x2B; // Tab
        case 16:  return 0x14; // Q
        case 17:  return 0x1A; // W
        case 18:  return 0x08; // E
        case 19:  return 0x15; // R
        case 20:  return 0x17; // T
        case 21:  return 0x1C; // Y
        case 22:  return 0x18; // U
        case 23:  return 0x0C; // I
        case 24:  return 0x12; // O
        case 25:  return 0x13; // P
        case 26:  return 0x2F; // [
        case 27:  return 0x30; // ]
        case 28:  return 0x28; // Enter
        case 29:  return 0xE0; // Left Ctrl
        case 30:  return 0x04; // A
        case 31:  return 0x16; // S
        case 32:  return 0x07; // D
        case 33:  return 0x09; // F
        case 34:  return 0x0A; // G
        case 35:  return 0x0B; // H
        case 36:  return 0x0D; // J
        case 37:  return 0x0E; // K
        case 38:  return 0x0F; // L
        case 39:  return 0x33; // ;
        case 40:  return 0x34; // '
        case 41:  return 0x35; // `
        case 42:  return 0xE1; // Left Shift
        case 43:  return 0x31; // backslash
        case 44:  return 0x1D; // Z
        case 45:  return 0x1B; // X
        case 46:  return 0x06; // C
        case 47:  return 0x19; // V
        case 48:  return 0x05; // B
        case 49:  return 0x11; // N
        case 50:  return 0x10; // M
        case 51:  return 0x36; // ,
        case 52:  return 0x37; // .
        case 53:  return 0x38; // /
        case 54:  return 0xE5; // Right Shift
        case 55:  return 0x55; // KP *
        case 56:  return 0xE2; // Left Alt
        case 57:  return 0x2C; // Space
        case 58:  return 0x39; // CapsLock
        case 59:  return 0x3A; // F1
        case 60:  return 0x3B; // F2
        case 61:  return 0x3C; // F3
        case 62:  return 0x3D; // F4
        case 63:  return 0x3E; // F5
        case 64:  return 0x3F; // F6
        case 65:  return 0x40; // F7
        case 66:  return 0x41; // F8
        case 67:  return 0x42; // F9
        case 68:  return 0x43; // F10
        case 69:  return 0x53; // NumLock
        case 70:  return 0x47; // ScrollLock
        case 87:  return 0x44; // F11
        case 88:  return 0x45; // F12
        case 96:  return 0x58; // KP Enter
        case 97:  return 0xE4; // Right Ctrl
        case 98:  return 0x54; // KP /
        case 99:  return 0x46; // PrintScreen
        case 100: return 0xE6; // Right Alt
        case 102: return 0x4A; // Home
        case 103: return 0x52; // Up
        case 104: return 0x4B; // PageUp
        case 105: return 0x50; // Left
        case 106: return 0x4F; // Right
        case 107: return 0x4D; // End
        case 108: return 0x51; // Down
        case 109: return 0x4E; // PageDown
        case 110: return 0x49; // Insert
        case 111: return 0x4C; // Delete
        case 119: return 0x48; // Pause
        case 125: return 0xE3; // Left Super
        case 126: return 0xE7; // Right Super
        default: break;
        }
    }
    return qtKeyToHidUsage(qtKey);
}

QString hidUsageName(int hidCode) {
    if (hidCode >= 0x04 && hidCode <= 0x1D) return QString(QChar('A' + (hidCode - 0x04)));
    if (hidCode >= 0x1E && hidCode <= 0x26) return QString::number(1 + (hidCode - 0x1E));
    if (hidCode == 0x27) return QStringLiteral("0");
    if (hidCode >= 0x3A && hidCode <= 0x45) return QStringLiteral("F") + QString::number(1 + (hidCode - 0x3A));

    switch (hidCode) {
    case 0x28: return QStringLiteral("Enter");
    case 0x29: return QStringLiteral("Esc");
    case 0x2A: return QStringLiteral("Backspace");
    case 0x2B: return QStringLiteral("Tab");
    case 0x2C: return QStringLiteral("Space");
    case 0x2D: return QStringLiteral("-");
    case 0x2E: return QStringLiteral("=");
    case 0x2F: return QStringLiteral("[");
    case 0x30: return QStringLiteral("]");
    case 0x31: return QStringLiteral("\\");
    case 0x33: return QStringLiteral(";");
    case 0x34: return QStringLiteral("'");
    case 0x35: return QStringLiteral("`");
    case 0x36: return QStringLiteral(",");
    case 0x37: return QStringLiteral(".");
    case 0x38: return QStringLiteral("/");
    case 0x39: return QStringLiteral("CapsLock");
    case 0x46: return QStringLiteral("PrtSc");
    case 0x47: return QStringLiteral("ScrLk");
    case 0x48: return QStringLiteral("Pause");
    case 0x49: return QStringLiteral("Insert");
    case 0x4A: return QStringLiteral("Home");
    case 0x4B: return QStringLiteral("PgUp");
    case 0x4C: return QStringLiteral("Del");
    case 0x4D: return QStringLiteral("End");
    case 0x4E: return QStringLiteral("PgDn");
    case 0x4F: return QStringLiteral("Right");
    case 0x50: return QStringLiteral("Left");
    case 0x51: return QStringLiteral("Down");
    case 0x52: return QStringLiteral("Up");
    case 0x53: return QStringLiteral("NumLock");
    case 0xE0: return QStringLiteral("Ctrl");
    case 0xE1: return QStringLiteral("Shift");
    case 0xE2: return QStringLiteral("Alt");
    case 0xE3: return QStringLiteral("Super");
    case 0xE4: return QStringLiteral("RCtrl");
    case 0xE5: return QStringLiteral("RShift");
    case 0xE6: return QStringLiteral("RAlt");
    case 0xE7: return QStringLiteral("RSuper");
    case 0xF1: return QStringLiteral("Mouse Left");
    case 0xF2: return QStringLiteral("Mouse Right");
    case 0xF3: return QStringLiteral("Mouse Middle");
    case 0xF4: return QStringLiteral("Mouse Back");
    case 0xF5: return QStringLiteral("Mouse Forward");
    default:   return QStringLiteral("0x%1").arg(hidCode, 2, 16, QLatin1Char('0')).toUpper();
    }
}

} // namespace HidKeyMap
