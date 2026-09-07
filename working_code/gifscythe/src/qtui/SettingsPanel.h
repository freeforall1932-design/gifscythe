// SettingsPanel.h - "Actions" tab content: every GifsicleSettings control
// the engine supports for whole-GIF work, organized in groups (XNConvert
// feel). Pure Qt Widgets; talks to the core via writeInto(gs::Settings&).
//
// Ground truth for value lists (gifsicle 1.96 source, verified 2026-09-07):
//   dither methods   : quantize.c set_dither_type()
//   resize methods   : gifsicle.c RESIZE_METHOD_TYPE string list
//   disposal         : gifsicle.c DISPOSAL_TYPE (none/asis/background/previous)
//   color methods    : gifsicle.c COLORMAP_ALG_TYPE (diversity/blend-diversity/median-cut)
//   gamma            : gifsicle.c GAMMA_OPT (srgb | oklab | numeric)
//
// Every widget has a stable objectName — the offscreen harness
// (tests/test_gui_offscreen.cpp) finds controls by name, not by type.
//
// Invariants (COMPILED_AUDIT):
//   * Mode default stays **Batch** (E4).
//   * Delay is labeled in 1/100 s, never "ms" (E7).
//   * Crop emitter stays X,Y+WxH plus-form (VP-5) — core handles it.
//   * --loopcount=0 = forever (VP-1); -O0 = off (VP-2) — unchanged.

#ifndef GIFSCYTHE_SETTINGSPANEL_H
#define GIFSCYTHE_SETTINGSPANEL_H

#include <QWidget>

#include "core/GifsicleSettings.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;

class SettingsPanel : public QWidget {
  Q_OBJECT
 public:
  explicit SettingsPanel(QWidget* parent = nullptr);

  // Write every control into s (mode, optimize, resize, crop, animation,
  // colors, metadata...). Inputs/output are NOT touched (MainWindow owns them).
  void writeInto(gs::Settings& s) const;

  gs::Mode mode() const;

 signals:
  void changed();  // any control changed (debounced consumers reconnect)

 private:
  void buildUi();
  void connectChanged();
  void updateModeDependentUi();
  void updateResizeUi();
  QString pickColor(const QString& current);

  // Mode
  QComboBox* modeCombo_ = nullptr;
  QCheckBox* explodeByNameCheck_ = nullptr;

  // Optimize / quantize
  QSpinBox* optimizeSpin_ = nullptr;
  QSpinBox* lossySpin_ = nullptr;
  QCheckBox* colorsCheck_ = nullptr;
  QSpinBox* colorsSpin_ = nullptr;
  QComboBox* ditherCombo_ = nullptr;
  QComboBox* colorMethodCombo_ = nullptr;
  QCheckBox* carefulCheck_ = nullptr;

  // Resize / scale
  QComboBox* resizeKindCombo_ = nullptr;
  QSpinBox* resizeWSpin_ = nullptr;
  QSpinBox* resizeHSpin_ = nullptr;
  QDoubleSpinBox* scaleXSpin_ = nullptr;
  QDoubleSpinBox* scaleYSpin_ = nullptr;
  QComboBox* resizeMethodCombo_ = nullptr;

  // Geometry
  QComboBox* rotateCombo_ = nullptr;
  QCheckBox* flipHCheck_ = nullptr;
  QCheckBox* flipVCheck_ = nullptr;
  QCheckBox* interlaceCheck_ = nullptr;
  QCheckBox* positionCheck_ = nullptr;
  QSpinBox* posXSpin_ = nullptr;
  QSpinBox* posYSpin_ = nullptr;

  // Crop
  QCheckBox* cropCheck_ = nullptr;
  QSpinBox* cropXSpin_ = nullptr;
  QSpinBox* cropYSpin_ = nullptr;
  QSpinBox* cropWSpin_ = nullptr;
  QSpinBox* cropHSpin_ = nullptr;
  QCheckBox* cropTransparencyCheck_ = nullptr;

  // Animation
  QCheckBox* delayCheck_ = nullptr;
  QSpinBox* delaySpin_ = nullptr;      // 1/100 s units (E7!)
  QComboBox* loopCombo_ = nullptr;
  QSpinBox* loopSpin_ = nullptr;
  QComboBox* disposalCombo_ = nullptr;
  QCheckBox* unoptimizeCheck_ = nullptr;
  QSpinBox* threadsSpin_ = nullptr;

  // Colors / gamma / transparency
  QComboBox* gammaCombo_ = nullptr;
  QLineEdit* gammaEdit_ = nullptr;
  QLineEdit* backgroundEdit_ = nullptr;
  QLineEdit* transparentEdit_ = nullptr;

  // Metadata
  QCheckBox* removeCommentsCheck_ = nullptr;
  QCheckBox* removeNamesCheck_ = nullptr;
  QCheckBox* removeExtensionsCheck_ = nullptr;
  QLineEdit* commentEdit_ = nullptr;
  QListWidget* commentList_ = nullptr;
};

#endif  // GIFSCYTHE_SETTINGSPANEL_H
