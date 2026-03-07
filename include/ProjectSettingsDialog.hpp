#ifndef PROJECT_SETTINGS_DIALOG_HPP
#define PROJECT_SETTINGS_DIALOG_HPP

#include <QDialog>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QTabWidget>
#include <QScrollArea>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>

#include "json/json.hpp"

class FireButton;
class VelixText;

class ProjectSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProjectSettingsDialog(QWidget* parent = nullptr);

    [[nodiscard]] nlohmann::json toSettingsJson() const;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QWidget* buildCameraTab();
    QWidget* buildRenderingTab();
    QWidget* buildRtxTab();

    static bool detectRtxCapability();

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
    QSpinBox*       m_anisotropySpin{nullptr};

    // Rendering – SSAO
    QCheckBox*      m_ssaoCheck{nullptr};
    QDoubleSpinBox* m_ssaoRadiusSpin{nullptr};
    QDoubleSpinBox* m_ssaoBiasSpin{nullptr};
    QSpinBox*       m_ssaoSamplesSpin{nullptr};
    QDoubleSpinBox* m_ssaoStrengthSpin{nullptr};

    // Rendering – Bloom
    QCheckBox*      m_bloomCheck{nullptr};
    QDoubleSpinBox* m_bloomStrengthSpin{nullptr};
    QDoubleSpinBox* m_bloomThresholdSpin{nullptr};
    QDoubleSpinBox* m_bloomKneeSpin{nullptr};

    // Rendering – Shadows
    QSpinBox*       m_shadowQualitySpin{nullptr};
    QSpinBox*       m_shadowCascadesSpin{nullptr};
    QDoubleSpinBox* m_shadowMaxDistSpin{nullptr};
    QCheckBox*      m_contactShadowsCheck{nullptr};

    // Rendering – Post-process
    QCheckBox*      m_postProcessingCheck{nullptr};
    QCheckBox*      m_colorGradingCheck{nullptr};
    QCheckBox*      m_chromAberrationCheck{nullptr};
    QCheckBox*      m_vignetteCheck{nullptr};
    QCheckBox*      m_filmGrainCheck{nullptr};
    QCheckBox*      m_gtaoCheck{nullptr};

    // RTX
    QCheckBox*      m_rtxCheck{nullptr};
    bool            m_rtxCapable{false};

    QPoint m_dragOffset;
    bool   m_dragging{false};
};

#endif //PROJECT_SETTINGS_DIALOG_HPP
