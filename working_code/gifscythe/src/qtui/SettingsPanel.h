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

#include <QStringList>
#include <QVector>
#include <QWidget>

#include "core/GifsicleSettings.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QScrollArea;
class QSpinBox;
class QToolButton;
class QVBoxLayout;

// One card of the Actions stack, shaped like XnConvert's (slice D, 2026-10-24):
//
//   [v] Title                        -  (reset)   [x] Enabled  v
//       <body: the action's own controls>
//
// `-` REMOVES the card from the stack (re-add it from "Add action >"), and the
// Enabled checkbox DISABLES it without removing it - two different affordances,
// exactly as the reference has them (XN_CONVERT_PROGRAM_STUDY.md 4.4). A card
// that is removed or disabled contributes nothing to the generated command.
class ActionCard : public QWidget {
  Q_OBJECT
 public:
  ActionCard(int id, const QString& title, QWidget* parent = nullptr);
  void setBody(QWidget* body);
  int id() const { return id_; }
  // Named "action" on purpose: QWidget::setEnabled()/isEnabled() exist and
  // mean "is this widget usable". A card can be enabled while its own widgets
  // are greyed out, so the two must not share a name.
  bool actionEnabled() const;
  void setActionEnabled(bool on);
  bool isCollapsed() const { return collapsed_; }
  // Hand the body back without destroying it (removing a card keeps its
  // controls alive for a later re-add).
  QWidget* takeBody();
  void setCollapsed(bool collapsed);
  QString title() const;

 signals:
  void enabledChanged(int id, bool on);
  void removeRequested(int id);
  void resetRequested(int id);
  void collapsedChanged(int id, bool collapsed);

 private:
  void updateChevron();
  int id_ = 0;
  bool collapsed_ = false;
  QToolButton* chevron_ = nullptr;
  QLabel* titleLabel_ = nullptr;
  QWidget* body_ = nullptr;
  QCheckBox* enabledCheck_ = nullptr;
};

class SettingsPanel : public QWidget {
  Q_OBJECT
 public:
  // The action registry. One card per gifsicle operation GROUP (not one per
  // flag): that is the granularity at which gifsicle's pipeline actually
  // composes, and the granularity a user thinks in ("resize, then optimise").
  enum ActionId {
    CardMode = 0,
    CardOptimize,
    CardResize,
    CardGeometry,
    CardCrop,
    CardAnimation,
    CardColors,
    CardMetadata,
    CardCount
  };

  explicit SettingsPanel(QWidget* parent = nullptr);

  // Slice D: the reference carries "Actions [enabled/total]" in the tab title.
  int actionCount() const;          // cards currently in the stack
  int enabledActionCount() const;   // ...of which enabled
  // Presence / enabled / collapsed as one string, for session persistence.
  QString cardState() const;
  void setCardState(const QString& state);

  // Write every control into s (mode, optimize, resize, crop, animation,
  // colors, metadata...). Inputs/output are NOT touched (MainWindow owns them).
  // A card that is removed or switched off contributes the DEFAULT of its
  // fields, i.e. nothing: that is what the coordinate's Enabled box means.
  void writeInto(gs::Settings& s) const;

  // Same, but ignores the card stack: every control is read, even the ones in a
  // card that is removed or off. Only for persistence - a disabled action must
  // still come back with the values the user typed into it next launch.
  void writeSessionInto(gs::Settings& s) const;

  // Inverse of writeInto: restore every control from s (used for session
  // persistence — S7). Fields that the GUI cannot represent (e.g. optimize
  // -1 "no flag") leave their control at the default; disposal 4..7 IS
  // representable since DS-10 (S37), and the -1 sentinel stays "Keep original".
  // Does NOT emit changed() — callers refresh consumers once afterwards.
  void readFrom(const gs::Settings& s);

  gs::Mode mode() const;

 signals:
  void changed();  // any control changed (debounced consumers reconnect)
  // Emitted when a card is added, removed, enabled or disabled - the caller
  // refreshes the tab title (Actions [n/m]).
  void actionCountsChanged();

 private:
  void buildUi();
  // Wrap one already-built group box in a card and put it in the stack.
  ActionCard* addCard(int id, const QString& title, QWidget* groupBox);
  void showAddActionMenu();
  void removeCard(int id);
  static QString actionTitle(int id);
  // Copy ONE card's fields between two Settings (disabled/removed => defaults;
  // reset => defaults for that card only).
  static void copyCardFields(int id, const gs::Settings& from, gs::Settings& to);
  void refreshActionCounts();
  void resetCard(int id);
  // True when the card is in the stack AND enabled: only then do its controls
  // reach the command.
  bool cardActive(int id) const;
  ActionCard* card(int id) const;
  // Read every control, ignoring the card stack (writeInto is this + the gate).
  void writeAllInto(gs::Settings& s) const;
  void connectChanged();
  void updateModeDependentUi();
  void updateResizeUi();
  QString pickColor(const QString& current);

  // ---- slice D: the stack ----
  QVector<ActionCard*> cards_;         // indexed by ActionId, nullptr if removed
  // A removed card's CONTROLS stay alive here, so "Add action >" restores the
  // same widgets (objectNames are a contract; values must not be lost).
  QVector<QWidget*> bodies_;
  // The control defaults, captured once the UI is built: what Reset restores.
  // A Settings round trip is the honest way to capture them, because it is the
  // same vocabulary writeInto/readFrom already speak.
  gs::Settings defaults_;
  QVBoxLayout* cardLayout_ = nullptr;  // where cards are inserted
  QLabel* actionsCountLabel_ = nullptr;
  QToolButton* addActionButton_ = nullptr;
  QScrollArea* actionsScroll_ = nullptr;  // kept for the harness's objectName

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
