/// @file Application.cpp
#include "app/Application.hpp"

#include <QApplication>
#include <QIcon>
#include <QKeySequence>
#include <QMainWindow>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickStyle>
#include <QQuickWidget>
#include <QQuickWindow>
#include <QShortcut>
#include <QStackedLayout>
#include <QSurfaceFormat>
#include <array>
#include <cstdlib>
#include <optional>
#include <sstream>
#include <string>

#include "app/AppGameplaySettingsProvider.hpp"
#include "app/CompositionRoot.hpp"
#include "app/EngineRenderBridge.hpp"
#include "app/GameplayWorldBridge.hpp"
#include "app/RuntimeLoop.hpp"
#include "app/SettingsConverter.hpp"
#include "app/SoundSystem.hpp"
#include "app/TrainingFlowController.hpp"
#include "app/WorldRaycastHitTest.hpp"
#include "core/CoreEvents.hpp"
#include "core/EventBus.hpp"
#include "core/Logger.hpp"
#include "engine/Engine.hpp"
#include "gameplay/BuiltInModes.hpp"
#include "gameplay/GameModeRegistry.hpp"
#include "gameplay/ModeOrchestrator.hpp"
#include "gameplay/TargetFactory.hpp"
#include "gameplay/TargetSystem.hpp"
#include "input/InputManager.hpp"
#include "persistence/AppSettings.hpp"
#include "persistence/PersistenceDatabase.hpp"
#include "persistence/ProfileManager.hpp"
#include "persistence/SessionHistory.hpp"
#include "persistence/SettingsManager.hpp"
#include "ui/UiViewModel.hpp"
#include "ui/Version.hpp"
#include "world/World.hpp"
#include "world/WorldRenderSync.hpp"

extern int qInitResources_ui();
extern int qCleanupResources_ui();

namespace xaimassist::app {

int Application::Run(int argc, char* argv[]) {
    qInitResources_ui();

    QSurfaceFormat::setDefaultFormat(engine::Engine::RecommendedSurfaceFormat());
    QQuickStyle::setStyle(QStringLiteral("Material"));
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QApplication qtApplication(argc, argv);
    const QString applicationName = QString::fromUtf8(ui::version::APP_NAME);
    const QString applicationVersion =
        QString::fromUtf8(ui::version::APP_VERSION);
    const QString applicationOrganization =
        QString::fromUtf8(ui::version::APP_ORGANIZATION);

    if (!applicationName.isEmpty()) {
        qtApplication.setApplicationName(applicationName);
    }
    if (!applicationVersion.isEmpty()) {
        qtApplication.setApplicationVersion(applicationVersion);
    }
    if (!applicationOrganization.isEmpty()) {
        qtApplication.setOrganizationName(applicationOrganization);
    }

    const QIcon applicationIcon(
        QStringLiteral(":/xaimassist/ui/XAimAssist.png"));
    if (!applicationIcon.isNull()) {
        qtApplication.setWindowIcon(applicationIcon);
    }

    CompositionRoot compositionRoot;
    RuntimeServices services = compositionRoot.CreateRuntimeServices();

    services.eventBus->Publish(core::events::ApplicationLifecycleEvent{
        core::events::ApplicationLifecycleState::Starting});
    services.logger->Info("app", "XAimAssist startup sequence initialized");
    services.logger->LogIf(
        "DEBUG_PERF", core::LogLevel::Info, "perf",
        "DEBUG_PERF enabled: session-end pipeline timing reports are active");

    const auto& runtimeSettings = services.settingsManager->Settings();

    {
        std::ostringstream settingsStream;
        settingsStream
            << "Loaded Settings: targetColor=("
            << runtimeSettings.gameplay.target.color.red << ','
            << runtimeSettings.gameplay.target.color.green << ','
            << runtimeSettings.gameplay.target.color.blue << ")"
            << ", TargetRadius=" << runtimeSettings.gameplay.target.radius
            << ", CmPer360=" << runtimeSettings.input.sensitivity.cmPer360
            << ", Dpi=" << runtimeSettings.input.sensitivity.dpi
            << ", SensitivityScale="
            << runtimeSettings.input.sensitivity.sensitivityScale
            << ", Theme="
            << SettingsConverter::ThemeModeToString(runtimeSettings.ui.themeMode)
            << ", language="
            << SettingsConverter::LanguageToString(runtimeSettings.ui.language);
        services.logger->Info("Settings", settingsStream.str());
    }

    if (services.profileManager) {
        if (const auto activeProfile = services.profileManager->ActiveProfile();
            activeProfile.has_value()) {
            std::ostringstream profileStream;
            profileStream << "Active profile: id=" << activeProfile->id
                          << ", name=" << activeProfile->name;
            services.logger->Info("persistence", profileStream.str());
        }
    }

    if (services.persistenceDatabase) {
        services.logger->Info(
            "persistence",
            "SQLite DB: " + services.persistenceDatabase->DatabasePath());
    }

    ui::UiViewModel uiViewModel(runtimeSettings);
    uiViewModel.ReturnToMainMenu();

    QMainWindow window;
    const QString windowTitle =
        applicationName.isEmpty() ? QStringLiteral("XAimAssist") : applicationName;
    window.setWindowTitle(windowTitle);
    if (!applicationIcon.isNull()) {
        window.setWindowIcon(applicationIcon);
    }
    window.resize(1480, 860);

    auto* viewportWidget = services.engine->CreateViewport(&window);
    if (viewportWidget == nullptr) {
        services.logger->Error("app", "Engine viewport initialization failed");
        return EXIT_FAILURE;
    }

    auto* uiOverlay = new QQuickWidget(&window);
    uiOverlay->setResizeMode(QQuickWidget::SizeRootObjectToView);
    uiOverlay->setClearColor(Qt::transparent);
    uiOverlay->setAttribute(Qt::WA_TranslucentBackground, true);
    uiOverlay->setAttribute(Qt::WA_AlwaysStackOnTop, true);
    uiOverlay->setFocusPolicy(Qt::StrongFocus);
    uiOverlay->rootContext()->setContextProperty(QStringLiteral("viewModel"),
                                                 &uiViewModel);
    uiOverlay->setSource(
        QUrl(QStringLiteral("qrc:/xaimassist/ui/MainView.qml")));

    if (uiOverlay->status() == QQuickWidget::Error) {
        for (const auto& error : uiOverlay->errors()) {
            services.logger->Error("ui", error.toString().toStdString());
        }
    }

    auto* layeredContainer = new QWidget(&window);
    auto* layeredLayout = new QStackedLayout(layeredContainer);
    layeredLayout->setStackingMode(QStackedLayout::StackAll);
    layeredLayout->setContentsMargins(0, 0, 0, 0);
    layeredLayout->addWidget(viewportWidget);
    layeredLayout->addWidget(uiOverlay);
    window.setCentralWidget(layeredContainer);

    viewportWidget->hide();

    world::World world;
    EngineRenderBridge renderBridge(*services.engine);
    world::WorldRenderSync worldRenderSync(world, renderBridge);

    GameplayWorldBridge gameplayWorldBridge(world);
    WorldRaycastHitTest worldRaycastHitTest(world, *services.engine);
    AppGameplaySettingsProvider gameplaySettingsProvider(*services.settingsManager);

    gameplay::TargetFactory targetFactory;
    gameplay::TargetSystem targetSystem(*services.eventBus, *services.logger,
                                        gameplayWorldBridge, worldRaycastHitTest,
                                        targetFactory);

    gameplay::GameModeRegistry gameModeRegistry;
    gameplay::RegisterBuiltInModes(gameModeRegistry);
    gameplay::ModeOrchestrator modeOrchestrator(
        *services.eventBus, *services.logger, gameModeRegistry,
        gameplayWorldBridge, targetSystem, gameplaySettingsProvider);

    uiViewModel.SetModes(
        SettingsConverter::ToModeDescriptors(gameModeRegistry.AvailableModes()));

    RuntimeLoop runtimeLoop(*services.frameClock, *services.eventBus);

    auto applyRuntimeSettings =
        [&](const persistence::AppSettings& settings,
            const std::array<double, 3>& viewportBackground,
            bool persistSettings) {
            if (services.settingsManager) {
                services.settingsManager->SetSettings(settings);
                if (persistSettings && !services.settingsManager->Save()) {
                    services.logger->Warning("persistence",
                                             "Failed to Save Settings changes");
                }
            }

            services.inputManager->SetSensitivitySettings(
                SettingsConverter::ToSensitivitySettings(settings));
            services.inputManager->SetRawInputEnabled(
                settings.input.rawInputEnabled);

            targetSystem.UpdateAllTargetColors(
                {settings.gameplay.target.color.red,
                 settings.gameplay.target.color.green,
                 settings.gameplay.target.color.blue});

            services.engine->SetBackgroundColor(viewportBackground);

            if (services.soundSystem) {
                services.soundSystem->ApplySettings(settings.sound);
            }
        };

    applyRuntimeSettings(runtimeSettings,
                         uiViewModel.GetViewportBackgroundColor(), false);

    auto* fullscreenShortcut = new QShortcut(
        QKeySequence::fromString(QString::fromStdString(
            runtimeSettings.keybindings.toggleFullscreen)),
        &window);
    QObject::connect(fullscreenShortcut, &QShortcut::activated, &window,
                     [&window]() {
                         if (window.isFullScreen()) {
                             window.showNormal();
                         } else {
                             window.showFullScreen();
                         }
                     });

    auto* fpsShortcut = new QShortcut(
        QKeySequence::fromString(QString::fromStdString(
            runtimeSettings.keybindings.toggleFpsCounter)),
        &window);
    QObject::connect(fpsShortcut, &QShortcut::activated, &window,
                     [&uiViewModel]() { uiViewModel.ToggleFpsCounterRuntime(); });

    auto* crosshairShortcut = new QShortcut(
        QKeySequence::fromString(QString::fromStdString(
            runtimeSettings.keybindings.toggleCrosshair)),
        &window);
    QObject::connect(crosshairShortcut, &QShortcut::activated, &window,
                     [&uiViewModel]() { uiViewModel.ToggleCrosshairKeyOverride(); });

    uiViewModel.SetSettingsChangedCallback(
        [&, fullscreenShortcut, fpsShortcut, crosshairShortcut](
            const persistence::AppSettings& settings,
            const std::array<double, 3>& viewportBackground) {
            applyRuntimeSettings(settings, viewportBackground, true);
            const auto toKey = [](const std::string& s) {
                return QKeySequence::fromString(QString::fromStdString(s));
            };
            fullscreenShortcut->setKey(toKey(settings.keybindings.toggleFullscreen));
            fpsShortcut->setKey(toKey(settings.keybindings.toggleFpsCounter));
            crosshairShortcut->setKey(toKey(settings.keybindings.toggleCrosshair));
        });

    services.inputManager->SetLookHandler([&](const input::LookAction& action) {
        services.engine->ApplyCameraLookDelta(action.yawDeltaDegrees,
                                              action.pitchDeltaDegrees);
    });

    TrainingFlowController controller(
        TrainingFlowController::Deps{
            *services.eventBus,
            *services.logger,
            *services.engine,
            *services.inputManager,
            modeOrchestrator,
            worldRenderSync,
            world,
            uiViewModel,
            runtimeLoop,
            services.profileManager,
            services.sessionHistory,
            viewportWidget,
            uiOverlay,
            &window},
        &window);
    controller.Initialize();

    services.inputManager->AttachViewport(viewportWidget);
    services.inputManager->SetCaptureEnabled(false);

    worldRenderSync.Sync();

    uiOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    uiOverlay->raise();
    uiOverlay->setFocus(Qt::OtherFocusReason);

    services.eventBus->Publish(core::events::ApplicationLifecycleEvent{
        core::events::ApplicationLifecycleState::Running});
    services.logger->Info("app", "Runtime loop started");

    QObject::connect(&qtApplication, &QCoreApplication::aboutToQuit, [&]() {
        controller.Stop();
        runtimeLoop.Stop();  // Safety net: idempotent if controller already stopped it
        services.inputManager->DetachViewport();
        worldRenderSync.Reset();
        services.engine->Shutdown();
        services.eventBus->Publish(core::events::ApplicationLifecycleEvent{
            core::events::ApplicationLifecycleState::Stopping});
        services.logger->Info("app", "XAimAssist Shutdown sequence completed");
    });

    if (runtimeSettings.window.startFullscreen) {
        window.showFullScreen();
    } else {
        window.show();
    }
    return qtApplication.exec();
}

}  // namespace xaimassist::app
