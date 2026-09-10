#include "qtui/SettingsPanel.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>

#include <vector>

namespace {

// Engine-truth value lists (gifsicle 1.96 source; see SettingsPanel.h).
const char* kDitherMethods[] = {
    "floyd-steinberg", "atkinson", "o3x3", "o4x4", "o8x8",
    "ro64", "diag45", "halftone", "sqhalftone"};
const char* kResizeMethods[] = {
    "point", "mix", "box", "catrom", "lanczos2", "lanczos3", "mitchell"};
const char* kColorMethods[] = {"diversity", "blend-diversity", "median-cut"};

QGroupBox* makeGroup(const QString& title, const QString& objectName,
                     QFormLayout** formOut) {
  auto* box = new QGroupBox(title);
  box->setObjectName(objectName);
  auto* form = new QFormLayout(box);
  form->setLabelAlignment(Qt::AlignLeft);
  if (formOut) *formOut = form;
  return box;
}

}  // namespace

SettingsPanel::SettingsPanel(QWidget* parent) : QWidget(parent) {
  buildUi();
  connectChanged();
  updateModeDependentUi();
  updateResizeUi();
}

void SettingsPanel::buildUi() {
  auto* outer = new QVBoxLayout(this);
  outer->setContentsMargins(0, 0, 0, 0);

  auto* scroll = new QScrollArea(this);
  scroll->setObjectName(QStringLiteral("actionsScroll"));
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  auto* content = new QWidget(scroll);
  auto* lay = new QVBoxLayout(content);

  // ================= Mode =================
  QFormLayout* form = nullptr;
  auto* modeBox = makeGroup(QStringLiteral("Mode"), QStringLiteral("modeGroup"), &form);
  modeCombo_ = new QComboBox(modeBox);
  modeCombo_->setObjectName(QStringLiteral("modeCombo"));
  modeCombo_->addItem(QStringLiteral("Batch (optimize each file separately)"),
                      static_cast<int>(gs::Mode::Batch));
  modeCombo_->addItem(QStringLiteral("Merge into one animation"),
                      static_cast<int>(gs::Mode::Merge));
  modeCombo_->addItem(QStringLiteral("Explode into frames"),
                      static_cast<int>(gs::Mode::Explode));
  modeCombo_->addItem(QStringLiteral("Auto (engine decides)"),
                      static_cast<int>(gs::Mode::Auto));
  modeCombo_->setCurrentIndex(0);  // Batch default — audit E4, do not change
  modeCombo_->setToolTip(QStringLiteral(
      "Batch optimizes each GIF separately.\n"
      "Merge concatenates all queued GIFs into ONE animation.\n"
      "Explode writes every frame as a separate file."));
  form->addRow(QStringLiteral("Queue mode"), modeCombo_);

  explodeByNameCheck_ = new QCheckBox(QStringLiteral("Name frames after input files (-E)"), modeBox);
  explodeByNameCheck_->setObjectName(QStringLiteral("explodeByNameCheck"));
  form->addRow(QString(), explodeByNameCheck_);
  lay->addWidget(modeBox);

  // ================= Optimize / quantize =================
  auto* optBox = makeGroup(QStringLiteral("Optimize"), QStringLiteral("optimizeGroup"), &form);

  optimizeSpin_ = new QSpinBox(optBox);
  optimizeSpin_->setObjectName(QStringLiteral("optimizeSpin"));
  optimizeSpin_->setRange(0, 3);
  optimizeSpin_->setValue(3);
  optimizeSpin_->setSpecialValueText(QStringLiteral("Off"));
  optimizeSpin_->setToolTip(QStringLiteral(
      "Engine -O level. 0 = off, 1–3 = smaller output (slower).\n"
      "(-O0 is a real setting: 'no optimization' — VP-2.)"));
  form->addRow(QStringLiteral("Optimization level"), optimizeSpin_);

  lossySpin_ = new QSpinBox(optBox);
  lossySpin_->setObjectName(QStringLiteral("lossySpin"));
  lossySpin_->setRange(0, 200);
  lossySpin_->setValue(0);
  lossySpin_->setSpecialValueText(QStringLiteral("Off"));
  lossySpin_->setToolTip(QStringLiteral(
      "--lossy=N: allow small color errors for much smaller files.\n"
      "Typical: 20–80. 0 = lossless."));
  form->addRow(QStringLiteral("Lossy compression"), lossySpin_);

  colorsCheck_ = new QCheckBox(QStringLiteral("Reduce colors"), optBox);
  colorsCheck_->setObjectName(QStringLiteral("colorsCheck"));
  colorsSpin_ = new QSpinBox(optBox);
  colorsSpin_->setObjectName(QStringLiteral("colorsSpin"));
  colorsSpin_->setRange(2, 256);
  colorsSpin_->setValue(128);
  colorsSpin_->setEnabled(false);
  auto* colorsRow = new QHBoxLayout();
  colorsRow->addWidget(colorsCheck_);
  colorsRow->addWidget(colorsSpin_);
  colorsRow->addStretch();
  form->addRow(QStringLiteral("Colormap"), colorsRow);

  ditherCombo_ = new QComboBox(optBox);
  ditherCombo_->setObjectName(QStringLiteral("ditherCombo"));
  ditherCombo_->addItem(QStringLiteral("Off (none)"), QString());
  ditherCombo_->addItem(QStringLiteral("Default (Floyd–Steinberg)"), QStringLiteral("default"));
  for (const char* m : kDitherMethods)
    ditherCombo_->addItem(QString::fromLatin1(m), QString::fromLatin1(m));
  ditherCombo_->setToolTip(QStringLiteral(
      "Dithering smooths color reduction. Methods from gifsicle 1.96\n"
      "set_dither_type(): floyd-steinberg, atkinson, o3x3…ro64, diag45,\n"
      "halftone, sqhalftone. 'Default' emits bare -f."));
  form->addRow(QStringLiteral("Dither method"), ditherCombo_);

  colorMethodCombo_ = new QComboBox(optBox);
  colorMethodCombo_->setObjectName(QStringLiteral("colorMethodCombo"));
  colorMethodCombo_->addItem(QStringLiteral("Engine default"), QString());
  for (const char* m : kColorMethods)
    colorMethodCombo_->addItem(QString::fromLatin1(m), QString::fromLatin1(m));
  form->addRow(QStringLiteral("Color method"), colorMethodCombo_);

  carefulCheck_ = new QCheckBox(QStringLiteral("Careful (slightly larger, safer output)"), optBox);
  carefulCheck_->setObjectName(QStringLiteral("carefulCheck"));
  carefulCheck_->setToolTip(QStringLiteral(
      "--careful: avoids some common software compatibility issues."));
  form->addRow(QString(), carefulCheck_);
  lay->addWidget(optBox);

  // ================= Resize / scale =================
  auto* resizeBox = makeGroup(QStringLiteral("Resize / Scale"), QStringLiteral("resizeGroup"), &form);

  resizeKindCombo_ = new QComboBox(resizeBox);
  resizeKindCombo_->setObjectName(QStringLiteral("resizeKindCombo"));
  resizeKindCombo_->addItem(QStringLiteral("No resize"), static_cast<int>(gs::ResizeKind::None));
  resizeKindCombo_->addItem(QStringLiteral("Fit inside W×H (keep aspect)"), static_cast<int>(gs::ResizeKind::Fit));
  resizeKindCombo_->addItem(QStringLiteral("Touch W×H (only enlarge/shrink to fit)"), static_cast<int>(gs::ResizeKind::Touch));
  resizeKindCombo_->addItem(QStringLiteral("Exact W×H (may distort)"), static_cast<int>(gs::ResizeKind::Exact));
  resizeKindCombo_->addItem(QStringLiteral("Scale by %"), static_cast<int>(gs::ResizeKind::Scale));
  resizeKindCombo_->addItem(QStringLiteral("Width only"), static_cast<int>(gs::ResizeKind::Width));
  resizeKindCombo_->addItem(QStringLiteral("Height only"), static_cast<int>(gs::ResizeKind::Height));
  form->addRow(QStringLiteral("Resize"), resizeKindCombo_);

  resizeWSpin_ = new QSpinBox(resizeBox);
  resizeWSpin_->setObjectName(QStringLiteral("resizeWSpin"));
  resizeWSpin_->setRange(1, 65535);
  resizeWSpin_->setValue(320);
  resizeHSpin_ = new QSpinBox(resizeBox);
  resizeHSpin_->setObjectName(QStringLiteral("resizeHSpin"));
  resizeHSpin_->setRange(1, 65535);
  resizeHSpin_->setValue(240);
  auto* whRow = new QHBoxLayout();
  whRow->addWidget(new QLabel(QStringLiteral("W"), resizeBox));
  whRow->addWidget(resizeWSpin_);
  whRow->addWidget(new QLabel(QStringLiteral("×  H"), resizeBox));
  whRow->addWidget(resizeHSpin_);
  whRow->addStretch();
  form->addRow(QStringLiteral("Size (px)"), whRow);

  scaleXSpin_ = new QDoubleSpinBox(resizeBox);
  scaleXSpin_->setObjectName(QStringLiteral("scaleXSpin"));
  scaleXSpin_->setRange(1.0, 1000.0);
  scaleXSpin_->setDecimals(1);
  scaleXSpin_->setSuffix(QStringLiteral(" %"));
  scaleXSpin_->setValue(100.0);
  scaleYSpin_ = new QDoubleSpinBox(resizeBox);
  scaleYSpin_->setObjectName(QStringLiteral("scaleYSpin"));
  scaleYSpin_->setRange(1.0, 1000.0);
  scaleYSpin_->setDecimals(1);
  scaleYSpin_->setSuffix(QStringLiteral(" %"));
  scaleYSpin_->setValue(100.0);
  auto* scaleRow = new QHBoxLayout();
  scaleRow->addWidget(scaleXSpin_);
  scaleRow->addWidget(scaleYSpin_);
  scaleRow->addStretch();
  form->addRow(QStringLiteral("Scale X / Y"), scaleRow);

  resizeMethodCombo_ = new QComboBox(resizeBox);
  resizeMethodCombo_->setObjectName(QStringLiteral("resizeMethodCombo"));
  resizeMethodCombo_->addItem(QStringLiteral("Engine default"), QString());
  for (const char* m : kResizeMethods)
    resizeMethodCombo_->addItem(QString::fromLatin1(m), QString::fromLatin1(m));
  resizeMethodCombo_->setToolTip(QStringLiteral(
      "Resampling algorithm (--resize-method). Values from the engine's\n"
      "RESIZE_METHOD_TYPE list: point, mix, box, catrom, lanczos2,\n"
      "lanczos3, mitchell."));
  form->addRow(QStringLiteral("Method"), resizeMethodCombo_);
  lay->addWidget(resizeBox);

  // ================= Geometry =================
  auto* geoBox = makeGroup(QStringLiteral("Geometry"), QStringLiteral("geometryGroup"), &form);

  rotateCombo_ = new QComboBox(geoBox);
  rotateCombo_->setObjectName(QStringLiteral("rotateCombo"));
  rotateCombo_->addItem(QStringLiteral("No rotation"), static_cast<int>(gs::Rotation::None));
  rotateCombo_->addItem(QStringLiteral("Rotate 90° clockwise"), static_cast<int>(gs::Rotation::R90));
  rotateCombo_->addItem(QStringLiteral("Rotate 180°"), static_cast<int>(gs::Rotation::R180));
  rotateCombo_->addItem(QStringLiteral("Rotate 270° clockwise"), static_cast<int>(gs::Rotation::R270));
  form->addRow(QStringLiteral("Rotate"), rotateCombo_);

  flipHCheck_ = new QCheckBox(QStringLiteral("Flip horizontal"), geoBox);
  flipHCheck_->setObjectName(QStringLiteral("flipHCheck"));
  flipVCheck_ = new QCheckBox(QStringLiteral("Flip vertical"), geoBox);
  flipVCheck_->setObjectName(QStringLiteral("flipVCheck"));
  auto* flipRow = new QHBoxLayout();
  flipRow->addWidget(flipHCheck_);
  flipRow->addWidget(flipVCheck_);
  flipRow->addStretch();
  form->addRow(QStringLiteral("Flip"), flipRow);

  positionCheck_ = new QCheckBox(QStringLiteral("Set frame position"), geoBox);
  positionCheck_->setObjectName(QStringLiteral("positionCheck"));
  posXSpin_ = new QSpinBox(geoBox);
  posXSpin_->setObjectName(QStringLiteral("posXSpin"));
  posXSpin_->setRange(0, 65535);
  posYSpin_ = new QSpinBox(geoBox);
  posYSpin_->setObjectName(QStringLiteral("posYSpin"));
  posYSpin_->setRange(0, 65535);
  auto* posRow = new QHBoxLayout();
  posRow->addWidget(positionCheck_);
  posRow->addWidget(new QLabel(QStringLiteral("X"), geoBox));
  posRow->addWidget(posXSpin_);
  posRow->addWidget(new QLabel(QStringLiteral("Y"), geoBox));
  posRow->addWidget(posYSpin_);
  posRow->addStretch();
  posXSpin_->setEnabled(false);
  posYSpin_->setEnabled(false);
  form->addRow(QStringLiteral("Position"), posRow);

  interlaceCheck_ = new QCheckBox(QStringLiteral("Interlace (progressive loading)"), geoBox);
  interlaceCheck_->setObjectName(QStringLiteral("interlaceCheck"));
  form->addRow(QString(), interlaceCheck_);
  lay->addWidget(geoBox);

  // ================= Crop =================
  auto* cropBox = makeGroup(QStringLiteral("Crop"), QStringLiteral("cropGroup"), &form);
  cropCheck_ = new QCheckBox(QStringLiteral("Crop to rectangle"), cropBox);
  cropCheck_->setObjectName(QStringLiteral("cropCheck"));
  form->addRow(QString(), cropCheck_);

  cropXSpin_ = new QSpinBox(cropBox);
  cropXSpin_->setObjectName(QStringLiteral("cropXSpin"));
  cropXSpin_->setRange(0, 65535);
  cropYSpin_ = new QSpinBox(cropBox);
  cropYSpin_->setObjectName(QStringLiteral("cropYSpin"));
  cropYSpin_->setRange(0, 65535);
  cropWSpin_ = new QSpinBox(cropBox);
  cropWSpin_->setObjectName(QStringLiteral("cropWSpin"));
  cropWSpin_->setRange(0, 65535);
  cropWSpin_->setValue(32);
  cropHSpin_ = new QSpinBox(cropBox);
  cropHSpin_->setObjectName(QStringLiteral("cropHSpin"));
  cropHSpin_->setRange(0, 65535);
  cropHSpin_->setValue(32);
  auto* cropRow = new QHBoxLayout();
  cropRow->addWidget(new QLabel(QStringLiteral("X"), cropBox));
  cropRow->addWidget(cropXSpin_);
  cropRow->addWidget(new QLabel(QStringLiteral("Y"), cropBox));
  cropRow->addWidget(cropYSpin_);
  cropRow->addWidget(new QLabel(QStringLiteral("W"), cropBox));
  cropRow->addWidget(cropWSpin_);
  cropRow->addWidget(new QLabel(QStringLiteral("H"), cropBox));
  cropRow->addWidget(cropHSpin_);
  cropRow->addStretch();
  form->addRow(QStringLiteral("Rectangle"), cropRow);
  auto* cropHint = new QLabel(QStringLiteral("Emitted as --crop X,Y+WxH (engine plus-form)."), cropBox);
  cropHint->setObjectName(QStringLiteral("cropHintLabel"));
  cropHint->setWordWrap(true);
  form->addRow(QString(), cropHint);

  cropTransparencyCheck_ = new QCheckBox(QStringLiteral("Crop transparent edges afterwards"), cropBox);
  cropTransparencyCheck_->setObjectName(QStringLiteral("cropTransparencyCheck"));
  form->addRow(QString(), cropTransparencyCheck_);
  for (QSpinBox* s : {cropXSpin_, cropYSpin_, cropWSpin_, cropHSpin_}) s->setEnabled(false);
  cropTransparencyCheck_->setEnabled(false);
  lay->addWidget(cropBox);

  // ================= Animation =================
  auto* animBox = makeGroup(QStringLiteral("Animation"), QStringLiteral("animationGroup"), &form);

  delayCheck_ = new QCheckBox(QStringLiteral("Set frame delay"), animBox);
  delayCheck_->setObjectName(QStringLiteral("delayCheck"));
  delaySpin_ = new QSpinBox(animBox);
  delaySpin_->setObjectName(QStringLiteral("delaySpin"));
  delaySpin_->setRange(0, 65535);
  delaySpin_->setValue(10);
  delaySpin_->setEnabled(false);
  auto* delayRow = new QHBoxLayout();
  delayRow->addWidget(delayCheck_);
  delayRow->addWidget(delaySpin_);
  delayRow->addStretch();
  // E7 guard: the unit label must say 1/100 s — never "ms".
  auto* delayLabel = new QLabel(QStringLiteral("Frame delay (1/100 s)"), animBox);
  delayLabel->setObjectName(QStringLiteral("delayLabel"));
  form->addRow(delayLabel, delayRow);

  loopCombo_ = new QComboBox(animBox);
  loopCombo_->setObjectName(QStringLiteral("loopCombo"));
  loopCombo_->addItem(QStringLiteral("Keep original"), 0);
  loopCombo_->addItem(QStringLiteral("Loop forever"), 1);
  loopCombo_->addItem(QStringLiteral("Loop N times…"), 2);
  form->addRow(QStringLiteral("Looping"), loopCombo_);

  loopSpin_ = new QSpinBox(animBox);
  loopSpin_->setObjectName(QStringLiteral("loopSpin"));
  loopSpin_->setRange(1, 65535);
  loopSpin_->setValue(3);
  loopSpin_->setEnabled(false);
  form->addRow(QStringLiteral("Loop count"), loopSpin_);

  disposalCombo_ = new QComboBox(animBox);
  disposalCombo_->setObjectName(QStringLiteral("disposalCombo"));
  disposalCombo_->addItem(QStringLiteral("Keep original"), -1);
  disposalCombo_->addItem(QStringLiteral("none (0) — unspecified"), 0);
  disposalCombo_->addItem(QStringLiteral("asis (1) — do not dispose"), 1);
  disposalCombo_->addItem(QStringLiteral("background (2) — restore to background"), 2);
  disposalCombo_->addItem(QStringLiteral("previous (3) — restore to previous"), 3);
  disposalCombo_->setToolTip(QStringLiteral(
      "Frame disposal method (--disposal). Names/values from the engine's\n"
      "DISPOSAL_TYPE parser."));
  form->addRow(QStringLiteral("Disposal"), disposalCombo_);

  unoptimizeCheck_ = new QCheckBox(QStringLiteral("Unoptimize (expand to full frames, -U)"), animBox);
  unoptimizeCheck_->setObjectName(QStringLiteral("unoptimizeCheck"));
  form->addRow(QString(), unoptimizeCheck_);

  threadsSpin_ = new QSpinBox(animBox);
  threadsSpin_->setObjectName(QStringLiteral("threadsSpin"));
  threadsSpin_->setRange(0, 64);
  threadsSpin_->setValue(0);
  threadsSpin_->setSpecialValueText(QStringLiteral("Auto"));
  form->addRow(QStringLiteral("Threads"), threadsSpin_);
  lay->addWidget(animBox);

  // ================= Colors / gamma / transparency =================
  auto* colorBox = makeGroup(QStringLiteral("Colors / Transparency"), QStringLiteral("colorGroup"), &form);

  gammaCombo_ = new QComboBox(colorBox);
  gammaCombo_->setObjectName(QStringLiteral("gammaCombo"));
  gammaCombo_->addItem(QStringLiteral("Keep original"), 0);
  gammaCombo_->addItem(QStringLiteral("sRGB"), 1);
  gammaCombo_->addItem(QStringLiteral("Oklab"), 2);
  gammaCombo_->addItem(QStringLiteral("Custom value…"), 3);
  gammaCombo_->setToolTip(QStringLiteral(
      "Color math for quantization (--gamma). Engine accepts srgb, oklab\n"
      "(when built with cbrtf) or a numeric gamma. VP-3: nothing is emitted\n"
      "unless you choose a value here."));
  form->addRow(QStringLiteral("Gamma"), gammaCombo_);

  gammaEdit_ = new QLineEdit(colorBox);
  gammaEdit_->setObjectName(QStringLiteral("gammaEdit"));
  gammaEdit_->setPlaceholderText(QStringLiteral("e.g. 2.2"));
  gammaEdit_->setEnabled(false);
  form->addRow(QStringLiteral("Gamma value"), gammaEdit_);

  auto makeColorRow = [colorBox, this](const QString& name, QLineEdit** editOut) {
    auto* edit = new QLineEdit(colorBox);
    edit->setObjectName(name);
    edit->setPlaceholderText(QStringLiteral("#rrggbb or color name (empty = unchanged)"));
    auto* pick = new QPushButton(QStringLiteral("Pick…"), colorBox);
    pick->setObjectName(name + QStringLiteral("Pick"));
    QObject::connect(pick, &QPushButton::clicked, this, [this, edit]() {
      const QString c = pickColor(edit->text().trimmed());
      if (!c.isEmpty()) {
        edit->setText(c);
        emit changed();
      }
    });
    auto* row = new QHBoxLayout();
    row->addWidget(edit);
    row->addWidget(pick);
    *editOut = edit;
    return row;
  };
  form->addRow(QStringLiteral("Background"), makeColorRow(QStringLiteral("backgroundEdit"), &backgroundEdit_));
  form->addRow(QStringLiteral("Transparent"), makeColorRow(QStringLiteral("transparentEdit"), &transparentEdit_));
  lay->addWidget(colorBox);

  // ================= Metadata =================
  auto* metaBox = makeGroup(QStringLiteral("Metadata"), QStringLiteral("metadataGroup"), &form);
  removeCommentsCheck_ = new QCheckBox(QStringLiteral("Remove comments (--no-comments)"), metaBox);
  removeCommentsCheck_->setObjectName(QStringLiteral("removeCommentsCheck"));
  removeNamesCheck_ = new QCheckBox(QStringLiteral("Remove frame names (--no-names)"), metaBox);
  removeNamesCheck_->setObjectName(QStringLiteral("removeNamesCheck"));
  removeExtensionsCheck_ = new QCheckBox(QStringLiteral("Remove unknown extensions (--no-extensions)"), metaBox);
  removeExtensionsCheck_->setObjectName(QStringLiteral("removeExtensionsCheck"));
  form->addRow(QString(), removeCommentsCheck_);
  form->addRow(QString(), removeNamesCheck_);
  form->addRow(QString(), removeExtensionsCheck_);

  commentEdit_ = new QLineEdit(metaBox);
  commentEdit_->setObjectName(QStringLiteral("commentEdit"));
  commentEdit_->setPlaceholderText(QStringLiteral("Comment text to add…"));
  auto* addComment = new QPushButton(QStringLiteral("Add"), metaBox);
  addComment->setObjectName(QStringLiteral("addCommentButton"));
  auto* removeComment = new QPushButton(QStringLiteral("Remove"), metaBox);
  removeComment->setObjectName(QStringLiteral("removeCommentButton"));
  commentList_ = new QListWidget(metaBox);
  commentList_->setObjectName(QStringLiteral("commentList"));
  commentList_->setMaximumHeight(72);
  auto* commentRow = new QHBoxLayout();
  commentRow->addWidget(commentEdit_);
  commentRow->addWidget(addComment);
  commentRow->addWidget(removeComment);
  form->addRow(QStringLiteral("Add comment"), commentRow);
  form->addRow(QStringLiteral("Comments"), commentList_);
  connect(addComment, &QPushButton::clicked, this, [this]() {
    const QString t = commentEdit_->text();
    if (!t.isEmpty()) {
      commentList_->addItem(t);
      commentEdit_->clear();
      emit changed();
    }
  });
  connect(removeComment, &QPushButton::clicked, this, [this]() {
    for (QListWidgetItem* item : commentList_->selectedItems()) delete item;
    emit changed();
  });
  lay->addWidget(metaBox);

  lay->addStretch();
  scroll->setWidget(content);
  outer->addWidget(scroll);
}

QString SettingsPanel::pickColor(const QString& current) {
  QColor initial = QColor::isValidColorName(current) ? QColor(current) : Qt::white;
  const QColor c = QColorDialog::getColor(initial, this, QStringLiteral("Choose color"));
  if (!c.isValid()) return {};
  return c.name(QColor::HexRgb);  // #rrggbb — gifsicle color syntax
}

void SettingsPanel::connectChanged() {
  connect(modeCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
    updateModeDependentUi();
    emit changed();
  });
  connect(explodeByNameCheck_, &QCheckBox::toggled, this, &SettingsPanel::changed);
  connect(optimizeSpin_, qOverload<int>(&QSpinBox::valueChanged), this, &SettingsPanel::changed);
  connect(lossySpin_, qOverload<int>(&QSpinBox::valueChanged), this, &SettingsPanel::changed);
  connect(colorsCheck_, &QCheckBox::toggled, this, [this](bool on) {
    colorsSpin_->setEnabled(on);
    emit changed();
  });
  connect(colorsSpin_, qOverload<int>(&QSpinBox::valueChanged), this, &SettingsPanel::changed);
  connect(ditherCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &SettingsPanel::changed);
  connect(colorMethodCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &SettingsPanel::changed);
  connect(carefulCheck_, &QCheckBox::toggled, this, &SettingsPanel::changed);
  connect(resizeKindCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
    updateResizeUi();
    emit changed();
  });
  connect(resizeWSpin_, qOverload<int>(&QSpinBox::valueChanged), this, &SettingsPanel::changed);
  connect(resizeHSpin_, qOverload<int>(&QSpinBox::valueChanged), this, &SettingsPanel::changed);
  connect(scaleXSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &SettingsPanel::changed);
  connect(scaleYSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &SettingsPanel::changed);
  connect(resizeMethodCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &SettingsPanel::changed);
  connect(rotateCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &SettingsPanel::changed);
  connect(flipHCheck_, &QCheckBox::toggled, this, &SettingsPanel::changed);
  connect(flipVCheck_, &QCheckBox::toggled, this, &SettingsPanel::changed);
  connect(interlaceCheck_, &QCheckBox::toggled, this, &SettingsPanel::changed);
  connect(positionCheck_, &QCheckBox::toggled, this, [this](bool on) {
    posXSpin_->setEnabled(on);
    posYSpin_->setEnabled(on);
    emit changed();
  });
  connect(posXSpin_, qOverload<int>(&QSpinBox::valueChanged), this, &SettingsPanel::changed);
  connect(posYSpin_, qOverload<int>(&QSpinBox::valueChanged), this, &SettingsPanel::changed);
  connect(cropCheck_, &QCheckBox::toggled, this, [this](bool on) {
    for (QSpinBox* s : {cropXSpin_, cropYSpin_, cropWSpin_, cropHSpin_}) s->setEnabled(on);
    cropTransparencyCheck_->setEnabled(on);
    emit changed();
  });
  connect(cropXSpin_, qOverload<int>(&QSpinBox::valueChanged), this, &SettingsPanel::changed);
  connect(cropYSpin_, qOverload<int>(&QSpinBox::valueChanged), this, &SettingsPanel::changed);
  connect(cropWSpin_, qOverload<int>(&QSpinBox::valueChanged), this, &SettingsPanel::changed);
  connect(cropHSpin_, qOverload<int>(&QSpinBox::valueChanged), this, &SettingsPanel::changed);
  connect(cropTransparencyCheck_, &QCheckBox::toggled, this, &SettingsPanel::changed);
  connect(delayCheck_, &QCheckBox::toggled, this, [this](bool on) {
    delaySpin_->setEnabled(on);
    emit changed();
  });
  connect(delaySpin_, qOverload<int>(&QSpinBox::valueChanged), this, &SettingsPanel::changed);
  connect(loopCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
    loopSpin_->setEnabled(loopCombo_->currentData().toInt() == 2);
    emit changed();
  });
  connect(loopSpin_, qOverload<int>(&QSpinBox::valueChanged), this, &SettingsPanel::changed);
  connect(disposalCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &SettingsPanel::changed);
  connect(unoptimizeCheck_, &QCheckBox::toggled, this, &SettingsPanel::changed);
  connect(threadsSpin_, qOverload<int>(&QSpinBox::valueChanged), this, &SettingsPanel::changed);
  connect(gammaCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
    gammaEdit_->setEnabled(gammaCombo_->currentData().toInt() == 3);
    emit changed();
  });
  connect(gammaEdit_, &QLineEdit::textChanged, this, &SettingsPanel::changed);
  connect(backgroundEdit_, &QLineEdit::textChanged, this, &SettingsPanel::changed);
  connect(transparentEdit_, &QLineEdit::textChanged, this, &SettingsPanel::changed);
  connect(removeCommentsCheck_, &QCheckBox::toggled, this, &SettingsPanel::changed);
  connect(removeNamesCheck_, &QCheckBox::toggled, this, &SettingsPanel::changed);
  connect(removeExtensionsCheck_, &QCheckBox::toggled, this, &SettingsPanel::changed);
}

void SettingsPanel::updateModeDependentUi() {
  const bool explode = mode() == gs::Mode::Explode;
  explodeByNameCheck_->setEnabled(explode);
}

void SettingsPanel::updateResizeUi() {
  const auto kind = static_cast<gs::ResizeKind>(resizeKindCombo_->currentData().toInt());
  const bool sizeUsed = kind == gs::ResizeKind::Fit || kind == gs::ResizeKind::Touch ||
                        kind == gs::ResizeKind::Exact || kind == gs::ResizeKind::Width ||
                        kind == gs::ResizeKind::Height;
  resizeWSpin_->setEnabled(sizeUsed);
  resizeHSpin_->setEnabled(sizeUsed);
  const bool scaleUsed = kind == gs::ResizeKind::Scale;
  scaleXSpin_->setEnabled(scaleUsed);
  scaleYSpin_->setEnabled(scaleUsed);
}

gs::Mode SettingsPanel::mode() const {
  return static_cast<gs::Mode>(modeCombo_->currentData().toInt());
}

void SettingsPanel::writeInto(gs::Settings& s) const {
  s.mode = mode();
  s.explode_by_name = explodeByNameCheck_->isChecked();

  s.optimize_level = optimizeSpin_->value();                 // -O0..-O3 (VP-2)
  s.lossy = lossySpin_->value() > 0 ? lossySpin_->value() : -1;
  s.color_count = colorsCheck_->isChecked() ? colorsSpin_->value() : -1;
  const QString ditherData = ditherCombo_->currentData().toString();
  if (ditherData.isEmpty()) {
    s.dither = false;
    s.dither_method.clear();
  } else if (ditherData == QLatin1String("default")) {
    s.dither = true;        // bare -f
    s.dither_method.clear();
  } else {
    s.dither = true;
    s.dither_method = ditherData.toStdString();
  }
  s.color_method = colorMethodCombo_->currentData().toString().toStdString();
  s.careful = carefulCheck_->isChecked();

  s.resize_kind = static_cast<gs::ResizeKind>(resizeKindCombo_->currentData().toInt());
  s.resize_w = static_cast<unsigned>(resizeWSpin_->value());
  s.resize_h = static_cast<unsigned>(resizeHSpin_->value());
  s.scale_x = scaleXSpin_->value() / 100.0;
  s.scale_y = scaleYSpin_->value() / 100.0;
  s.resize_method = resizeMethodCombo_->currentData().toString().toStdString();

  s.rotation = static_cast<gs::Rotation>(rotateCombo_->currentData().toInt());
  s.flip_horizontal = flipHCheck_->isChecked();
  s.flip_vertical = flipVCheck_->isChecked();
  s.interlace = interlaceCheck_->isChecked();
  s.has_position = positionCheck_->isChecked();
  s.position_x = static_cast<unsigned>(posXSpin_->value());
  s.position_y = static_cast<unsigned>(posYSpin_->value());

  s.crop = cropCheck_->isChecked();
  s.crop_x = static_cast<unsigned>(cropXSpin_->value());
  s.crop_y = static_cast<unsigned>(cropYSpin_->value());
  s.crop_w = static_cast<unsigned>(cropWSpin_->value());
  s.crop_h = static_cast<unsigned>(cropHSpin_->value());
  s.crop_transparency = cropTransparencyCheck_->isChecked();

  s.delay_cs = delayCheck_->isChecked() ? delaySpin_->value() : -1;  // 1/100 s (E7)
  switch (loopCombo_->currentData().toInt()) {
    case 1:  s.loopcount = 0; break;                 // forever (VP-1)
    case 2:  s.loopcount = loopSpin_->value(); break;
    default: s.loopcount = -1; break;                // unchanged
  }
  s.disposal = disposalCombo_->currentData().toInt();
  s.unoptimize = unoptimizeCheck_->isChecked();
  s.threads = threadsSpin_->value();

  switch (gammaCombo_->currentData().toInt()) {
    case 1: s.gamma_str = "srgb"; break;
    case 2: s.gamma_str = "oklab"; break;
    case 3: s.gamma_str = gammaEdit_->text().trimmed().toStdString(); break;
    default: s.gamma_str.clear(); break;
  }
  s.background = backgroundEdit_->text().trimmed().toStdString();
  s.transparent = transparentEdit_->text().trimmed().toStdString();

  s.remove_comments = removeCommentsCheck_->isChecked();
  s.remove_names = removeNamesCheck_->isChecked();
  s.remove_extensions = removeExtensionsCheck_->isChecked();
  s.comments.clear();
  for (int i = 0; i < commentList_->count(); ++i)
    s.comments.push_back(commentList_->item(i)->text().toStdString());
}

void SettingsPanel::readFrom(const gs::Settings& s) {
  // Block every control's signals while restoring: no per-widget changed()
  // storm and no toggled-lambda side effects. Enabled-state sync is done
  // explicitly at the end instead.
  const std::vector<QWidget*> controls = {
      modeCombo_, explodeByNameCheck_, optimizeSpin_, lossySpin_, colorsCheck_,
      colorsSpin_, ditherCombo_, colorMethodCombo_, carefulCheck_,
      resizeKindCombo_, resizeWSpin_, resizeHSpin_, scaleXSpin_, scaleYSpin_,
      resizeMethodCombo_, rotateCombo_, flipHCheck_, flipVCheck_,
      interlaceCheck_, positionCheck_, posXSpin_, posYSpin_, cropCheck_,
      cropXSpin_, cropYSpin_, cropWSpin_, cropHSpin_, cropTransparencyCheck_,
      delayCheck_, delaySpin_, loopCombo_, loopSpin_, disposalCombo_,
      unoptimizeCheck_, threadsSpin_, gammaCombo_, gammaEdit_, backgroundEdit_,
      transparentEdit_, removeCommentsCheck_, removeNamesCheck_,
      removeExtensionsCheck_, commentList_};
  std::vector<QSignalBlocker> blockers;
  blockers.reserve(controls.size());
  for (QWidget* w : controls) blockers.emplace_back(w);

  // Helpers: combo lookup by itemData; spin clamp into widget range.
  const auto selectByData = [](QComboBox* combo, const QVariant& data) {
    const int idx = combo->findData(data);
    if (idx >= 0) combo->setCurrentIndex(idx);
  };
  const auto setSpin = [](QSpinBox* spin, int v) {
    spin->setValue(qBound(spin->minimum(), v, spin->maximum()));
  };

  // ---- Mode ----
  selectByData(modeCombo_, static_cast<int>(s.mode));
  explodeByNameCheck_->setChecked(s.explode_by_name);

  // ---- Optimize / quantize ----
  // optimize_level -1 ("no -O flag") is not GUI-representable; only apply
  // values the panel could have produced (0..3). GUI saves always contain it.
  if (s.optimize_level >= 0 && s.optimize_level <= 3)
    setSpin(optimizeSpin_, s.optimize_level);
  // lossy: GUI maps spin 0 -> -1 on write; map -1 back to 0 (Off).
  lossySpin_->setValue(s.lossy >= 0 ? qBound(0, s.lossy, 200) : 0);
  if (s.color_count >= 2 && s.color_count <= 256) {
    colorsCheck_->setChecked(true);
    setSpin(colorsSpin_, s.color_count);
  } else {
    colorsCheck_->setChecked(false);
  }
  if (!s.dither_method.empty()) {
    selectByData(ditherCombo_, QString::fromStdString(s.dither_method));
    if (ditherCombo_->currentData().toString() !=
        QString::fromStdString(s.dither_method))
      ditherCombo_->setCurrentIndex(0);  // unknown method -> honest Off
  } else {
    ditherCombo_->setCurrentIndex(s.dither ? 1 : 0);  // 1 = "Default" (bare -f)
  }
  if (!s.color_method.empty()) {
    selectByData(colorMethodCombo_, QString::fromStdString(s.color_method));
    if (colorMethodCombo_->currentData().toString() !=
        QString::fromStdString(s.color_method))
      colorMethodCombo_->setCurrentIndex(0);
  } else {
    colorMethodCombo_->setCurrentIndex(0);
  }
  carefulCheck_->setChecked(s.careful);

  // ---- Resize / scale ----
  selectByData(resizeKindCombo_, static_cast<int>(s.resize_kind));
  if (s.resize_w >= 1) setSpin(resizeWSpin_, static_cast<int>(s.resize_w));
  if (s.resize_h >= 1) setSpin(resizeHSpin_, static_cast<int>(s.resize_h));
  if (s.scale_x > 0.0)
    scaleXSpin_->setValue(qBound(scaleXSpin_->minimum(), s.scale_x * 100.0,
                                 scaleXSpin_->maximum()));
  if (s.scale_y > 0.0)
    scaleYSpin_->setValue(qBound(scaleYSpin_->minimum(), s.scale_y * 100.0,
                                 scaleYSpin_->maximum()));
  if (!s.resize_method.empty()) {
    selectByData(resizeMethodCombo_, QString::fromStdString(s.resize_method));
    if (resizeMethodCombo_->currentData().toString() !=
        QString::fromStdString(s.resize_method))
      resizeMethodCombo_->setCurrentIndex(0);
  } else {
    resizeMethodCombo_->setCurrentIndex(0);
  }

  // ---- Geometry ----
  selectByData(rotateCombo_, static_cast<int>(s.rotation));
  flipHCheck_->setChecked(s.flip_horizontal);
  flipVCheck_->setChecked(s.flip_vertical);
  interlaceCheck_->setChecked(s.interlace);
  positionCheck_->setChecked(s.has_position);
  setSpin(posXSpin_, static_cast<int>(s.position_x));
  setSpin(posYSpin_, static_cast<int>(s.position_y));

  // ---- Crop ----
  cropCheck_->setChecked(s.crop);
  setSpin(cropXSpin_, static_cast<int>(s.crop_x));
  setSpin(cropYSpin_, static_cast<int>(s.crop_y));
  setSpin(cropWSpin_, static_cast<int>(s.crop_w));
  setSpin(cropHSpin_, static_cast<int>(s.crop_h));
  cropTransparencyCheck_->setChecked(s.crop_transparency);

  // ---- Animation ----
  delayCheck_->setChecked(s.delay_cs >= 0);
  if (s.delay_cs >= 0) setSpin(delaySpin_, s.delay_cs);
  if (s.loopcount == 0) {
    selectByData(loopCombo_, 1);  // forever (VP-1: emits --loopcount=0)
  } else if (s.loopcount > 0) {
    selectByData(loopCombo_, 2);  // loop N times
    setSpin(loopSpin_, s.loopcount);
  } else {
    selectByData(loopCombo_, 0);  // keep original
  }
  selectByData(disposalCombo_, s.disposal);  // -1..3; 4..7 not GUI-representable
  unoptimizeCheck_->setChecked(s.unoptimize);
  if (s.threads >= 0 && s.threads <= 64) setSpin(threadsSpin_, s.threads);

  // ---- Colors / gamma / transparency ----
  // gamma_str is authoritative (load_settings always fills it from the file);
  // the legacy numeric gamma only applies when no string form is present.
  if (!s.gamma_str.empty()) {
    const QString g = QString::fromStdString(s.gamma_str);
    if (g == QLatin1String("srgb")) selectByData(gammaCombo_, 1);
    else if (g == QLatin1String("oklab")) selectByData(gammaCombo_, 2);
    else { selectByData(gammaCombo_, 3); gammaEdit_->setText(g); }
  } else if (s.gamma >= 0) {
    selectByData(gammaCombo_, 3);
    gammaEdit_->setText(QString::number(s.gamma));
  } else {
    selectByData(gammaCombo_, 0);  // keep original
  }
  backgroundEdit_->setText(QString::fromStdString(s.background));
  transparentEdit_->setText(QString::fromStdString(s.transparent));

  // ---- Metadata ----
  removeCommentsCheck_->setChecked(s.remove_comments);
  removeNamesCheck_->setChecked(s.remove_names);
  removeExtensionsCheck_->setChecked(s.remove_extensions);
  commentList_->clear();
  for (const auto& c : s.comments)
    commentList_->addItem(QString::fromStdString(c));

  // ---- Enabled-state sync (the blocked toggled-lambdas would have done this) ----
  updateModeDependentUi();
  updateResizeUi();
  colorsSpin_->setEnabled(colorsCheck_->isChecked());
  posXSpin_->setEnabled(positionCheck_->isChecked());
  posYSpin_->setEnabled(positionCheck_->isChecked());
  for (QSpinBox* sp : {cropXSpin_, cropYSpin_, cropWSpin_, cropHSpin_})
    sp->setEnabled(cropCheck_->isChecked());
  cropTransparencyCheck_->setEnabled(cropCheck_->isChecked());
  delaySpin_->setEnabled(delayCheck_->isChecked());
  loopSpin_->setEnabled(loopCombo_->currentData().toInt() == 2);
  gammaEdit_->setEnabled(gammaCombo_->currentData().toInt() == 3);
}
