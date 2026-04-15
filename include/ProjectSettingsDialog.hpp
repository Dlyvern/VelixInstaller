#ifndef PROJECT_SETTINGS_DIALOG_HPP
#define PROJECT_SETTINGS_DIALOG_HPP

#include <QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QTabWidget>
#include <QScrollArea>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QLabel>
#include <QProgressBar>

#include "json/json.hpp"
#include "PluginManagerDialog.hpp"

class FireButton;
class VelixText;

class ProjectSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProjectSettingsDialog(QWidget* parent = nullptr);

    [[nodiscard]] nlohmann::json toSettingsJson() const;
    [[nodiscard]] QVector<PluginEntry> selectedPlugins() const;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QWidget* buildCameraTab();
    QWidget* buildRenderingTab();
    QWidget* buildRtxTab();
    QWidget* buildPluginsTab();

    void fetchPluginManifest();
    void onPluginManifestFetched(QNetworkReply* reply);

    static bool detectRtxCapability();

    // Template
    QComboBox* m_templateCombo{nullptr};
    void applyTemplate(int index);

    // Camera
    QDoubleSpinBox* m_fovSpin{nullptr};
    QDoubleSpinBox* m_nearPlaneSpin{nullptr};
    QDoubleSpinBox* m_farPlaneSpin{nullptr};
    QDoubleSpinBox* m_moveSpeedSpin{nullptr};
    QDoubleSpinBox* m_mouseSensSpin{nullptr};

    // Rendering – general
    QDoubleSpinBox* m_renderScaleSpin{nullptr};
    QCheckBox*      m_vsyncCheck{nullptr};
    QCheckBox*      m_fxaaCheck{nullptr};
    QCheckBox*      m_smaaCheck{nullptr};
    QCheckBox*      m_taaCheck{nullptr};
    QComboBox*      m_anisotropyCombo{nullptr};

    // Rendering – Ambient Occlusion
    QCheckBox*      m_ssaoCheck{nullptr};
    QDoubleSpinBox* m_ssaoRadiusSpin{nullptr};
    QDoubleSpinBox* m_ssaoBiasSpin{nullptr};
    QSpinBox*       m_ssaoSamplesSpin{nullptr};
    QDoubleSpinBox* m_ssaoStrengthSpin{nullptr};
    QCheckBox*      m_gtaoCheck{nullptr};
    QSpinBox*       m_gtaoDirectionsSpin{nullptr};
    QSpinBox*       m_gtaoStepsSpin{nullptr};
    QCheckBox*      m_bentNormalsCheck{nullptr};

    // Rendering – Bloom
    QCheckBox*      m_bloomCheck{nullptr};
    QDoubleSpinBox* m_bloomStrengthSpin{nullptr};
    QDoubleSpinBox* m_bloomThresholdSpin{nullptr};
    QDoubleSpinBox* m_bloomKneeSpin{nullptr};

    // Rendering – SSR
    QCheckBox*      m_ssrCheck{nullptr};
    QDoubleSpinBox* m_ssrMaxDistanceSpin{nullptr};
    QSpinBox*       m_ssrStepsSpin{nullptr};
    QDoubleSpinBox* m_ssrStrengthSpin{nullptr};
    QDoubleSpinBox* m_ssrRoughnessCutoffSpin{nullptr};

    // Rendering – Shadows
    QSpinBox*       m_shadowQualitySpin{nullptr};
    QSpinBox*       m_shadowCascadesSpin{nullptr};
    QDoubleSpinBox* m_shadowMaxDistSpin{nullptr};
    QCheckBox*      m_contactShadowsCheck{nullptr};

    // Rendering – Volumetric Fog
    QComboBox*      m_volumetricFogQualityCombo{nullptr};
    QCheckBox*      m_volumetricFogOverrideCheck{nullptr};

    // Rendering – Post-process
    QCheckBox*      m_postProcessingCheck{nullptr};
    QCheckBox*      m_colorGradingCheck{nullptr};
    QDoubleSpinBox* m_cgSaturationSpin{nullptr};
    QDoubleSpinBox* m_cgContrastSpin{nullptr};
    QDoubleSpinBox* m_cgTemperatureSpin{nullptr};
    QDoubleSpinBox* m_cgTintSpin{nullptr};
    QCheckBox*      m_chromAberrationCheck{nullptr};
    QDoubleSpinBox* m_chromAberrationStrengthSpin{nullptr};
    QCheckBox*      m_vignetteCheck{nullptr};
    QDoubleSpinBox* m_vignetteStrengthSpin{nullptr};
    QCheckBox*      m_filmGrainCheck{nullptr};
    QDoubleSpinBox* m_filmGrainStrengthSpin{nullptr};

    // RTX
    QCheckBox*      m_rtxCheck{nullptr};
    QCheckBox*      m_rtShadowsCheck{nullptr};
    QCheckBox*      m_rtReflectionsCheck{nullptr};
    QComboBox*      m_rtModeCombo{nullptr};
    bool            m_rtxCapable{false};

    // Plugins
    QNetworkAccessManager* m_pluginNetManager{nullptr};
    QVBoxLayout*           m_pluginListLayout{nullptr};
    QLabel*                m_pluginStatusLabel{nullptr};
    QVector<PluginEntry>   m_manifestPlugins;
    QMap<QString, bool>    m_pluginSelection;   // name -> checked

    QPoint m_dragOffset;
    bool   m_dragging{false};
};

#endif //PROJECT_SETTINGS_DIALOG_HPP
