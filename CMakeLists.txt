cmake_minimum_required(VERSION 3.24)
project(deepcomplex VERSION 2.8.6 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core DBus Gui Qml Quick QuickControls2 Widgets)
find_package(KF6Kirigami QUIET)

qt_standard_project_setup(REQUIRES 6.5)
if(COMMAND qt_policy)
    qt_policy(SET QTP0004 NEW)
endif()

qt_add_executable(deepcomplex
    src/main.cpp
    src/driver/M700ADriver.h
    src/driver/M700ADriver.cpp
    src/driver/HidKeyMap.h
    src/driver/HidKeyMap.cpp
    src/system/AsyncProcess.h
    src/system/BackgroundPortal.h
    src/system/BackgroundPortal.cpp
    src/system/SandboxManager.h
    src/system/SandboxManager.cpp
    src/system/SingleInstance.h
    src/system/SingleInstance.cpp
    src/system/ThemeDpiMonitor.h
    src/system/ThemeDpiMonitor.cpp
    src/core/ProfileStorage.h
    src/core/ProfileStorage.cpp
    src/core/MouseController.h
    src/core/MouseController.cpp
)

set_source_files_properties(src/qml/I18n.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
    QT_RESOURCE_ALIAS I18n.qml
)
set_source_files_properties(src/qml/Theme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
    QT_RESOURCE_ALIAS Theme.qml
)
set_source_files_properties(src/qml/Translations.js PROPERTIES QT_RESOURCE_ALIAS Translations.js)
set_source_files_properties(src/qml/Main.qml PROPERTIES QT_RESOURCE_ALIAS Main.qml)
set_source_files_properties(src/qml/SettingSwitchRow.qml PROPERTIES QT_RESOURCE_ALIAS SettingSwitchRow.qml)
set_source_files_properties(src/qml/components/AnimatedAuroraBackground.qml PROPERTIES QT_RESOURCE_ALIAS AnimatedAuroraBackground.qml)
set_source_files_properties(src/qml/components/GlassCard.qml PROPERTIES QT_RESOURCE_ALIAS GlassCard.qml)
set_source_files_properties(src/qml/components/ModernButton.qml PROPERTIES QT_RESOURCE_ALIAS ModernButton.qml)
set_source_files_properties(src/qml/components/ModernDialog.qml PROPERTIES QT_RESOURCE_ALIAS ModernDialog.qml)
set_source_files_properties(src/qml/components/ModernNavButton.qml PROPERTIES QT_RESOURCE_ALIAS ModernNavButton.qml)
set_source_files_properties(src/qml/components/ModernSegmentedTabs.qml PROPERTIES QT_RESOURCE_ALIAS ModernSegmentedTabs.qml)
set_source_files_properties(src/qml/components/ThemedAppIcon.qml PROPERTIES QT_RESOURCE_ALIAS ThemedAppIcon.qml)
set_source_files_properties(src/qml/components/StartupSplashOverlay.qml PROPERTIES QT_RESOURCE_ALIAS StartupSplashOverlay.qml)
set_source_files_properties(src/qml/components/WindowBorderFrame.qml PROPERTIES QT_RESOURCE_ALIAS WindowBorderFrame.qml)

set_source_files_properties(src/qml/pages/HomePage.qml PROPERTIES QT_RESOURCE_ALIAS HomePage.qml)
set_source_files_properties(src/qml/pages/MacroStudioPage.qml PROPERTIES QT_RESOURCE_ALIAS MacroStudioPage.qml)
set_source_files_properties(src/qml/pages/SettingsPage.qml PROPERTIES QT_RESOURCE_ALIAS SettingsPage.qml)

set_source_files_properties(src/qml/dialogs/CommandExplainDialog.qml PROPERTIES QT_RESOURCE_ALIAS CommandExplainDialog.qml)
set_source_files_properties(src/qml/dialogs/ExtraModeZoneDialog.qml PROPERTIES QT_RESOURCE_ALIAS ExtraModeZoneDialog.qml)
set_source_files_properties(src/qml/dialogs/ResetConfirmDialog.qml PROPERTIES QT_RESOURCE_ALIAS ResetConfirmDialog.qml)
set_source_files_properties(src/qml/dialogs/ThemeEditorDialog.qml PROPERTIES QT_RESOURCE_ALIAS ThemeEditorDialog.qml)
set_source_files_properties(src/qml/dialogs/UnsavedChangesDialog.qml PROPERTIES QT_RESOURCE_ALIAS UnsavedChangesDialog.qml)
set_source_files_properties(src/qml/dialogs/OnboardingDialog.qml PROPERTIES QT_RESOURCE_ALIAS OnboardingDialog.qml)
set_source_files_properties(src/qml/dialogs/RepairDialog.qml PROPERTIES QT_RESOURCE_ALIAS RepairDialog.qml)
set_source_files_properties(src/qml/dialogs/ProgressLogDialog.qml PROPERTIES QT_RESOURCE_ALIAS ProgressLogDialog.qml)
set_source_files_properties(src/qml/dialogs/PermissionsDialog.qml PROPERTIES QT_RESOURCE_ALIAS PermissionsDialog.qml)
set_source_files_properties(src/qml/dialogs/FlatpakAppsDialog.qml PROPERTIES QT_RESOURCE_ALIAS FlatpakAppsDialog.qml)
set_source_files_properties(src/qml/dialogs/MacroPickerDialog.qml PROPERTIES QT_RESOURCE_ALIAS MacroPickerDialog.qml)
set_source_files_properties(src/qml/dialogs/MacroEditorDialog.qml PROPERTIES QT_RESOURCE_ALIAS MacroEditorDialog.qml)

qt_add_qml_module(deepcomplex
    URI org.deepcomplex.App
    VERSION 1.0
    QML_FILES
        src/qml/I18n.qml
        src/qml/Theme.qml
        src/qml/Translations.js
        src/qml/Main.qml
        src/qml/SettingSwitchRow.qml
        src/qml/components/AnimatedAuroraBackground.qml
        src/qml/components/GlassCard.qml
        src/qml/components/ModernButton.qml
        src/qml/components/ModernDialog.qml
        src/qml/components/ModernNavButton.qml
        src/qml/components/ModernSegmentedTabs.qml
        src/qml/components/ThemedAppIcon.qml
        src/qml/components/StartupSplashOverlay.qml
        src/qml/components/WindowBorderFrame.qml
        src/qml/pages/HomePage.qml
        src/qml/pages/MacroStudioPage.qml
        src/qml/pages/SettingsPage.qml
        src/qml/dialogs/CommandExplainDialog.qml
        src/qml/dialogs/ExtraModeZoneDialog.qml
        src/qml/dialogs/ResetConfirmDialog.qml
        src/qml/dialogs/ThemeEditorDialog.qml
        src/qml/dialogs/UnsavedChangesDialog.qml
        src/qml/dialogs/OnboardingDialog.qml
        src/qml/dialogs/RepairDialog.qml
        src/qml/dialogs/ProgressLogDialog.qml
        src/qml/dialogs/PermissionsDialog.qml
        src/qml/dialogs/FlatpakAppsDialog.qml
        src/qml/dialogs/MacroPickerDialog.qml
        src/qml/dialogs/MacroEditorDialog.qml
)

target_compile_definitions(deepcomplex PRIVATE DEEPCOMPLEX_VERSION="${PROJECT_VERSION}")

target_link_libraries(deepcomplex PRIVATE
    Qt6::Core
    Qt6::DBus
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
)

install(TARGETS deepcomplex RUNTIME DESTINATION bin)
install(FILES data/org.deepcomplex.App.desktop DESTINATION share/applications)
install(FILES data/org.deepcomplex.App.metainfo.xml DESTINATION share/metainfo)
install(FILES data/org.deepcomplex.App.svg DESTINATION share/icons/hicolor/scalable/apps)
install(FILES LICENSE DESTINATION share/licenses/org.deepcomplex.App)
