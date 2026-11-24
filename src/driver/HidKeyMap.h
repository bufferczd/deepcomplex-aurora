#pragma once

#include <QString>

namespace HidKeyMap {

int keyEventToHidUsage(int qtKey, int nativeScanCode);
int qtKeyToHidUsage(int qtKey);
QString hidUsageName(int hidCode);

} // namespace HidKeyMap
