#include "ProjectSettingsDialog.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QScrollArea>
#include <QTabWidget>
#include <QTabBar>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QProcess>
#include <QApplication>
#include <QScreen>
#include <QMouseEvent>

#include "widgets/VelixText.hpp"
#include "FireButton.hpp"

namespace
{
static const QString kDialogStyle = R"(
QWidget#tabContent {
    background-color: transparent;
}
QCheckBox {
    color: #c8c8c8;
    spacing: 8px;
    font-size: 12px;
}
QCheckBox::indicator {
    width: 15px;
    height: 15px;
    border-radius: 3px;
    border: 1px solid #505050;
    background-color: #252525;
}
QCheckBox::indicator:checked {
    background-color: #e04800;
    border-color: #ff6620;
    image: none;
}
QCheckBox::indicator:disabled {
    background-color: #1e1e1e;
    border-color: #383838;
}
QDoubleSpinBox, QSpinBox {
    background-color: #1c1c1c;
    border: 1px solid #424242;
    border-radius: 5px;
    color: #dcdcdc;
    padding: 3px 8px;
    min-height: 26px;
    font-size: 12px;
}
QDoubleSpinBox:focus, QSpinBox:focus {
    border-color: #e04800;
}
QDoubleSpinBox::up-button, QSpinBox::up-button,
QDoubleSpinBox::down-button, QSpinBox::down-button {
    background-color: #2e2e2e;
    border: none;
    width: 18px;
}
QDoubleSpinBox::up-arrow, QSpinBox::up-arrow {
    width: 7px; height: 7px;
}
QDoubleSpinBox::down-arrow, QSpinBox::down-arrow {
    width: 7px; height: 7px;
}
QTabWidget::pane {
    background-color: #181818;
    border: 1px solid #303030;
    border-top: none;
    border-bottom-left-radius: 8px;
    border-bottom-right-radius: 8px;
}
QTabBar::tab {
    background-color: #222222;
    color: #888888;
    padding: 9px 22px;
    border: 1px solid #303030;
    border-bottom: none;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
    margin-right: 3px;
    font-size: 12px;
    font-weight: bold;
}
QTabBar::tab:selected {
    background-color: #181818;
    color: #ff5c10;
    border-bottom: 2px solid #e04800;
}
QTabBar::tab:hover:!selected {
    background-color: #2a2a2a;
    color: #cccccc;
}
QGroupBox {
    border: 1px solid #323232;
    border-radius: 7px;
    margin-top: 18px;
    padding-top: 6px;
    color: #888;
    font-size: 11px;
    font-weight: bold;
    background-color: transparent;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 10px;
    padding: 0 6px;
    color: #cc5018;
    font-size: 11px;
}
QLabel {
    color: #aaaaaa;
    font-size: 12px;
    background: transparent;
}
QScrollArea {
    background-color: transparent;
    border: none;
}
QScrollBar:vertical {
    background-color: #141414;
    width: 7px;
    border-radius: 3px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background-color: #404040;
    border-radius: 3px;
    min-height: 20px;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
)";

QWidget* makeSeparator(QWidget* parent)
{
    auto* sep = new QWidget(parent);
    sep->setFixedHeight(1);
    sep->setStyleSheet("background-color: #2e2e2e;");
    return sep;
}

QDoubleSpinBox* makeDoubleSpin(double min, double max, double step, double value, QWidget* parent)
{
    auto* spin = new QDoubleSpinBox(parent);
    spin->setRange(min, max);
    spin->setSingleStep(step);
    spin->setValue(value);
    spin->setFixedWidth(110);
    return spin;
}

QSpinBox* makeIntSpin(int min, int max, int value, QWidget* parent)
{
    auto* spin = new QSpinBox(parent);
    spin->setRange(min, max);
    spin->setValue(value);
    spin->setFixedWidth(110);
    return spin;
}

void addRow(QGridLayout* grid, int& row, const QString& label, QWidget* control)
{
    auto* lbl = new QLabel(label, grid->parentWidget());
    grid->addWidget(lbl, row, 0, Qt::AlignVCenter);
    grid->addWidget(control, row, 1, Qt::AlignVCenter);
    ++row;
}

void addCheck(QVBoxLayout* layout, QCheckBox* check, const QString& label)
{
    check->setText(label);
    layout->addWidget(check);
}

static const QString kComboStyle = R"(
QComboBox {
    background-color: #1c1c1c;
    border: 1px solid #424242;
    border-radius: 5px;
    color: #dcdcdc;
    padding: 3px 8px;
    min-height: 26px;
    font-size: 12px;
}
QComboBox:focus { border-color: #e04800; }
QComboBox::drop-down { border: none; width: 20px; }
QComboBox QAbstractItemView {
    background-color: #1c1c1c;
    color: #dcdcdc;
    selection-background-color: #e04800;
}
)";

QComboBox* makeCombo(QWidget* parent, const QStringList& items, int defaultIndex = 0)
{
    auto* combo = new QComboBox(parent);
    combo->setStyleSheet(kComboStyle);
    combo->addItems(items);
    combo->setCurrentIndex(defaultIndex);
    combo->setFixedWidth(150);
    return combo;
}
} // namespace

ProjectSettingsDialog::ProjectSettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    m_rtxCapable = detectRtxCapability();

    setWindowTitle("Project Settings");
    setFixedSize(700, 560);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet(kDialogStyle);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(14, 14, 14, 14);
    rootLayout->setSpacing(10);

    // ── Title bar ────────────────────────────────────────────────────────────
    auto* titleBar = new QWidget(this);
    titleBar->setFixedHeight(42);
    titleBar->setAttribute(Qt::WA_TransparentForMouseEvents, false);

    auto* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(6, 0, 0, 0);

    auto* titleLabel = new VelixText("New Project Settings", titleBar);
    titleLabel->setPointSize(13);
    titleLabel->setTextColor(Qt::white);

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch(1);

    rootLayout->addWidget(titleBar);
    rootLayout->addWidget(makeSeparator(this));

    // ── Tabs ─────────────────────────────────────────────────────────────────
    auto* tabs = new QTabWidget(this);
    tabs->addTab(buildCameraTab(),    "Camera");
    tabs->addTab(buildRenderingTab(), "Rendering");
    tabs->addTab(buildRtxTab(),       "RTX");
    rootLayout->addWidget(tabs, 1);

    // ── Buttons ───────────────────────────────────────────────────────────────
    rootLayout->addWidget(makeSeparator(this));

    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    btnLayout->addStretch(1);

    auto* cancelBtn = new FireButton("Cancel", FireButton::Variant::Secondary, this);
    cancelBtn->setFixedWidth(110);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    auto* createBtn = new FireButton("Create Project", FireButton::Variant::Primary, this);
    createBtn->setFixedWidth(145);
    connect(createBtn, &QPushButton::clicked, this, &QDialog::accept);

    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(createBtn);
    rootLayout->addLayout(btnLayout);

    // Center on screen
    if (const QScreen* screen = QApplication::primaryScreen())
    {
        const QRect sg = screen->availableGeometry();
        move(sg.center() - rect().center());
    }
}

QWidget* ProjectSettingsDialog::buildCameraTab()
{
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);

    auto* content = new QWidget();
    content->setObjectName("tabContent");
    auto* mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(16, 14, 16, 14);
    mainLayout->setSpacing(14);

    auto* group = new QGroupBox("Camera Defaults", content);
    auto* grid = new QGridLayout(group);
    grid->setColumnStretch(0, 1);
    grid->setColumnMinimumWidth(1, 120);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(10);

    int row = 0;
    m_fovSpin        = makeDoubleSpin(10.0, 180.0, 1.0, 60.0, group);
    m_nearPlaneSpin  = makeDoubleSpin(0.001, 10.0, 0.01, 0.1, group);
    m_farPlaneSpin   = makeDoubleSpin(10.0, 100000.0, 10.0, 1000.0, group);
    m_moveSpeedSpin  = makeDoubleSpin(0.1, 100.0, 0.1, 3.0, group);
    m_mouseSensSpin  = makeDoubleSpin(0.001, 1.0, 0.01, 0.1, group);
    m_nearPlaneSpin->setDecimals(3);
    m_mouseSensSpin->setDecimals(3);

    addRow(grid, row, "Field of View (deg)", m_fovSpin);
    addRow(grid, row, "Near Plane", m_nearPlaneSpin);
    addRow(grid, row, "Far Plane", m_farPlaneSpin);
    addRow(grid, row, "Move Speed", m_moveSpeedSpin);
    addRow(grid, row, "Mouse Sensitivity", m_mouseSensSpin);

    mainLayout->addWidget(group);
    mainLayout->addStretch(1);

    scroll->setWidget(content);
    return scroll;
}

QWidget* ProjectSettingsDialog::buildRenderingTab()
{
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);

    auto* content = new QWidget();
    content->setObjectName("tabContent");
    auto* mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(16, 14, 16, 14);
    mainLayout->setSpacing(12);

    // ── General ──────────────────────────────────────────────────────────────
    auto* genGroup = new QGroupBox("General", content);
    auto* genGrid = new QGridLayout(genGroup);
    genGrid->setColumnStretch(0, 1);
    genGrid->setHorizontalSpacing(16);
    genGrid->setVerticalSpacing(8);

    int row = 0;
    m_renderScaleSpin = makeDoubleSpin(0.25, 2.0, 0.05, 1.0, genGroup);
    addRow(genGrid, row, "Render Scale", m_renderScaleSpin);

    m_anisotropyCombo = makeCombo(genGroup, {"Off", "2×", "4×", "8×", "16×"}, 2);
    addRow(genGrid, row, "Anisotropy", m_anisotropyCombo);

    auto* flagsLayout = new QHBoxLayout();
    m_vsyncCheck = new QCheckBox(genGroup);  m_vsyncCheck->setText("VSync");
    m_fxaaCheck  = new QCheckBox(genGroup);  m_fxaaCheck->setText("FXAA");  m_fxaaCheck->setChecked(true);
    m_smaaCheck  = new QCheckBox(genGroup);  m_smaaCheck->setText("SMAA");
    m_taaCheck   = new QCheckBox(genGroup);  m_taaCheck->setText("TAA");
    flagsLayout->addWidget(m_vsyncCheck);
    flagsLayout->addWidget(m_fxaaCheck);
    flagsLayout->addWidget(m_smaaCheck);
    flagsLayout->addWidget(m_taaCheck);
    flagsLayout->addStretch(1);
    genGrid->addWidget(new QLabel("Anti-aliasing / Sync", genGroup), row, 0, Qt::AlignVCenter);
    genGrid->addLayout(flagsLayout, row, 1);

    mainLayout->addWidget(genGroup);

    // ── Ambient Occlusion ────────────────────────────────────────────────────
    auto* aoGroup = new QGroupBox("Ambient Occlusion", content);
    auto* aoLayout = new QVBoxLayout(aoGroup);
    aoLayout->setSpacing(8);

    // SSAO
    m_ssaoCheck = new QCheckBox(aoGroup);
    m_ssaoCheck->setText("Enable SSAO");
    m_ssaoCheck->setChecked(true);
    aoLayout->addWidget(m_ssaoCheck);

    auto* ssaoGrid = new QGridLayout();
    ssaoGrid->setColumnStretch(0, 1);
    ssaoGrid->setHorizontalSpacing(16);
    ssaoGrid->setVerticalSpacing(8);
    int sr = 0;
    m_ssaoRadiusSpin   = makeDoubleSpin(0.01, 5.0, 0.05, 0.5, aoGroup);
    m_ssaoBiasSpin     = makeDoubleSpin(0.001, 0.5, 0.005, 0.025, aoGroup);
    m_ssaoSamplesSpin  = makeIntSpin(4, 128, 32, aoGroup);
    m_ssaoStrengthSpin = makeDoubleSpin(0.1, 5.0, 0.1, 1.2, aoGroup);
    m_ssaoBiasSpin->setDecimals(3);
    addRow(ssaoGrid, sr, "Radius", m_ssaoRadiusSpin);
    addRow(ssaoGrid, sr, "Bias", m_ssaoBiasSpin);
    addRow(ssaoGrid, sr, "Samples", m_ssaoSamplesSpin);
    addRow(ssaoGrid, sr, "Strength", m_ssaoStrengthSpin);
    aoLayout->addLayout(ssaoGrid);

    aoLayout->addWidget(makeSeparator(aoGroup));

    // GTAO
    m_gtaoCheck = new QCheckBox(aoGroup);
    m_gtaoCheck->setText("Enable GTAO (Ground-Truth AO)");
    aoLayout->addWidget(m_gtaoCheck);

    auto* gtaoGrid = new QGridLayout();
    gtaoGrid->setColumnStretch(0, 1);
    gtaoGrid->setHorizontalSpacing(16);
    gtaoGrid->setVerticalSpacing(8);
    int gr = 0;
    m_gtaoDirectionsSpin = makeIntSpin(2, 8, 4, aoGroup);
    m_gtaoStepsSpin      = makeIntSpin(2, 8, 4, aoGroup);
    addRow(gtaoGrid, gr, "Directions", m_gtaoDirectionsSpin);
    addRow(gtaoGrid, gr, "Steps", m_gtaoStepsSpin);
    aoLayout->addLayout(gtaoGrid);

    m_bentNormalsCheck = new QCheckBox(aoGroup);
    m_bentNormalsCheck->setText("Use Bent Normals (requires GTAO)");
    aoLayout->addWidget(m_bentNormalsCheck);

    connect(m_gtaoCheck, &QCheckBox::toggled, m_gtaoDirectionsSpin, &QWidget::setEnabled);
    connect(m_gtaoCheck, &QCheckBox::toggled, m_gtaoStepsSpin,      &QWidget::setEnabled);
    connect(m_gtaoCheck, &QCheckBox::toggled, m_bentNormalsCheck,   &QWidget::setEnabled);
    m_gtaoDirectionsSpin->setEnabled(false);
    m_gtaoStepsSpin->setEnabled(false);
    m_bentNormalsCheck->setEnabled(false);

    mainLayout->addWidget(aoGroup);

    // ── Bloom ────────────────────────────────────────────────────────────────
    auto* bloomGroup = new QGroupBox("Bloom", content);
    auto* bloomLayout = new QVBoxLayout(bloomGroup);
    bloomLayout->setSpacing(8);

    m_bloomCheck = new QCheckBox(bloomGroup);
    m_bloomCheck->setText("Enable Bloom");
    m_bloomCheck->setChecked(true);
    bloomLayout->addWidget(m_bloomCheck);

    auto* bloomGrid = new QGridLayout();
    bloomGrid->setColumnStretch(0, 1);
    bloomGrid->setHorizontalSpacing(16);
    bloomGrid->setVerticalSpacing(8);
    int br = 0;
    m_bloomStrengthSpin  = makeDoubleSpin(0.0, 5.0, 0.05, 0.5, bloomGroup);
    m_bloomThresholdSpin = makeDoubleSpin(0.0, 5.0, 0.05, 0.85, bloomGroup);
    m_bloomKneeSpin      = makeDoubleSpin(0.0, 1.0, 0.01, 0.1, bloomGroup);
    addRow(bloomGrid, br, "Strength", m_bloomStrengthSpin);
    addRow(bloomGrid, br, "Threshold", m_bloomThresholdSpin);
    addRow(bloomGrid, br, "Knee", m_bloomKneeSpin);
    bloomLayout->addLayout(bloomGrid);

    mainLayout->addWidget(bloomGroup);

    // ── SSR ──────────────────────────────────────────────────────────────────
    auto* ssrGroup = new QGroupBox("Screen Space Reflections (SSR)", content);
    auto* ssrLayout = new QVBoxLayout(ssrGroup);
    ssrLayout->setSpacing(8);

    m_ssrCheck = new QCheckBox(ssrGroup);
    m_ssrCheck->setText("Enable SSR");
    ssrLayout->addWidget(m_ssrCheck);

    auto* ssrGrid = new QGridLayout();
    ssrGrid->setColumnStretch(0, 1);
    ssrGrid->setHorizontalSpacing(16);
    ssrGrid->setVerticalSpacing(8);
    int sr2 = 0;
    m_ssrMaxDistanceSpin     = makeDoubleSpin(1.0, 100.0, 1.0, 15.0, ssrGroup);
    m_ssrStepsSpin           = makeIntSpin(8, 256, 64, ssrGroup);
    m_ssrStrengthSpin        = makeDoubleSpin(0.0, 1.0, 0.05, 0.8, ssrGroup);
    m_ssrRoughnessCutoffSpin = makeDoubleSpin(0.05, 0.8, 0.05, 0.3, ssrGroup);
    addRow(ssrGrid, sr2, "Max Distance",     m_ssrMaxDistanceSpin);
    addRow(ssrGrid, sr2, "Steps",            m_ssrStepsSpin);
    addRow(ssrGrid, sr2, "Strength",         m_ssrStrengthSpin);
    addRow(ssrGrid, sr2, "Roughness Cutoff", m_ssrRoughnessCutoffSpin);
    ssrLayout->addLayout(ssrGrid);

    mainLayout->addWidget(ssrGroup);

    // ── Shadows ──────────────────────────────────────────────────────────────
    auto* shadowGroup = new QGroupBox("Shadows", content);
    auto* shadowLayout = new QVBoxLayout(shadowGroup);
    shadowLayout->setSpacing(8);

    m_contactShadowsCheck = new QCheckBox(shadowGroup);
    m_contactShadowsCheck->setText("Enable Contact Shadows");
    shadowLayout->addWidget(m_contactShadowsCheck);

    auto* shadowGrid = new QGridLayout();
    shadowGrid->setColumnStretch(0, 1);
    shadowGrid->setHorizontalSpacing(16);
    shadowGrid->setVerticalSpacing(8);
    int shRow = 0;
    m_shadowQualitySpin  = makeIntSpin(256, 8192, 4096, shadowGroup);
    m_shadowCascadesSpin = makeIntSpin(1, 8, 4, shadowGroup);
    m_shadowMaxDistSpin  = makeDoubleSpin(10.0, 2000.0, 10.0, 180.0, shadowGroup);
    addRow(shadowGrid, shRow, "Shadow Map Resolution", m_shadowQualitySpin);
    addRow(shadowGrid, shRow, "Cascade Count",         m_shadowCascadesSpin);
    addRow(shadowGrid, shRow, "Max Distance",          m_shadowMaxDistSpin);
    shadowLayout->addLayout(shadowGrid);

    mainLayout->addWidget(shadowGroup);

    // ── Volumetric Fog ───────────────────────────────────────────────────────
    auto* fogGroup = new QGroupBox("Volumetric Fog", content);
    auto* fogGrid  = new QGridLayout(fogGroup);
    fogGrid->setColumnStretch(0, 1);
    fogGrid->setHorizontalSpacing(16);
    fogGrid->setVerticalSpacing(8);

    int fogRow = 0;
    m_volumetricFogQualityCombo = makeCombo(fogGroup, {"Off", "Low", "High"}, 0);
    addRow(fogGrid, fogRow, "Quality", m_volumetricFogQualityCombo);

    m_volumetricFogOverrideCheck = new QCheckBox(fogGroup);
    m_volumetricFogOverrideCheck->setText("Override scene fog settings");
    fogGrid->addWidget(m_volumetricFogOverrideCheck, fogRow, 0, 1, 2);

    mainLayout->addWidget(fogGroup);

    // ── Post-Processing ───────────────────────────────────────────────────────
    auto* ppGroup = new QGroupBox("Post-Processing", content);
    auto* ppLayout = new QVBoxLayout(ppGroup);
    ppLayout->setSpacing(6);

    m_postProcessingCheck = new QCheckBox(ppGroup);
    addCheck(ppLayout, m_postProcessingCheck, "Enable Post-Processing");
    m_postProcessingCheck->setChecked(true);

    ppLayout->addWidget(makeSeparator(ppGroup));

    // Color Grading
    m_colorGradingCheck = new QCheckBox(ppGroup);
    addCheck(ppLayout, m_colorGradingCheck, "Color Grading");

    auto* cgGrid = new QGridLayout();
    cgGrid->setColumnStretch(0, 1);
    cgGrid->setHorizontalSpacing(16);
    cgGrid->setVerticalSpacing(6);
    int cgRow = 0;
    m_cgSaturationSpin  = makeDoubleSpin(0.0,  2.0,  0.05, 1.0, ppGroup);
    m_cgContrastSpin    = makeDoubleSpin(0.0,  2.0,  0.05, 1.0, ppGroup);
    m_cgTemperatureSpin = makeDoubleSpin(-1.0, 1.0,  0.05, 0.0, ppGroup);
    m_cgTintSpin        = makeDoubleSpin(-1.0, 1.0,  0.05, 0.0, ppGroup);
    addRow(cgGrid, cgRow, "Saturation",  m_cgSaturationSpin);
    addRow(cgGrid, cgRow, "Contrast",    m_cgContrastSpin);
    addRow(cgGrid, cgRow, "Temperature", m_cgTemperatureSpin);
    addRow(cgGrid, cgRow, "Tint",        m_cgTintSpin);
    ppLayout->addLayout(cgGrid);

    connect(m_colorGradingCheck, &QCheckBox::toggled, m_cgSaturationSpin,  &QWidget::setEnabled);
    connect(m_colorGradingCheck, &QCheckBox::toggled, m_cgContrastSpin,    &QWidget::setEnabled);
    connect(m_colorGradingCheck, &QCheckBox::toggled, m_cgTemperatureSpin, &QWidget::setEnabled);
    connect(m_colorGradingCheck, &QCheckBox::toggled, m_cgTintSpin,        &QWidget::setEnabled);
    m_cgSaturationSpin->setEnabled(false);
    m_cgContrastSpin->setEnabled(false);
    m_cgTemperatureSpin->setEnabled(false);
    m_cgTintSpin->setEnabled(false);

    ppLayout->addWidget(makeSeparator(ppGroup));

    // Chromatic Aberration
    auto* caGrid = new QGridLayout();
    caGrid->setColumnStretch(0, 1);
    caGrid->setHorizontalSpacing(16);
    int caRow = 0;
    m_chromAberrationCheck = new QCheckBox(ppGroup);
    m_chromAberrationCheck->setText("Chromatic Aberration");
    caGrid->addWidget(m_chromAberrationCheck, caRow, 0, 1, 2);
    ++caRow;
    m_chromAberrationStrengthSpin = makeDoubleSpin(0.0, 0.02, 0.001, 0.003, ppGroup);
    m_chromAberrationStrengthSpin->setDecimals(3);
    m_chromAberrationStrengthSpin->setEnabled(false);
    addRow(caGrid, caRow, "Strength", m_chromAberrationStrengthSpin);
    ppLayout->addLayout(caGrid);
    connect(m_chromAberrationCheck, &QCheckBox::toggled, m_chromAberrationStrengthSpin, &QWidget::setEnabled);

    // Vignette
    auto* vigGrid = new QGridLayout();
    vigGrid->setColumnStretch(0, 1);
    vigGrid->setHorizontalSpacing(16);
    int vigRow = 0;
    m_vignetteCheck = new QCheckBox(ppGroup);
    m_vignetteCheck->setText("Vignette");
    vigGrid->addWidget(m_vignetteCheck, vigRow, 0, 1, 2);
    ++vigRow;
    m_vignetteStrengthSpin = makeDoubleSpin(0.0, 1.0, 0.05, 0.4, ppGroup);
    m_vignetteStrengthSpin->setEnabled(false);
    addRow(vigGrid, vigRow, "Strength", m_vignetteStrengthSpin);
    ppLayout->addLayout(vigGrid);
    connect(m_vignetteCheck, &QCheckBox::toggled, m_vignetteStrengthSpin, &QWidget::setEnabled);

    // Film Grain
    auto* fgGrid = new QGridLayout();
    fgGrid->setColumnStretch(0, 1);
    fgGrid->setHorizontalSpacing(16);
    int fgRow = 0;
    m_filmGrainCheck = new QCheckBox(ppGroup);
    m_filmGrainCheck->setText("Film Grain");
    fgGrid->addWidget(m_filmGrainCheck, fgRow, 0, 1, 2);
    ++fgRow;
    m_filmGrainStrengthSpin = makeDoubleSpin(0.0, 0.2, 0.005, 0.03, ppGroup);
    m_filmGrainStrengthSpin->setDecimals(3);
    m_filmGrainStrengthSpin->setEnabled(false);
    addRow(fgGrid, fgRow, "Strength", m_filmGrainStrengthSpin);
    ppLayout->addLayout(fgGrid);
    connect(m_filmGrainCheck, &QCheckBox::toggled, m_filmGrainStrengthSpin, &QWidget::setEnabled);

    mainLayout->addWidget(ppGroup);
    mainLayout->addStretch(1);

    scroll->setWidget(content);
    return scroll;
}

QWidget* ProjectSettingsDialog::buildRtxTab()
{
    auto* content = new QWidget(this);
    content->setObjectName("tabContent");
    auto* mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    // GPU status card
    auto* statusGroup = new QGroupBox("GPU Capability", content);
    auto* statusLayout = new QVBoxLayout(statusGroup);
    statusLayout->setContentsMargins(14, 14, 14, 14);
    statusLayout->setSpacing(8);

    const QString gpuStatusText = m_rtxCapable
        ? "RTX-capable GPU detected. Hardware ray tracing is supported."
        : "No RTX-capable GPU detected. Hardware ray tracing is not supported on this machine.";

    const QColor gpuStatusColor = m_rtxCapable ? QColor(100, 220, 100) : QColor(220, 100, 80);

    auto* gpuLabel = new VelixText(gpuStatusText, statusGroup);
    gpuLabel->setPointSize(10);
    gpuLabel->setTextColor(gpuStatusColor);
    gpuLabel->setWordWrap(true);
    statusLayout->addWidget(gpuLabel);

    mainLayout->addWidget(statusGroup);

    // RTX toggle
    auto* rtxGroup = new QGroupBox("Ray Tracing", content);
    auto* rtxLayout = new QVBoxLayout(rtxGroup);
    rtxLayout->setContentsMargins(14, 14, 14, 14);
    rtxLayout->setSpacing(10);

    m_rtxCheck = new QCheckBox(rtxGroup);
    m_rtxCheck->setText("Enable Ray Tracing");
    m_rtxCheck->setEnabled(m_rtxCapable);
    rtxLayout->addWidget(m_rtxCheck);

    auto* rtModeGrid = new QGridLayout();
    rtModeGrid->setColumnStretch(0, 1);
    rtModeGrid->setHorizontalSpacing(16);
    rtModeGrid->setVerticalSpacing(8);
    int rtRow = 0;

    m_rtModeCombo = makeCombo(rtxGroup, {"Ray Query (Hybrid)", "Pipeline (Full RT)"});
    m_rtModeCombo->setFixedWidth(160);
    m_rtModeCombo->setEnabled(m_rtxCapable);
    auto* rtModeLbl = new QLabel("Ray Tracing Mode", rtxGroup);
    rtModeGrid->addWidget(rtModeLbl, rtRow, 0, Qt::AlignVCenter);
    rtModeGrid->addWidget(m_rtModeCombo, rtRow, 1, Qt::AlignVCenter);
    ++rtRow;

    rtxLayout->addLayout(rtModeGrid);
    rtxLayout->addSpacing(4);

    auto* rtFeaturesGroup = new QGroupBox("Features", rtxGroup);
    auto* rtFeaturesLayout = new QVBoxLayout(rtFeaturesGroup);
    rtFeaturesLayout->setSpacing(6);

    m_rtShadowsCheck = new QCheckBox(rtFeaturesGroup);
    m_rtShadowsCheck->setText("RT Shadows");
    m_rtShadowsCheck->setEnabled(m_rtxCapable);
    rtFeaturesLayout->addWidget(m_rtShadowsCheck);

    m_rtReflectionsCheck = new QCheckBox(rtFeaturesGroup);
    m_rtReflectionsCheck->setText("RT Reflections");
    m_rtReflectionsCheck->setEnabled(m_rtxCapable);
    rtFeaturesLayout->addWidget(m_rtReflectionsCheck);

    rtxLayout->addWidget(rtFeaturesGroup);

    // Disable sub-options when main RT toggle is off
    connect(m_rtxCheck, &QCheckBox::toggled, m_rtModeCombo,       &QWidget::setEnabled);
    connect(m_rtxCheck, &QCheckBox::toggled, m_rtShadowsCheck,    &QWidget::setEnabled);
    connect(m_rtxCheck, &QCheckBox::toggled, m_rtReflectionsCheck,&QWidget::setEnabled);

    if (!m_rtxCapable)
    {
        auto* noRtxLabel = new VelixText(
            "No RTX-capable GPU detected. Ray tracing settings will have no effect at runtime.",
            rtxGroup
        );
        noRtxLabel->setPointSize(9);
        noRtxLabel->setBold(false);
        noRtxLabel->setTextColor(QColor(160, 100, 80));
        noRtxLabel->setWordWrap(true);
        rtxLayout->addWidget(noRtxLabel);
    }

    mainLayout->addWidget(rtxGroup);
    mainLayout->addStretch(1);

    return content;
}

bool ProjectSettingsDialog::detectRtxCapability()
{
    QProcess proc;
    proc.start("nvidia-smi", {"--query-gpu=name", "--format=csv,noheader"});
    if (!proc.waitForFinished(3000))
        return false;

    const QString output = QString::fromLocal8Bit(proc.readAllStandardOutput()).toLower();
    return output.contains("rtx");
}

nlohmann::json ProjectSettingsDialog::toSettingsJson() const
{
    nlohmann::json j;

    j["camera"] = {
        {"fov",              m_fovSpin->value()},
        {"near_plane",       m_nearPlaneSpin->value()},
        {"far_plane",        m_farPlaneSpin->value()},
        {"move_speed",       m_moveSpeedSpin->value()},
        {"mouse_sensitivity",m_mouseSensSpin->value()},
        {"position_x",       0.0},
        {"position_y",       1.0},
        {"position_z",       5.0},
        {"pitch",           -2.0},
        {"yaw",              2.2},
        {"projection_mode",  0},
        {"orthographic_size",10.0}
    };

    j["render_settings"] = {
        {"render_scale",                m_renderScaleSpin->value()},
        {"anisotropy_mode",             m_anisotropyCombo->currentIndex()},
        {"enable_vsync",                m_vsyncCheck->isChecked()},
        {"enable_fxaa",                 m_fxaaCheck->isChecked()},
        {"enable_smaa",                 m_smaaCheck->isChecked()},
        {"enable_taa",                  m_taaCheck->isChecked()},
        {"enable_cmaa",                 false},

        {"enable_ssao",                 m_ssaoCheck->isChecked()},
        {"ssao_radius",                 m_ssaoRadiusSpin->value()},
        {"ssao_bias",                   m_ssaoBiasSpin->value()},
        {"ssao_samples",                m_ssaoSamplesSpin->value()},
        {"ssao_strength",               m_ssaoStrengthSpin->value()},
        {"enable_gtao",                 m_gtaoCheck->isChecked()},
        {"gtao_directions",             m_gtaoDirectionsSpin->value()},
        {"gtao_steps",                  m_gtaoStepsSpin->value()},
        {"use_bent_normals",            m_bentNormalsCheck->isChecked()},

        {"enable_bloom",                m_bloomCheck->isChecked()},
        {"bloom_strength",              m_bloomStrengthSpin->value()},
        {"bloom_threshold",             m_bloomThresholdSpin->value()},
        {"bloom_knee",                  m_bloomKneeSpin->value()},

        {"enable_ssr",                  m_ssrCheck->isChecked()},
        {"ssr_max_distance",            m_ssrMaxDistanceSpin->value()},
        {"ssr_steps",                   m_ssrStepsSpin->value()},
        {"ssr_strength",                m_ssrStrengthSpin->value()},
        {"ssr_thickness",               0.05},
        {"ssr_roughness_cutoff",        m_ssrRoughnessCutoffSpin->value()},

        {"shadow_quality",              m_shadowQualitySpin->value()},
        {"shadow_cascade_count",        m_shadowCascadesSpin->value()},
        {"shadow_max_distance",         m_shadowMaxDistSpin->value()},
        {"shadow_ambient_strength",     0.5},
        {"enable_contact_shadows",      m_contactShadowsCheck->isChecked()},
        {"contact_shadow_length",       0.5},
        {"contact_shadow_steps",        16},
        {"contact_shadow_strength",     0.8},

        {"volumetric_fog_quality",                m_volumetricFogQualityCombo->currentIndex()},
        {"override_volumetric_fog_scene_setting", m_volumetricFogOverrideCheck->isChecked()},
        {"volumetric_fog_override_enabled",       m_volumetricFogOverrideCheck->isChecked()},

        {"enable_post_processing",      m_postProcessingCheck->isChecked()},
        {"enable_color_grading",        m_colorGradingCheck->isChecked()},
        {"color_grading_saturation",    m_cgSaturationSpin->value()},
        {"color_grading_contrast",      m_cgContrastSpin->value()},
        {"color_grading_temperature",   m_cgTemperatureSpin->value()},
        {"color_grading_tint",          m_cgTintSpin->value()},
        {"enable_chromatic_aberration", m_chromAberrationCheck->isChecked()},
        {"chromatic_aberration_strength",m_chromAberrationStrengthSpin->value()},
        {"enable_vignette",             m_vignetteCheck->isChecked()},
        {"vignette_strength",           m_vignetteStrengthSpin->value()},
        {"enable_film_grain",           m_filmGrainCheck->isChecked()},
        {"film_grain_strength",         m_filmGrainStrengthSpin->value()},

        {"enable_ray_tracing",          m_rtxCheck->isChecked()},
        {"enable_rt_shadows",           m_rtShadowsCheck->isChecked()},
        {"enable_rt_reflections",       m_rtReflectionsCheck->isChecked()},
        {"ray_tracing_mode",            m_rtModeCombo->currentIndex() + 1}
    };

    j["version"] = 1;

    return j;
}

void ProjectSettingsDialog::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF bounds = rect().adjusted(1, 1, -1, -1);

    QPainterPath path;
    path.addRoundedRect(bounds, 10, 10);

    QLinearGradient bg(bounds.topLeft(), bounds.bottomLeft());
    bg.setColorAt(0.0, QColor(22, 22, 22, 252));
    bg.setColorAt(1.0, QColor(14, 14, 14, 252));
    painter.fillPath(path, bg);

    painter.setPen(QPen(QColor(60, 60, 60), 1));
    painter.drawPath(path);
}

void ProjectSettingsDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && event->pos().y() < 56)
    {
        m_dragging = true;
        m_dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
    }
    QDialog::mousePressEvent(event);
}

void ProjectSettingsDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging)
        move(event->globalPosition().toPoint() - m_dragOffset);
    QDialog::mouseMoveEvent(event);
}
