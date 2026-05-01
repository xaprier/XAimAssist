/// @file I18nProvider.cpp
#include "ui/I18nProvider.hpp"

#include <QString>

#include "ui/Version.hpp"

namespace xaimassist::ui {

QVariantMap I18nProvider::Build(persistence::UiLanguage language) {
    if (language == persistence::UiLanguage::Turkish) {
        return _BuildTurkish();
    }

    return _BuildEnglish();
}

QString I18nProvider::ModeName(persistence::UiLanguage language,
                               const std::string& modeId,
                               const std::string& fallback) {
    if (language == persistence::UiLanguage::Turkish) {
        if (modeId == "static_sphere") return QStringLiteral("Sabit Küre");
        if (modeId == "gridshot") return QStringLiteral("GridShot");
        if (modeId == "next_shot") return QStringLiteral("Next Shot");
        if (modeId == "strafing_targets") return QStringLiteral("Strafing Hedefler");
        if (modeId == "tracking_targets") return QStringLiteral("Tracking Hedefler");
    }

    return QString::fromStdString(fallback);
}

QString I18nProvider::ModeDescription(persistence::UiLanguage language,
                                      const std::string& modeId,
                                      const std::string& fallback) {
    if (language == persistence::UiLanguage::Turkish) {
        if (modeId == "static_sphere") {
            return QStringLiteral(
                "Tek bir sabit küre hedefi üretir; vurulduğunda "
                "yeni bir konuma taşır.");
        }
        if (modeId == "gridshot") {
            return QStringLiteral(
                "Sabit bir uzaklık düzleminde varsayılan 5x5 grid ve 3 aktif hedef "
                "ile başlar; satır/sütun/aktif hedef sayısı ayarlanabilir ve "
                "vurulan hedef boş bir hücrede rastgele yeniden üretilir.");
        }
        if (modeId == "next_shot") {
            return QStringLiteral(
                "Static Sphere ile benzer çalışır; fark olarak bir sonraki hedefin "
                "konumu düşük opaklıkta önizleme küresi ile önceden gösterilir.");
        }
        if (modeId == "strafing_targets") {
            return QStringLiteral(
                "Tek bir sabit küre ile başlar. İlk isabet küreyi rastgele bir YZ "
                "ekseninde ayarlı hızda sürekli hareket ettirir. Hareketli küre 2.5 "
                "saniye içinde vurulmazsa miss sayılır ve yeni sabit küre rastgele "
                "konumda üretilir.");
        }
        if (modeId == "tracking_targets") {
            return QStringLiteral(
                "Tek bir hareketli küre üretir. Küre ayarlı hızda sürekli hareket "
                "eder, YZ alan sınırına geldiğinde sekme benzeri şekilde ters yönde "
                "rastgele eksende hareketine devam eder.");
        }
    }

    return QString::fromStdString(fallback);
}

QString I18nProvider::ModeSettingName(persistence::UiLanguage language,
                                      const std::string& modeId,
                                      const std::string& settingKey,
                                      const std::string& fallback) {
    if (language == persistence::UiLanguage::Turkish) {
        if (modeId == "gridshot") {
            if (settingKey == "grid_rows") return QStringLiteral("Grid Satır");
            if (settingKey == "grid_columns") return QStringLiteral("Grid Sütun");
            if (settingKey == "active_target_count") return QStringLiteral("Aktif Hedef");
        }
        if (modeId == "strafing_targets" || modeId == "tracking_targets") {
            if (settingKey == "speed") return QStringLiteral("Hız");
        }
    }

    return QString::fromStdString(fallback);
}

// ---------------------------------------------------------------------------
// Private builders
// ---------------------------------------------------------------------------

QVariantMap I18nProvider::_BuildTurkish() {
    return {
        {"appTitle", QStringLiteral("XAimAssist")},
        {"appSubtitle", QStringLiteral("Modüler FPS Aim Trainer")},

        {"tabMenu", QStringLiteral("Ana Menü")},
        {"tabModes", QStringLiteral("Mod Seçimi")},
        {"tabTraining", QStringLiteral("Antrenman")},
        {"tabStats", QStringLiteral("İstatistik")},
        {"tabSettings", QStringLiteral("Ayarlar")},
        {"tabAbout", QStringLiteral("Hakkında")},

        {"menuTitle", QStringLiteral("Hoş geldin")},
        {"menuSubtitle", QStringLiteral("Mod seç, ayarları düzenle ve antrenmana başla.")},
        {"menuActionModes", QStringLiteral("Mod Seç")},
        {"menuActionStart", QStringLiteral("Antrenmanı Başlat")},
        {"menuActionStats", QStringLiteral("İstatistikleri Aç")},
        {"menuActionSettings", QStringLiteral("Ayarları Aç")},
        {"menuActiveMode", QStringLiteral("Seçili Mod")},
        {"menuSession", QStringLiteral("Oturum")},

        {"modeTitle", QStringLiteral("Mod Seçimi")},
        {"modeSubtitle", QStringLiteral("Antrenmana başlamadan önce oyun modunu belirle.")},
        {"modeSelect", QStringLiteral("Bu modu seç")},
        {"modeDuration", QStringLiteral("Varsayılan Süre")},
        {"modeConfiguredDuration", QStringLiteral("Antrenman Süresi")},
        {"modeDistance", QStringLiteral("Varsayılan Uzaklık")},
        {"modeConfiguredDistance", QStringLiteral("Spawn Uzaklığı")},
        {"modeConfiguredGridRows", QStringLiteral("Grid Satır")},
        {"modeConfiguredGridColumns", QStringLiteral("Grid Sütun")},
        {"modeConfiguredActiveTargets", QStringLiteral("Aktif Hedef")},
        {"modeReset", QStringLiteral("Mod Ayarlarını Sıfırla")},
        {"modeStart", QStringLiteral("Seçili Modu Başlat")},
        {"modeNoModes", QStringLiteral("Kayıtlı mod bulunamadı.")},

        {"resultTitle", QStringLiteral("Sonuç")},
        {"resultLive", QStringLiteral("Canlı Özet")},
        {"resultSaved", QStringLiteral("Kaydedildi")},
        {"resultNotSaved", QStringLiteral("Kaydedilmedi")},
        {"resultInvalid", QStringLiteral("Geçersiz")},
        {"resultNotSavedHint", QStringLiteral("Exit Training ile çıkıldığı için sonuç geçmişe eklenmedi.")},
        {"resultInvalidHint", QStringLiteral("Bu sonuç geçersiz olduğu için geçmişe eklenmedi.")},
        {"resultInvalidReasonReaction", QStringLiteral("Tepki süresi 0.0 ms")},
        {"resultInvalidReasonHits", QStringLiteral("İsabet sayısı 0")},
        {"resultInvalidReasonScore", QStringLiteral("Skor negatif")},
        {"resultInvalidReasonAccuracy", QStringLiteral("Doğruluk 0% veya 100%'ün üstünde")},
        {"resultInvalidReasonUnknown", QStringLiteral("Geçersiz metrik")},
        {"resultLiveHint", QStringLiteral("Antrenman bitmeden önce seçili mod ve canlı metrikler burada gösterilir.")},
        {"resultMode", QStringLiteral("Mod")},
        {"resultConfiguredDuration", QStringLiteral("Hedef Süre")},
        {"resultElapsedDuration", QStringLiteral("Gerçek Süre")},
        {"resultStoppedAt", QStringLiteral("Bitiş Zamanı")},
        {"resultStopReason", QStringLiteral("Bitiş Nedeni")},
        {"resultStopCompleted", QStringLiteral("Tamamlandı")},
        {"resultStopAborted", QStringLiteral("Kullanıcı Çıkışı")},
        {"resultPerfFirst", QStringLiteral("İlk Sonuç")},
        {"resultPerfBest", QStringLiteral("En İyi Performans")},
        {"resultPerfWorst", QStringLiteral("En Kötü Performans")},
        {"resultPerfAboveAverage", QStringLiteral("Ortalamanın Üstünde")},
        {"resultPerfBelowAverage", QStringLiteral("Ortalamanın Altında")},
        {"resultPerfAverage", QStringLiteral("Ortalama Seviyede")},
        {"resultPerfUnavailable", QStringLiteral("Karşılaştırma Yok")},
        {"resultPerfCurrentShort", QStringLiteral("Şu an")},
        {"resultPerfAverageShort", QStringLiteral("Ort")},
        {"resultPerfBestShort", QStringLiteral("En İyi")},
        {"resultPerfWorstShort", QStringLiteral("En Kötü")},
        {"resultPerfMetricAverage", QStringLiteral("Mod Ortalama Skor")},
        {"resultPerfMetricBest", QStringLiteral("Mod En İyi Skor")},
        {"resultPerfMetricWorst", QStringLiteral("Mod En Kötü Skor")},
        {"resultPerfMetricSessions", QStringLiteral("Mod Oturum Sayısı")},

        {"trainingTitle", QStringLiteral("Antrenman")},
        {"trainingSubtitle", QStringLiteral("Canlı metrikleri takip et ve performansını yükselt.")},
        {"trainingStart", QStringLiteral("Başlat")},
        {"trainingStop", QStringLiteral("Durdur")},
        {"trainingStatus", QStringLiteral("Durum")},
        {"trainingStatusActive", QStringLiteral("Aktif")},
        {"trainingStatusIdle", QStringLiteral("Beklemede")},
        {"trainingMode", QStringLiteral("Mod")},
        {"trainingElapsed", QStringLiteral("Süre")},
        {"trainingScore", QStringLiteral("Skor")},
        {"trainingHits", QStringLiteral("İsabet")},
        {"trainingMisses", QStringLiteral("Iska")},
        {"trainingAccuracy", QStringLiteral("Doğruluk")},
        {"trainingReaction", QStringLiteral("Tepki Süresi")},
        {"trainingSps", QStringLiteral("Atış/Saniye")},
        {"trainingCaptureHint", QStringLiteral("İpucu: Nişan kontrolü için eğitim sırasında fare yakalama açıktır.")},
        {"trainingWaitForClick", QStringLiteral("Antrenmanı başlatmak için sahneye sol tık yap.")},
        {"trainingCountdown", QStringLiteral("Başlıyor")},
        {"trainingPausedTitle", QStringLiteral("Antrenman Duraklatıldı")},
        {"trainingPausedSubtitle", QStringLiteral("Sayaçlar ve mod güncellemeleri beklemede.")},
        {"trainingContinue", QStringLiteral("Devam Et")},
        {"trainingExit", QStringLiteral("Antrenmandan Çık")},
        {"trainingOpenSettings", QStringLiteral("Ayarlar")},
        {"trainingBack", QStringLiteral("Geri")},

        {"statsTitle", QStringLiteral("İstatistikler")},
        {"statsSubtitle", QStringLiteral("Geçmiş oturumları incele ve en iyi sonucunla kıyasla.")},
        {"statsBestSession", QStringLiteral("Bu Moddaki En İyi Oturum")},
        {"statsNoBest", QStringLiteral("Bu mod için kayıt bulunamadı.")},
        {"statsRecent", QStringLiteral("Son Oturumlar")},
        {"statsNoRecent", QStringLiteral("Henüz geçmiş oturum yok.")},
        {"statsScore", QStringLiteral("Skor")},
        {"statsAccuracy", QStringLiteral("Doğruluk")},
        {"statsReaction", QStringLiteral("Ort. Tepki")},
        {"statsDate", QStringLiteral("Tarih")},
        {"statsShots", QStringLiteral("Atış")},

        {"settingsTitle", QStringLiteral("Ayarlar")},
        {"settingsSubtitle", QStringLiteral("Tüm değişiklikler anında uygulanır ve kaydedilir.")},
        {"settingsTheme", QStringLiteral("Tema")},
        {"settingsEnable", QStringLiteral("Aç")},
        {"settingsSize", QStringLiteral("Boyut")},
        {"settingsThemeDark", QStringLiteral("Koyu")},
        {"settingsThemeLight", QStringLiteral("Açık")},
        {"settingsLanguage", QStringLiteral("Dil")},
        {"settingsLanguageEn", QStringLiteral("İngilizce")},
        {"settingsLanguageTr", QStringLiteral("Türkçe")},
        {"settingsTargetColor", QStringLiteral("Hedef Rengi")},
        {"settingsTargetRadius", QStringLiteral("Hedef Yarıçapı")},
        {"settingsInput", QStringLiteral("Girdi ve Hassasiyet")},
        {"settingsRawInput", QStringLiteral("Raw Input")},
        {"settingsInvertY", QStringLiteral("Y Eksenini Ters Çevir")},
        {"settingsCm360", QStringLiteral("cm/360")},
        {"settingsDpi", QStringLiteral("DPI")},
        {"settingsSensitivityScale", QStringLiteral("Sens Ölçeği")},
        {"settingsYaw", QStringLiteral("Yaw Çarpanı")},
        {"settingsPitch", QStringLiteral("Pitch Çarpanı")},
        {"settingsCrosshair", QStringLiteral("Nişangah")},
        {"settingsCrosshairColor", QStringLiteral("Nişangah Rengi")},
        {"settingsCrosshairThickness", QStringLiteral("Kalınlık")},
        {"settingsCrosshairCenterDot", QStringLiteral("Merkez Noktası")},
        {"settingsCrosshairCenterDotSize", QStringLiteral("Merkez Nokta Boyutu")},
        {"settingsCrosshairLines", QStringLiteral("Çizgiler")},
        {"settingsCrosshairBorder", QStringLiteral("Kenar Çizgisi")},
        {"settingsCrosshairBorderThickness", QStringLiteral("Kenar Kalınlığı")},
        {"settingsCrosshairHorizontalLength", QStringLiteral("Yatay Uzunluk")},
        {"settingsCrosshairVerticalLength", QStringLiteral("Dikey Uzunluk")},
        {"settingsCrosshairGap", QStringLiteral("Boşluk")},
        {"settingsFps", QStringLiteral("FPS Sayaç")},
        {"settingsFpsEnabled", QStringLiteral("FPS Göster")},
        {"settingsFpsPosition", QStringLiteral("FPS Konumu")},
        {"settingsFpsTopLeft", QStringLiteral("Sol Üst")},
        {"settingsFpsTopRight", QStringLiteral("Sağ Üst")},
        {"settingsFpsBottomLeft", QStringLiteral("Sol Alt")},
        {"settingsFpsBottomRight", QStringLiteral("Sağ Alt")},
        {"settingsWindow", QStringLiteral("Pencere")},
        {"settingsWindowFullscreen", QStringLiteral("Tam Ekranda Başlat")},
        {"settingsWindowFullscreenHint", QStringLiteral("Sonraki açılışta geçerli olur")},
        {"settingsKeybindings", QStringLiteral("Tuş Atamaları")},
        {"settingsKeybindToggleFullscreen", QStringLiteral("Tam Ekranı Aç/Kapat")},
        {"settingsKeybindToggleFpsCounter", QStringLiteral("FPS Sayacı Aç/Kapat")},
        {"settingsKeybindToggleCrosshair", QStringLiteral("Nişangahı Gizle/Göster")},
        {"settingsInstantApply", QStringLiteral("Değişiklikler arayüz, hedefler ve arka plana canlı uygulanır.")},
        {"settingsSound", QStringLiteral("Ses")},
        {"settingsSoundEnabled", QStringLiteral("Ses Efektleri")},
        {"settingsSoundVolume", QStringLiteral("Ses Düzeyi")},
        {"settingsSoundHitVariant", QStringLiteral("İsabet Sesi")},
        {"settingsSoundMissVariant", QStringLiteral("Iska Sesi")},
        {"settingsSoundHitSwish", QStringLiteral("Basketbol Filesi")},
        {"settingsSoundHitClick", QStringLiteral("Tıklama")},
        {"settingsSoundHitGunshot", QStringLiteral("Silah Sesi")},
        {"settingsSoundHitPop", QStringLiteral("Pop")},
        {"settingsSoundMissRicochet", QStringLiteral("Kaçırılan Şut")},
        {"settingsSoundMissEmpty", QStringLiteral("Boş Silah")},
        {"settingsSoundMissBeep", QStringLiteral("Bip")},
        {"settingsSoundMissTap", QStringLiteral("Hafif Tık")},
        {"settingsSoundMissPop", QStringLiteral("Yumuşak Pop")},
        {"settingsSoundMissOops", QStringLiteral("Oops")},

        {"aboutTitle", QStringLiteral("XAimAssist Hakkında")},
        {"aboutVersion", QStringLiteral("Sürüm")},
        {"aboutVersionUnknown", QStringLiteral("Bilinmiyor")},
        {"aboutSummaryTitle", QStringLiteral("Uygulama")},
        {"aboutSummaryBody", QStringLiteral("XAimAssist, FPS nişan becerilerini geliştirmek için tasarlanmış açık kaynaklı bir 3D antrenman uygulamasıdır. Modüler C++ mimarisi, gerçek zamanlı metrikler ve özelleştirilebilir antrenman akışı sunar.")},
        {"aboutHighlightsTitle", QStringLiteral("Öne Çıkanlar")},
        {"aboutFeatureModes", QStringLiteral("Çoklu antrenman modları: Static Sphere, GridShot, Next Shot, Tracking, Strafing.")},
        {"aboutFeatureMetrics", QStringLiteral("Canlı performans metrikleri: skor, isabet, ıska, doğruluk, tepki süresi ve atış/saniye.")},
        {"aboutFeatureStats", QStringLiteral("Oturum geçmişi ve mod bazlı karşılaştırmalı istatistik analizi.")},
        {"aboutFeatureInput", QStringLiteral("Fiziksel hassasiyet modeli (cm/360 + DPI), raw input ve eksen çarpanları.")},
        {"aboutFeatureTheme", QStringLiteral("Hedef rengiyle uyumlu dinamik açık/koyu tema sistemi.")},
        {"aboutBuiltWithTitle", QStringLiteral("Kullanılan Teknolojiler")},
        {"aboutBuiltWithValue", QStringLiteral("Qt 6 • QML • C++17 • VTK • CMake")},
        {"aboutDeveloperTitle", QStringLiteral("Geliştirici")},
        {"aboutDeveloperBody", QStringLiteral("XAimAssist, xaprier tarafından geliştirilen açık kaynaklı bir projedir.")},
        {"aboutGithub", QStringLiteral("GitHub Projesi")},
        {"aboutGithubUrl", QString::fromUtf8(version::APP_GITHUB_URL)},

        {"unitSec", QStringLiteral("sn")},
        {"unitMs", QStringLiteral("ms")},
        {"unitPercent", QStringLiteral("%")},
    };
}

QVariantMap I18nProvider::_BuildEnglish() {
    return {
        {"appTitle", QStringLiteral("XAimAssist")},
        {"appSubtitle", QStringLiteral("Modular FPS Aim Trainer")},

        {"tabMenu", QStringLiteral("Main Menu")},
        {"tabModes", QStringLiteral("Mode Selection")},
        {"tabTraining", QStringLiteral("Training")},
        {"tabStats", QStringLiteral("Statistics")},
        {"tabSettings", QStringLiteral("Settings")},
        {"tabAbout", QStringLiteral("About")},

        {"menuTitle", QStringLiteral("Welcome")},
        {"menuSubtitle", QStringLiteral("Choose a mode, adjust settings, and start training.")},
        {"menuActionModes", QStringLiteral("Choose Mode")},
        {"menuActionStart", QStringLiteral("Start Training")},
        {"menuActionStats", QStringLiteral("Open Statistics")},
        {"menuActionSettings", QStringLiteral("Open Settings")},
        {"menuActiveMode", QStringLiteral("Selected Mode")},
        {"menuSession", QStringLiteral("Session")},

        {"modeTitle", QStringLiteral("Mode Selection")},
        {"modeSubtitle", QStringLiteral("Pick your training mode before starting a run.")},
        {"modeSelect", QStringLiteral("Select this mode")},
        {"modeDuration", QStringLiteral("Default Duration")},
        {"modeConfiguredDuration", QStringLiteral("Training Duration")},
        {"modeDistance", QStringLiteral("Default Distance")},
        {"modeConfiguredDistance", QStringLiteral("Spawn Distance")},
        {"modeConfiguredGridRows", QStringLiteral("Grid Rows")},
        {"modeConfiguredGridColumns", QStringLiteral("Grid Columns")},
        {"modeConfiguredActiveTargets", QStringLiteral("Active Targets")},
        {"modeReset", QStringLiteral("Reset Mode Settings")},
        {"modeStart", QStringLiteral("Start Selected Mode")},
        {"modeNoModes", QStringLiteral("No registered modes found.")},

        {"resultTitle", QStringLiteral("Result")},
        {"resultLive", QStringLiteral("Live Summary")},
        {"resultSaved", QStringLiteral("Saved")},
        {"resultNotSaved", QStringLiteral("Not Saved")},
        {"resultInvalid", QStringLiteral("Invalid")},
        {"resultNotSavedHint", QStringLiteral("Session exited via Exit Training, so this result was not added to history.")},
        {"resultInvalidHint", QStringLiteral("This result is invalid and was not added to history.")},
        {"resultInvalidReasonReaction", QStringLiteral("Reaction time is 0.0 ms")},
        {"resultInvalidReasonHits", QStringLiteral("Hit count is 0")},
        {"resultInvalidReasonScore", QStringLiteral("Score is negative")},
        {"resultInvalidReasonAccuracy", QStringLiteral("Accuracy is 0% or above 100%")},
        {"resultInvalidReasonUnknown", QStringLiteral("Invalid metric state")},
        {"resultLiveHint", QStringLiteral("Selected mode and live metrics are shown here until the session ends.")},
        {"resultMode", QStringLiteral("Mode")},
        {"resultConfiguredDuration", QStringLiteral("Target Duration")},
        {"resultElapsedDuration", QStringLiteral("Elapsed Duration")},
        {"resultStoppedAt", QStringLiteral("Ended At")},
        {"resultStopReason", QStringLiteral("Stop Reason")},
        {"resultStopCompleted", QStringLiteral("Completed")},
        {"resultStopAborted", QStringLiteral("Exited By User")},
        {"resultPerfFirst", QStringLiteral("First Result")},
        {"resultPerfBest", QStringLiteral("Best Performance")},
        {"resultPerfWorst", QStringLiteral("Worst Performance")},
        {"resultPerfAboveAverage", QStringLiteral("Above Average")},
        {"resultPerfBelowAverage", QStringLiteral("Below Average")},
        {"resultPerfAverage", QStringLiteral("Around Average")},
        {"resultPerfUnavailable", QStringLiteral("No Comparison")},
        {"resultPerfCurrentShort", QStringLiteral("Current")},
        {"resultPerfAverageShort", QStringLiteral("Avg")},
        {"resultPerfBestShort", QStringLiteral("Best")},
        {"resultPerfWorstShort", QStringLiteral("Worst")},
        {"resultPerfMetricAverage", QStringLiteral("Mode Average Score")},
        {"resultPerfMetricBest", QStringLiteral("Mode Best Score")},
        {"resultPerfMetricWorst", QStringLiteral("Mode Worst Score")},
        {"resultPerfMetricSessions", QStringLiteral("Mode Session Count")},

        {"trainingTitle", QStringLiteral("Training")},
        {"trainingSubtitle", QStringLiteral("Track real-time metrics and improve consistency.")},
        {"trainingStart", QStringLiteral("Start")},
        {"trainingStop", QStringLiteral("Stop")},
        {"trainingStatus", QStringLiteral("Status")},
        {"trainingStatusActive", QStringLiteral("Active")},
        {"trainingStatusIdle", QStringLiteral("Idle")},
        {"trainingMode", QStringLiteral("Mode")},
        {"trainingElapsed", QStringLiteral("Elapsed")},
        {"trainingScore", QStringLiteral("Score")},
        {"trainingHits", QStringLiteral("Hits")},
        {"trainingMisses", QStringLiteral("Misses")},
        {"trainingAccuracy", QStringLiteral("Accuracy")},
        {"trainingReaction", QStringLiteral("Reaction Time")},
        {"trainingSps", QStringLiteral("Shots/Sec")},
        {"trainingCaptureHint", QStringLiteral("Hint: mouse capture is enabled while training for FPS-style aim control.")},
        {"trainingWaitForClick", QStringLiteral("Left click inside the scene to start training.")},
        {"trainingCountdown", QStringLiteral("Starting")},
        {"trainingPausedTitle", QStringLiteral("Training Paused")},
        {"trainingPausedSubtitle", QStringLiteral("Mode updates and counters are paused.")},
        {"trainingContinue", QStringLiteral("Continue")},
        {"trainingExit", QStringLiteral("Exit Training")},
        {"trainingOpenSettings", QStringLiteral("Settings")},
        {"trainingBack", QStringLiteral("Back")},

        {"statsTitle", QStringLiteral("Statistics")},
        {"statsSubtitle", QStringLiteral("Review session history and compare against your best run.")},
        {"statsBestSession", QStringLiteral("Best Session For This Mode")},
        {"statsNoBest", QStringLiteral("No sessions recorded for this mode yet.")},
        {"statsRecent", QStringLiteral("Recent Sessions")},
        {"statsNoRecent", QStringLiteral("No recent sessions yet.")},
        {"statsScore", QStringLiteral("Score")},
        {"statsAccuracy", QStringLiteral("Accuracy")},
        {"statsReaction", QStringLiteral("Avg Reaction")},
        {"statsDate", QStringLiteral("Date")},
        {"statsShots", QStringLiteral("Shots")},

        {"settingsTitle", QStringLiteral("Settings")},
        {"settingsSubtitle", QStringLiteral("All changes are applied and saved instantly.")},
        {"settingsTheme", QStringLiteral("Theme")},
        {"settingsEnable", QStringLiteral("Enable")},
        {"settingsSize", QStringLiteral("Size")},
        {"settingsThemeDark", QStringLiteral("Dark")},
        {"settingsThemeLight", QStringLiteral("Light")},
        {"settingsLanguage", QStringLiteral("Language")},
        {"settingsLanguageEn", QStringLiteral("English")},
        {"settingsLanguageTr", QStringLiteral("Turkish")},
        {"settingsTargetColor", QStringLiteral("Target Color")},
        {"settingsTargetRadius", QStringLiteral("Target Radius")},
        {"settingsInput", QStringLiteral("Input & Sensitivity")},
        {"settingsRawInput", QStringLiteral("Raw Input")},
        {"settingsInvertY", QStringLiteral("Invert Y")},
        {"settingsCm360", QStringLiteral("cm/360")},
        {"settingsDpi", QStringLiteral("DPI")},
        {"settingsSensitivityScale", QStringLiteral("Sensitivity Scale")},
        {"settingsYaw", QStringLiteral("Yaw Multiplier")},
        {"settingsPitch", QStringLiteral("Pitch Multiplier")},
        {"settingsCrosshair", QStringLiteral("Crosshair")},
        {"settingsCrosshairColor", QStringLiteral("Crosshair Color")},
        {"settingsCrosshairThickness", QStringLiteral("Thickness")},
        {"settingsCrosshairCenterDot", QStringLiteral("Center Dot")},
        {"settingsCrosshairCenterDotSize", QStringLiteral("Center Dot Size")},
        {"settingsCrosshairLines", QStringLiteral("Lines")},
        {"settingsCrosshairBorder", QStringLiteral("Outline")},
        {"settingsCrosshairBorderThickness", QStringLiteral("Outline Thickness")},
        {"settingsCrosshairHorizontalLength", QStringLiteral("Horizontal Length")},
        {"settingsCrosshairVerticalLength", QStringLiteral("Vertical Length")},
        {"settingsCrosshairGap", QStringLiteral("Gap")},
        {"settingsFps", QStringLiteral("FPS Counter")},
        {"settingsFpsEnabled", QStringLiteral("Show FPS")},
        {"settingsFpsPosition", QStringLiteral("FPS Position")},
        {"settingsFpsTopLeft", QStringLiteral("Top Left")},
        {"settingsFpsTopRight", QStringLiteral("Top Right")},
        {"settingsFpsBottomLeft", QStringLiteral("Bottom Left")},
        {"settingsFpsBottomRight", QStringLiteral("Bottom Right")},
        {"settingsWindow", QStringLiteral("Window")},
        {"settingsWindowFullscreen", QStringLiteral("Start in Fullscreen")},
        {"settingsWindowFullscreenHint", QStringLiteral("Takes effect on next launch")},
        {"settingsKeybindings", QStringLiteral("Keybindings")},
        {"settingsKeybindToggleFullscreen", QStringLiteral("Toggle Fullscreen")},
        {"settingsKeybindToggleFpsCounter", QStringLiteral("Toggle FPS Counter")},
        {"settingsKeybindToggleCrosshair", QStringLiteral("Toggle Crosshair")},
        {"settingsInstantApply", QStringLiteral("Changes update UI, targets, and background in real time.")},
        {"settingsSound", QStringLiteral("Sound")},
        {"settingsSoundEnabled", QStringLiteral("Sound Effects")},
        {"settingsSoundVolume", QStringLiteral("Volume")},
        {"settingsSoundHitVariant", QStringLiteral("Hit Sound")},
        {"settingsSoundMissVariant", QStringLiteral("Miss Sound")},
        {"settingsSoundHitSwish", QStringLiteral("Basketball Swish")},
        {"settingsSoundHitClick", QStringLiteral("Click")},
        {"settingsSoundHitGunshot", QStringLiteral("Gunshot")},
        {"settingsSoundHitPop", QStringLiteral("Pop")},
        {"settingsSoundMissRicochet", QStringLiteral("Missed Shot")},
        {"settingsSoundMissEmpty", QStringLiteral("Empty Gun")},
        {"settingsSoundMissBeep", QStringLiteral("Beep")},
        {"settingsSoundMissTap", QStringLiteral("Tap")},
        {"settingsSoundMissPop", QStringLiteral("Soft Pop")},
        {"settingsSoundMissOops", QStringLiteral("Oops")},

        {"aboutTitle", QStringLiteral("About XAimAssist")},
        {"aboutVersion", QStringLiteral("Version")},
        {"aboutVersionUnknown", QStringLiteral("Unknown")},
        {"aboutSummaryTitle", QStringLiteral("Application")},
        {"aboutSummaryBody", QStringLiteral("XAimAssist is an open-source 3D FPS aim training application focused on improving aiming consistency through modular C++ architecture, real-time metrics, and configurable training flows.")},
        {"aboutHighlightsTitle", QStringLiteral("Highlights")},
        {"aboutFeatureModes", QStringLiteral("Multiple built-in modes: Static Sphere, GridShot, Next Shot, Tracking, Strafing.")},
        {"aboutFeatureMetrics", QStringLiteral("Live performance metrics: score, hits, misses, accuracy, reaction time, and shots per second.")},
        {"aboutFeatureStats", QStringLiteral("Session history with per-mode comparative statistics.")},
        {"aboutFeatureInput", QStringLiteral("Physical sensitivity model (cm/360 + DPI), raw input, and axis multipliers.")},
        {"aboutFeatureTheme", QStringLiteral("Dynamic light/dark theme system harmonized with target color.")},
        {"aboutBuiltWithTitle", QStringLiteral("Built With")},
        {"aboutBuiltWithValue", QStringLiteral("Qt 6 • QML • C++17 • VTK • CMake")},
        {"aboutDeveloperTitle", QStringLiteral("Developer")},
        {"aboutDeveloperBody", QStringLiteral("XAimAssist is an open-source project developed by xaprier.")},
        {"aboutGithub", QStringLiteral("Open on GitHub")},
        {"aboutGithubUrl", QString::fromUtf8(version::APP_GITHUB_URL)},

        {"unitSec", QStringLiteral("sec")},
        {"unitMs", QStringLiteral("ms")},
        {"unitPercent", QStringLiteral("%")},
    };
}

}  // namespace xaimassist::ui
