#include "qtui/SettingsPanel.h"

#include <QAction>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
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

// XNConvert-style inline help: tooltips are useful for discovery, but a user
// should not have to hover over every unfamiliar gifsicle term. These labels
// stay in the Actions tab and explain the practical trade-off in one sentence.
QLabel* addDescription(QFormLayout* form, QWidget* parent, const QString& text,
                       const QString& objectName = QString()) {
  auto* label = new QLabel(text, parent);
  if (!objectName.isEmpty()) label->setObjectName(objectName);
  label->setProperty("role", "description");
  label->setWordWrap(true);
  label->setTextFormat(Qt::PlainText);
  label->setStyleSheet(QStringLiteral("QLabel[role=description] { color: palette(mid); "
                                      "font-size: 11px; margin-bottom: 3px; }"));
  form->addRow(QString(), label);
  return label;
}

}  // namespace

// ===========================================================================
// Slice D — the action stack.
//
// The reference's Actions tab is a list of CARDS, and the card header is where
// three different verbs live side by side:
//   [v] Title            -  (reset)      [x] Enabled  v
// `-` removes the card from the stack, `Enabled` disables it in place, and the
// chevron collapses it. (XN_CONVERT_PROGRAM_STUDY.md 4.2/4.4 measured exactly
// this: the `-` DELETED a card and dropped the counter to [0/0], while the
// Enabled link kept the card and dropped the counter to [0/1].)
//
// Everything here is deliberately a shell around the controls that already
// exist: the widgets, their objectNames and their tooltips are untouched, so
// the 325 harness CHECK sites and the honesty guarantees are unchanged.
// ===========================================================================

ActionCard::ActionCard(int id, const QString& title, QWidget* parent)
    : QWidget(parent), id_(id) {
  auto* outer = new QVBoxLayout(this);
  outer->setContentsMargins(0, 0, 0, 0);
  outer->setSpacing(0);

  auto* header = new QFrame(this);
  header->setObjectName(QStringLiteral("actionCardHeader"));
  header->setFrameShape(QFrame::StyledPanel);
  // Palette roles only, never a literal colour: dark mode (D-06) must stay
  // possible, and QL006 fails the gate on hard-coded hex.
  header->setStyleSheet(QStringLiteral(
      "QFrame#actionCardHeader { background: palette(midlight); "
      "border: 1px solid palette(mid); }"));
  auto* row = new QHBoxLayout(header);
  row->setContentsMargins(4, 2, 4, 2);
  row->setSpacing(6);

  chevron_ = new QToolButton(header);
  chevron_->setObjectName(QStringLiteral("actionCardChevron_%1").arg(id_));
  chevron_->setAutoRaise(true);
  chevron_->setCheckable(true);
  chevron_->setChecked(true);
  chevron_->setToolTip(QStringLiteral("Collapse this action"));
  connect(chevron_, &QToolButton::toggled, this, [this](bool expanded) {
    setCollapsed(!expanded);
    emit collapsedChanged(id_, !expanded);
  });
  row->addWidget(chevron_);

  titleLabel_ = new QLabel(title, header);
  titleLabel_->setObjectName(QStringLiteral("actionCardTitle"));
  QFont bold = titleLabel_->font();
  bold.setBold(true);
  titleLabel_->setFont(bold);
  row->addWidget(titleLabel_);
  row->addStretch();

  auto* remove = new QToolButton(header);
  remove->setObjectName(QStringLiteral("actionCardRemove_%1").arg(id_));
  remove->setText(QStringLiteral("−"));  // the reference's minus
  remove->setAutoRaise(true);
  remove->setToolTip(QStringLiteral("Remove this action from the stack"));
  connect(remove, &QToolButton::clicked, this, [this]() { emit removeRequested(id_); });
  row->addWidget(remove);

  auto* reset = new QToolButton(header);
  reset->setObjectName(QStringLiteral("actionCardReset_%1").arg(id_));
  reset->setText(QStringLiteral("↺"));  // the reference's reset arrow
  reset->setAutoRaise(true);
  reset->setToolTip(QStringLiteral("Reset this action to its defaults"));
  connect(reset, &QToolButton::clicked, this, [this]() { emit resetRequested(id_); });
  row->addWidget(reset);

  enabledCheck_ = new QCheckBox(QStringLiteral("Enabled"), header);
  enabledCheck_->setObjectName(QStringLiteral("actionCardEnabled_%1").arg(id_));
  enabledCheck_->setChecked(true);
  enabledCheck_->setToolTip(QStringLiteral(
      "Off keeps the card on screen but leaves its options out of the command."));
  connect(enabledCheck_, &QCheckBox::toggled, this, [this](bool on) {
    setActionEnabled(on);
    emit enabledChanged(id_, on);
  });
  row->addWidget(enabledCheck_);

  outer->addWidget(header);
  updateChevron();
}

QString ActionCard::title() const { return titleLabel_ ? titleLabel_->text() : QString(); }

void ActionCard::setBody(QWidget* body) {
  // Never destroy a previous body: its controls are the contract with the
  // harness. Park it instead (removeCard() parents it to the panel).
  if (body_ && body_ != body) {
    layout()->removeWidget(body_);
    body_->setParent(nullptr);
    body_->hide();
  }
  body_ = body;
  if (!body_) return;
  // The group box keeps its own title OUT of sight: the header above already
  // says it, and two titles stacked would look like a mistake.
  if (auto* box = qobject_cast<QGroupBox*>(body_)) box->setTitle(QString());
  layout()->addWidget(body_);
  body_->setVisible(!collapsed_);
}

QWidget* ActionCard::takeBody() {
  QWidget* body = body_;
  body_ = nullptr;
  if (body) {
    body->setParent(nullptr);
    body->hide();
  }
  return body;
}

bool ActionCard::actionEnabled() const {
  return enabledCheck_ && enabledCheck_->isChecked();
}

void ActionCard::setActionEnabled(bool on) {
  if (!enabledCheck_) return;
  if (enabledCheck_->isChecked() != on) {
    QSignalBlocker blocker(enabledCheck_);
    enabledCheck_->setChecked(on);
  }
  // A disabled card greyed out is the reference's behaviour and the honest one:
  // the values stay readable, they just do not reach the command.
  if (body_) body_->setEnabled(on);
}

void ActionCard::setCollapsed(bool collapsed) {
  collapsed_ = collapsed;
  if (body_) body_->setVisible(!collapsed_);
  if (chevron_ && chevron_->isChecked() == collapsed) {
    QSignalBlocker blocker(chevron_);
    chevron_->setChecked(!collapsed);
  }
  updateChevron();
}

void ActionCard::updateChevron() {
  if (!chevron_) return;
  chevron_->setText(collapsed_ ? QStringLiteral("▼") : QStringLiteral("▲"));
  chevron_->setToolTip(collapsed_ ? QStringLiteral("Expand this action")
                                  : QStringLiteral("Collapse this action"));
}

SettingsPanel::SettingsPanel(QWidget* parent) : QWidget(parent) {
  buildUi();
  connectChanged();
  updateModeDependentUi();
  updateResizeUi();
  // Captured last, so it includes whatever the build/refresh passes settled on.
  // Every card is in the stack here, so the gate inside writeInto() is a no-op.
  writeInto(defaults_);
  refreshActionCounts();
}

// ---------------------------------------------------------------------------
// The stack plumbing.
//
// Removing a card must NOT destroy the controls inside it: their objectNames
// are a contract with the harness, and the reference's "Add action >" puts the
// same action back with its values intact. So a removed card hands its body
// back and ActionCard lets go of it; the body is parked in bodies_ and only
// re-parented when the card is added again.
// ---------------------------------------------------------------------------

ActionCard* SettingsPanel::card(int id) const {
  return (id >= 0 && id < CardCount) ? cards_.value(id) : nullptr;
}

bool SettingsPanel::cardActive(int id) const {
  const ActionCard* c = card(id);
  return c != nullptr && c->actionEnabled();
}

int SettingsPanel::actionCount() const {
  int n = 0;
  for (int i = 0; i < CardCount; ++i) {
    if (cards_.value(i)) ++n;
  }
  return n;
}

int SettingsPanel::enabledActionCount() const {
  int n = 0;
  for (int i = 0; i < CardCount; ++i) {
    if (cardActive(i)) ++n;
  }
  return n;
}

ActionCard* SettingsPanel::addCard(int id, const QString& title, QWidget* groupBox) {
  if (cards_.value(id)) return cards_.value(id);
  if (id < 0 || id >= CardCount) return nullptr;
  if (groupBox) bodies_[id] = groupBox;
  QWidget* body = bodies_[id];
  if (!body) return nullptr;

  auto* c = new ActionCard(id, title, this);
  c->setObjectName(QStringLiteral("actionCard_%1").arg(id));
  c->setBody(body);
  connect(c, &ActionCard::enabledChanged, this, [this](int, bool) {
    refreshActionCounts();
    emit actionCountsChanged();
    emit changed();
  });
  connect(c, &ActionCard::removeRequested, this, [this](int cardId) { removeCard(cardId); });
  connect(c, &ActionCard::resetRequested, this, [this](int cardId) { resetCard(cardId); });
  connect(c, &ActionCard::collapsedChanged, this, [this](int, bool) {
    emit actionCountsChanged();  // persistence piggybacks on this signal
  });
  cards_[id] = c;
  // Before the trailing stretch, so the stack grows downwards like a list.
  if (cardLayout_) {
    cardLayout_->insertWidget(std::max(0, cardLayout_->count() - 1), c);
  }
  refreshActionCounts();
  emit actionCountsChanged();
  return c;
}

void SettingsPanel::removeCard(int id) {
  ActionCard* c = card(id);
  if (!c) return;
  bodies_[id] = c->takeBody();  // keep the controls alive
  if (bodies_[id]) {
    // Parent it back to the panel (hidden): an unparented QWidget would leak
    // for the lifetime of the process, and this one must survive to be added
    // back with its values.
    bodies_[id]->setParent(this);
    bodies_[id]->hide();
  }
  cards_[id] = nullptr;
  // Hide now: deleteLater() only runs when the event loop gets a turn, and a
  // still-visible empty header would read as a half-removed card.
  c->hide();
  c->deleteLater();
  refreshActionCounts();
  emit actionCountsChanged();
  emit changed();
}

void SettingsPanel::showAddActionMenu() {
  QMenu* menu = addActionButton_ ? addActionButton_->menu() : nullptr;
  if (!menu) return;
  menu->clear();
  struct Entry { int id; const char* title; const char* hint; };
  const Entry entries[] = {
      {CardMode, "Mode", "Batch / Merge / Explode - what the run produces"},
      {CardOptimize, "Optimize", "Optimization level, lossy, colormap, dither"},
      {CardResize, "Resize", "Absolute size or a scale factor, plus resampling"},
      {CardGeometry, "Geometry", "Rotate, flip, position, interlace"},
      {CardCrop, "Crop", "Rectangle crop, optionally keeping transparency"},
      {CardAnimation, "Animation", "Delay, loop count, disposal, threads"},
      {CardColors, "Colors / Transparency", "Gamma, background and transparent colour"},
      {CardMetadata, "Metadata", "Comments, frame names, unknown extensions"},
  };
  for (const Entry& e : entries) {
    const bool present = cards_.value(e.id) != nullptr;
    QAction* act = menu->addAction(QString::fromLatin1(e.title));
    act->setToolTip(QString::fromLatin1(e.hint));
    act->setEnabled(!present);
    if (present) act->setText(QStringLiteral("%1  (in the stack)").arg(QString::fromLatin1(e.title)));
    const int id = e.id;
    connect(act, &QAction::triggered, this, [this, id]() {
      // The body is parked in bodies_ from the first buildUi() pass, so adding
      // a card back restores the SAME widgets - values and objectNames alive.
      // Deliberately NO writeInto()/readFrom() round trip: writeInto() zeroes
      // the fields of an inactive card, so a round trip would wipe exactly the
      // values that parking the body was meant to keep.
      addCard(id, actionTitle(id), nullptr);
    });
  }
  menu->addSeparator();
  QAction* all = menu->addAction(QStringLiteral("Add all actions"));
  all->setEnabled(actionCount() < CardCount);
  connect(all, &QAction::triggered, this, [this]() {
    for (int i = 0; i < CardCount; ++i) {
      if (!cards_.value(i)) addCard(i, actionTitle(i), nullptr);
    }
  });
}

QString SettingsPanel::actionTitle(int id) {
  switch (id) {
    case CardMode: return QStringLiteral("Mode");
    case CardOptimize: return QStringLiteral("Optimize");
    case CardResize: return QStringLiteral("Resize");
    case CardGeometry: return QStringLiteral("Geometry");
    case CardCrop: return QStringLiteral("Crop");
    case CardAnimation: return QStringLiteral("Animation");
    case CardColors: return QStringLiteral("Colors / Transparency");
    case CardMetadata: return QStringLiteral("Metadata");
    default: return QStringLiteral("Action");
  }
}

void SettingsPanel::refreshActionCounts() {
  const int total = actionCount();
  const int on = enabledActionCount();
  if (actionsCountLabel_) {
    actionsCountLabel_->setText(QStringLiteral("Actions [%1/%2]").arg(on).arg(total));
  }
}

QString SettingsPanel::cardState() const {
  // id:present:enabled:collapsed, semicolon separated. Small, and readable in
  // the settings file, which is the point of a conf-file format.
  QStringList parts;
  for (int i = 0; i < CardCount; ++i) {
    const ActionCard* c = card(i);
    parts << QStringLiteral("%1:%2:%3:%4")
                 .arg(i)
                 .arg(c ? 1 : 0)
                 .arg(c && c->actionEnabled() ? 1 : 0)
                 .arg(c && c->isCollapsed() ? 1 : 0);
  }
  return parts.join(QLatin1Char(';'));
}

void SettingsPanel::setCardState(const QString& state) {
  if (state.trimmed().isEmpty()) return;
  // Restoring a session must not look like the user editing eight cards: no
  // changed() storm, no preview scheduled per card. The caller refreshes once.
  const QSignalBlocker blocker(this);
  const QStringList parts = state.split(QLatin1Char(';'), Qt::SkipEmptyParts);
  for (const QString& part : parts) {
    const QStringList f = part.split(QLatin1Char(':'));
    if (f.size() != 4) continue;
    const int id = f.at(0).toInt();
    if (id < 0 || id >= CardCount) continue;
    const bool present = f.at(1) == QStringLiteral("1");
    const bool enabled = f.at(2) == QStringLiteral("1");
    const bool collapsed = f.at(3) == QStringLiteral("1");
    if (present && !cards_.value(id)) addCard(id, actionTitle(id), nullptr);
    if (!present && cards_.value(id)) removeCard(id);
    if (cards_.value(id)) {
      cards_.value(id)->setActionEnabled(enabled);
      cards_.value(id)->setCollapsed(collapsed);
    }
  }
  refreshActionCounts();
}

// Copy ONE card's fields between two Settings. Used twice, for the same reason:
//   * writeInto(): a card that is removed or disabled must contribute the
//     DEFAULT value of its fields, i.e. nothing. That is what "disabled" means.
//   * resetCard(): the inverse - restore one card's defaults without touching
//     the user's work in every other card.
void SettingsPanel::copyCardFields(int id, const gs::Settings& from, gs::Settings& to) {
  switch (id) {
    case CardMode:
      to.mode = from.mode;
      to.explode_by_name = from.explode_by_name;
      break;
    case CardOptimize:
      to.optimize_level = from.optimize_level;
      to.lossy = from.lossy;
      to.color_count = from.color_count;
      to.dither = from.dither;
      to.dither_method = from.dither_method;
      to.color_method = from.color_method;
      to.careful = from.careful;
      break;
    case CardResize:
      to.resize_kind = from.resize_kind;
      to.resize_w = from.resize_w;
      to.resize_h = from.resize_h;
      to.scale_x = from.scale_x;
      to.scale_y = from.scale_y;
      to.resize_method = from.resize_method;
      break;
    case CardGeometry:
      to.rotation = from.rotation;
      to.flip_horizontal = from.flip_horizontal;
      to.flip_vertical = from.flip_vertical;
      to.position_x = from.position_x;
      to.position_y = from.position_y;
      to.has_position = from.has_position;
      to.interlace = from.interlace;
      break;
    case CardCrop:
      to.crop = from.crop;
      to.crop_x = from.crop_x;
      to.crop_y = from.crop_y;
      to.crop_w = from.crop_w;
      to.crop_h = from.crop_h;
      to.crop_transparency = from.crop_transparency;
      break;
    case CardAnimation:
      to.delay_cs = from.delay_cs;
      to.disposal = from.disposal;
      to.loopcount = from.loopcount;
      to.unoptimize = from.unoptimize;
      to.threads = from.threads;
      break;
    case CardColors:
      to.gamma_str = from.gamma_str;
      to.gamma = from.gamma;
      to.background = from.background;
      to.transparent = from.transparent;
      break;
    case CardMetadata:
      to.remove_comments = from.remove_comments;
      to.remove_names = from.remove_names;
      to.remove_extensions = from.remove_extensions;
      to.comments = from.comments;
      break;
    default:
      break;
  }
}

void SettingsPanel::resetCard(int id) {
  // Read EVERY control (not just the active cards) so that resetting one card
  // cannot quietly wipe the values parked in a removed or disabled one.
  gs::Settings current;
  writeAllInto(current);
  copyCardFields(id, defaults_, current);
  readFrom(current);
  emit changed();
}

void SettingsPanel::buildUi() {
  auto* outer = new QVBoxLayout(this);
  outer->setContentsMargins(0, 0, 0, 0);

  cards_.fill(nullptr, CardCount);
  bodies_.fill(nullptr, CardCount);

  auto* scroll = new QScrollArea(this);
  actionsScroll_ = scroll;
  scroll->setObjectName(QStringLiteral("actionsScroll"));
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  auto* content = new QWidget(scroll);
  auto* lay = new QVBoxLayout(content);

  // ---- toolbar (the reference: "Add action >" left, "Clear all" right) ----
  auto* toolbar = new QHBoxLayout();
  addActionButton_ = new QToolButton(content);
  addActionButton_->setObjectName(QStringLiteral("addActionButton"));
  addActionButton_->setText(QStringLiteral("Add action >"));
  addActionButton_->setPopupMode(QToolButton::InstantPopup);
  addActionButton_->setToolTip(QStringLiteral(
      "Put an action back into the stack. Removing a card never loses its settings."));
  auto* addMenu = new QMenu(addActionButton_);
  addMenu->setObjectName(QStringLiteral("addActionMenu"));
  addMenu->setToolTipsVisible(true);
  addActionButton_->setMenu(addMenu);
  connect(addMenu, &QMenu::aboutToShow, this, [this, addMenu]() { showAddActionMenu(); (void)addMenu; });
  toolbar->addWidget(addActionButton_);

  actionsCountLabel_ = new QLabel(content);
  actionsCountLabel_->setObjectName(QStringLiteral("actionsCountLabel"));
  toolbar->addWidget(actionsCountLabel_);
  toolbar->addStretch();

  auto* clearAll = new QToolButton(content);
  clearAll->setObjectName(QStringLiteral("clearActionsButton"));
  clearAll->setText(QStringLiteral("Clear all"));
  clearAll->setToolTip(QStringLiteral("Remove every action from the stack"));
  connect(clearAll, &QToolButton::clicked, this, [this]() {
    for (int i = 0; i < CardCount; ++i) {
      if (cards_.value(i)) removeCard(i);
    }
  });
  toolbar->addWidget(clearAll);
  lay->addLayout(toolbar);

  auto* intro = new QLabel(QStringLiteral(
      "One card per action. Drag-free ordering: gifsicle applies these in a fixed pipeline, so the "
      "stack order is the engine order and the UI does not pretend otherwise. Hover a control for "
      "the exact gifsicle option."), content);
  intro->setObjectName(QStringLiteral("actionsIntro"));
  intro->setWordWrap(true);
  intro->setTextFormat(Qt::PlainText);
  lay->addWidget(intro);
  cardLayout_ = lay;

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
  addDescription(form, modeBox,
                 QStringLiteral("Batch makes one result per input; Merge combines the queue; "
                                "Explode writes one GIF per frame; Auto is the low-level one-file mode."),
                 QStringLiteral("modeDescription"));

  explodeByNameCheck_ = new QCheckBox(QStringLiteral("Name frames after input files (-E)"), modeBox);
  explodeByNameCheck_->setObjectName(QStringLiteral("explodeByNameCheck"));
  form->addRow(QString(), explodeByNameCheck_);
  addDescription(form, modeBox,
                 QStringLiteral("-E keeps names already stored inside a GIF. It does not invent names for unnamed frames."),
                 QStringLiteral("explodeDescription"));
  addCard(CardMode, QStringLiteral("Mode"), modeBox);

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
  addDescription(form, optBox,
                 QStringLiteral("Removes redundant pixels and improves the file layout. Higher levels can take longer; "
                                "level 0 turns this pass off without changing colors."),
                 QStringLiteral("optimizationDescription"));

  lossySpin_ = new QSpinBox(optBox);
  lossySpin_->setObjectName(QStringLiteral("lossySpin"));
  lossySpin_->setRange(0, 200);
  lossySpin_->setValue(0);
  lossySpin_->setSpecialValueText(QStringLiteral("Off"));
  lossySpin_->setToolTip(QStringLiteral(
      "--lossy=N: allow small color errors for much smaller files.\n"
      "Typical: 20–80. 0 = lossless."));
  form->addRow(QStringLiteral("Lossy compression"), lossySpin_);
  addDescription(form, optBox,
                 QStringLiteral("Allows small visual changes for a smaller GIF. 0 is lossless; start around 20–60 "
                                "when file size matters more than exact pixels."),
                 QStringLiteral("lossyDescription"));

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
  addDescription(form, optBox,
                 QStringLiteral("Limits the palette per frame. Fewer colors reduce size but can create banding in "
                                "gradients; enable Dither to hide some of that banding."),
                 QStringLiteral("colorsDescription"));

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
  addDescription(form, optBox,
                 QStringLiteral("Dithering mixes nearby palette colors in a pattern so gradients look smoother. "
                                "It can add detail and may make encoding slower."),
                 QStringLiteral("ditherDescription"));

  colorMethodCombo_ = new QComboBox(optBox);
  colorMethodCombo_->setObjectName(QStringLiteral("colorMethodCombo"));
  colorMethodCombo_->addItem(QStringLiteral("Engine default"), QString());
  for (const char* m : kColorMethods)
    colorMethodCombo_->addItem(QString::fromLatin1(m), QString::fromLatin1(m));
  form->addRow(QStringLiteral("Color method"), colorMethodCombo_);
  addDescription(form, optBox,
                 QStringLiteral("Chooses how colors are sampled when reducing a palette. Engine default is the "
                                "safe starting point; the named methods are advanced tuning."),
                 QStringLiteral("colorMethodDescription"));

  carefulCheck_ = new QCheckBox(QStringLiteral("Careful (slightly larger, safer output)"), optBox);
  carefulCheck_->setObjectName(QStringLiteral("carefulCheck"));
  carefulCheck_->setToolTip(QStringLiteral(
      "--careful: avoids some common software compatibility issues."));
  form->addRow(QString(), carefulCheck_);
  addDescription(form, optBox,
                 QStringLiteral("Careful favors compatibility with older or strict GIF players over the smallest "
                                "possible output."),
                 QStringLiteral("carefulDescription"));
  addCard(CardOptimize, QStringLiteral("Optimize"), optBox);

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
  addDescription(form, resizeBox,
                 QStringLiteral("Fit and Touch keep the aspect ratio. Exact may stretch. Width/Height change one "
                                "axis; Scale uses independent X and Y percentages."),
                 QStringLiteral("resizeDescription"));

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
  addDescription(form, resizeBox,
                 QStringLiteral("The target box in pixels. Width and height are used by Fit, Touch and Exact; "
                                "unused numbers are ignored."),
                 QStringLiteral("resizeSizeDescription"));

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
  addDescription(form, resizeBox,
                 QStringLiteral("100% keeps the current size. 50% makes each axis half as large; values can be "
                                "different when you intentionally want non-uniform scaling."),
                 QStringLiteral("scaleDescription"));

  resizeMethodCombo_ = new QComboBox(resizeBox);
  resizeMethodCombo_->setObjectName(QStringLiteral("resizeMethodCombo"));
  resizeMethodCombo_->addItem(QStringLiteral("Engine default"), QString());
  for (const char* m : kResizeMethods)
    resizeMethodCombo_->addItem(QString::fromLatin1(m), QString::fromLatin1(m));
  resizeMethodCombo_->setToolTip(QStringLiteral(
      "Resampling algorithm (--resize-method). Values from the engine's\n"
      "RESIZE_METHOD_TYPE list: point, mix, box, catrom, lanczos2,\n"
      "lanczos3, mitchell."));
  form->addRow(QStringLiteral("Resampling method"), resizeMethodCombo_);
  addDescription(form, resizeBox,
                 QStringLiteral("Controls how new pixels are calculated. Point is fastest and sharp; Lanczos/Mitchell "
                                "are smoother quality choices."),
                 QStringLiteral("resizeMethodDescription"));
  addCard(CardResize, QStringLiteral("Resize"), resizeBox);

  // ================= Geometry =================
  auto* geoBox = makeGroup(QStringLiteral("Geometry"), QStringLiteral("geometryGroup"), &form);

  rotateCombo_ = new QComboBox(geoBox);
  rotateCombo_->setObjectName(QStringLiteral("rotateCombo"));
  rotateCombo_->addItem(QStringLiteral("No rotation"), static_cast<int>(gs::Rotation::None));
  rotateCombo_->addItem(QStringLiteral("Rotate 90° clockwise"), static_cast<int>(gs::Rotation::R90));
  rotateCombo_->addItem(QStringLiteral("Rotate 180°"), static_cast<int>(gs::Rotation::R180));
  rotateCombo_->addItem(QStringLiteral("Rotate 270° clockwise"), static_cast<int>(gs::Rotation::R270));
  form->addRow(QStringLiteral("Rotate"), rotateCombo_);
  addDescription(form, geoBox,
                 QStringLiteral("Rotates every frame around the animation canvas."),
                 QStringLiteral("rotateDescription"));

  flipHCheck_ = new QCheckBox(QStringLiteral("Flip horizontal"), geoBox);
  flipHCheck_->setObjectName(QStringLiteral("flipHCheck"));
  flipVCheck_ = new QCheckBox(QStringLiteral("Flip vertical"), geoBox);
  flipVCheck_->setObjectName(QStringLiteral("flipVCheck"));
  auto* flipRow = new QHBoxLayout();
  flipRow->addWidget(flipHCheck_);
  flipRow->addWidget(flipVCheck_);
  flipRow->addStretch();
  form->addRow(QStringLiteral("Flip"), flipRow);
  addDescription(form, geoBox,
                 QStringLiteral("Mirrors the frames horizontally or vertically."),
                 QStringLiteral("flipDescription"));

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
  addDescription(form, geoBox,
                 QStringLiteral("Moves the frame origin inside the logical screen. Leave this off unless you need "
                                "to preserve a specific canvas position."),
                 QStringLiteral("positionDescription"));

  interlaceCheck_ = new QCheckBox(QStringLiteral("Interlace (progressive loading)"), geoBox);
  interlaceCheck_->setObjectName(QStringLiteral("interlaceCheck"));
  form->addRow(QString(), interlaceCheck_);
  addDescription(form, geoBox,
                 QStringLiteral("Interlacing lets some players display a rough preview while the GIF downloads. "
                                "It is not a compression setting."),
                 QStringLiteral("interlaceDescription"));
  addCard(CardGeometry, QStringLiteral("Geometry"), geoBox);

  // ================= Crop =================
  auto* cropBox = makeGroup(QStringLiteral("Crop"), QStringLiteral("cropGroup"), &form);
  cropCheck_ = new QCheckBox(QStringLiteral("Crop to rectangle"), cropBox);
  cropCheck_->setObjectName(QStringLiteral("cropCheck"));
  form->addRow(QString(), cropCheck_);
  addDescription(form, cropBox,
                 QStringLiteral("Crop removes pixels outside a rectangle. It is useful before resizing when the "
                                "source has unwanted borders."),
                 QStringLiteral("cropDescription"));

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
  addDescription(form, cropBox,
                 QStringLiteral("X and Y are the top-left corner; W and H are width and height in pixels. "
                                "The engine uses its X,Y+WxH syntax under the hood."),
                 QStringLiteral("cropRectangleDescription"));
  auto* cropHint = new QLabel(QStringLiteral("Emitted as --crop X,Y+WxH (engine plus-form)."), cropBox);
  cropHint->setObjectName(QStringLiteral("cropHintLabel"));
  cropHint->setWordWrap(true);
  form->addRow(QString(), cropHint);

  cropTransparencyCheck_ = new QCheckBox(QStringLiteral("Crop transparent edges afterwards"), cropBox);
  cropTransparencyCheck_->setObjectName(QStringLiteral("cropTransparencyCheck"));
  form->addRow(QString(), cropTransparencyCheck_);
  addDescription(form, cropBox,
                 QStringLiteral("After cropping, trim transparent edges too. This can make the logical canvas "
                                "tighter but may change frame positioning."),
                 QStringLiteral("cropTransparencyDescription"));
  for (QSpinBox* s : {cropXSpin_, cropYSpin_, cropWSpin_, cropHSpin_}) s->setEnabled(false);
  cropTransparencyCheck_->setEnabled(false);
  addCard(CardCrop, QStringLiteral("Crop"), cropBox);

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
  addDescription(form, animBox,
                 QStringLiteral("GIF timing is in hundredths of a second: 10 means 0.10 s. It is not milliseconds."),
                 QStringLiteral("delayDescription"));

  loopCombo_ = new QComboBox(animBox);
  loopCombo_->setObjectName(QStringLiteral("loopCombo"));
  loopCombo_->addItem(QStringLiteral("Keep original"), 0);
  loopCombo_->addItem(QStringLiteral("Loop forever"), 1);
  loopCombo_->addItem(QStringLiteral("Loop N times…"), 2);
  // U-63 / P1-40: "play once" is a fourth state, not "keep original" — the engine
  // flag is --no-loopcount. Appended at the end on purpose: the offscreen harness
  // (and every saved-conf expectation) addresses items 0-2 by index.
  loopCombo_->addItem(QStringLiteral("Play once (no loop)"), 3);
  form->addRow(QStringLiteral("Looping"), loopCombo_);
  addDescription(form, animBox,
                 QStringLiteral("Forever repeats the animation; Play once removes the loop extension; Keep original "
                                "does not change the source's loop behavior."),
                 QStringLiteral("loopDescription"));

  loopSpin_ = new QSpinBox(animBox);
  loopSpin_->setObjectName(QStringLiteral("loopSpin"));
  loopSpin_->setRange(1, 65535);
  loopSpin_->setValue(3);
  loopSpin_->setEnabled(false);
  form->addRow(QStringLiteral("Loop count"), loopSpin_);
  addDescription(form, animBox,
                 QStringLiteral("Only used for Loop N times. The number means total plays, not extra repeats."),
                 QStringLiteral("loopCountDescription"));

  disposalCombo_ = new QComboBox(animBox);
  disposalCombo_->setObjectName(QStringLiteral("disposalCombo"));
  disposalCombo_->addItem(QStringLiteral("Keep original"), -1);
  disposalCombo_->addItem(QStringLiteral("none (0) — unspecified"), 0);
  disposalCombo_->addItem(QStringLiteral("asis (1) — do not dispose"), 1);
  disposalCombo_->addItem(QStringLiteral("background (2) — restore to background"), 2);
  disposalCombo_->addItem(QStringLiteral("previous (3) — restore to previous"), 3);
  // DS-10 / P3-11 (S37): 4..7 are reachable. GIF89a defines 0..3 and leaves
  // 4..7 "to be defined"; gifsicle's DISPOSAL_TYPE parser accepts every value
  // 0..7 (Clp_AllowNumbers) and its bounds check is `val.i < 0 || val.i > 7`
  // (gifsicle.c DISPOSAL_OPT), and web/validate.mjs admits 0..7 too. The
  // desktop picker stopped at 3, so a session exported from the web (or a
  // config a user hand-wrote) could not be represented here — readFrom()
  // silently left the control at "Keep original". Values stay numeric, which
  // is what the engine receives, so nothing here invents a name the engine
  // does not know.
  for (int d = 4; d <= 7; ++d) {
    disposalCombo_->addItem(
        QStringLiteral("%1 (reserved — GIF89a \u201Cto be defined\u201D)").arg(d), d);
  }
  disposalCombo_->setToolTip(QStringLiteral(
      "Frame disposal method (--disposal). Names/values from the engine's\n"
      "DISPOSAL_TYPE parser (none/asis/background/previous, or 0..7).\n"
      "4..7 are reserved by the GIF spec and passed to the engine as-is."));
  form->addRow(QStringLiteral("Disposal"), disposalCombo_);
  addDescription(form, animBox,
                 QStringLiteral("Tells a player what to do with the previous frame. Keep original is safest; "
                                "reserved values are for advanced GIF compatibility work."),
                 QStringLiteral("disposalDescription"));

  unoptimizeCheck_ = new QCheckBox(QStringLiteral("Unoptimize (expand to full frames, -U)"), animBox);
  unoptimizeCheck_->setObjectName(QStringLiteral("unoptimizeCheck"));
  form->addRow(QString(), unoptimizeCheck_);
  addDescription(form, animBox,
                 QStringLiteral("Expands optimized frames back to full frames. Use this when another tool needs "
                                "simple, self-contained frames; it usually increases size."),
                 QStringLiteral("unoptimizeDescription"));

  threadsSpin_ = new QSpinBox(animBox);
  threadsSpin_->setObjectName(QStringLiteral("threadsSpin"));
  // DS-07 / P1-30, and it only became a real bug with DS-06 / P0-2: the range used
  // to start at 0, so "no thread flag" was unrepresentable AND writeInto() always
  // wrote a value — a conf saying `threads = -1` was silently rewritten to 0 by
  // opening and closing the window. With the tri-state, 0 is a *request* for the
  // engine's own count (bare -j), so it can no longer stand in for "unset".
  threadsSpin_->setRange(gs::GS_THREADS_UNSET, 64);
  threadsSpin_->setValue(gs::GS_THREADS_UNSET);
  threadsSpin_->setSpecialValueText(QStringLiteral("Unchanged (engine default)"));
  threadsSpin_->setToolTip(QStringLiteral(
      "Unchanged: no thread flag (the engine's own default, 1 thread)\n"
      "0: auto — passes a bare -j, i.e. the engine's thread count\n"
      "N>0: passes -jN"));
  form->addRow(QStringLiteral("Threads"), threadsSpin_);
  addDescription(form, animBox,
                 QStringLiteral("Parallelizes some work. Unchanged uses the engine default; Auto asks the engine for "
                                "its automatic count. More threads can use more memory on very large GIFs."),
                 QStringLiteral("threadsDescription"));
  addCard(CardAnimation, QStringLiteral("Animation"), animBox);

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
  form->addRow(QStringLiteral("Gamma / color math"), gammaCombo_);
  addDescription(form, colorBox,
                 QStringLiteral("Changes how colors are compared while quantizing. Keep original is the least "
                                "surprising choice; sRGB and Oklab are advanced color-space options."),
                 QStringLiteral("gammaDescription"));

  gammaEdit_ = new QLineEdit(colorBox);
  gammaEdit_->setObjectName(QStringLiteral("gammaEdit"));
  gammaEdit_->setPlaceholderText(QStringLiteral("e.g. 2.2"));
  gammaEdit_->setEnabled(false);
  form->addRow(QStringLiteral("Custom gamma value"), gammaEdit_);
  addDescription(form, colorBox,
                 QStringLiteral("Only used for Custom value. A common starting value is 2.2; leave it empty unless "
                                "you know the source's color assumptions."),
                 QStringLiteral("gammaValueDescription"));

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
  form->addRow(QStringLiteral("Background color"), makeColorRow(QStringLiteral("backgroundEdit"), &backgroundEdit_));
  addDescription(form, colorBox,
                 QStringLiteral("Color used when a frame disposes to the background. Empty leaves the source unchanged."),
                 QStringLiteral("backgroundDescription"));
  form->addRow(QStringLiteral("Transparent color"), makeColorRow(QStringLiteral("transparentEdit"), &transparentEdit_));
  addDescription(form, colorBox,
                 QStringLiteral("Color treated as transparent. Use a picker or #rrggbb; empty means unchanged."),
                 QStringLiteral("transparentDescription"));
  addCard(CardColors, QStringLiteral("Colors / Transparency"), colorBox);

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
  addDescription(form, metaBox,
                 QStringLiteral("Metadata is not visible picture content. Removing it can make files cleaner or "
                                "smaller, while comments can be useful for provenance."),
                 QStringLiteral("metadataDescription"));

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
  addCard(CardMetadata, QStringLiteral("Metadata"), metaBox);

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

void SettingsPanel::writeSessionInto(gs::Settings& s) const { writeAllInto(s); }

void SettingsPanel::writeInto(gs::Settings& s) const {
  writeAllInto(s);
  // ---- slice D: a card that is removed or disabled contributes NOTHING ----
  // Copying the default value of its fields is exactly that, and it is stronger
  // than "ignore the widgets": a stale value cannot leak back in after the card
  // was switched off with something typed into it.
  const gs::Settings defaults;
  for (int id = 0; id < CardCount; ++id) {
    if (!cardActive(id)) copyCardFields(id, defaults, s);
  }
}

void SettingsPanel::writeAllInto(gs::Settings& s) const {
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
    case 3:  s.loopcount = gs::GS_LOOPCOUNT_ONCE; break; // play once (--no-loopcount)
    default: s.loopcount = gs::GS_LOOPCOUNT_UNSET; break;  // unchanged
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
  if (s.loopcount == gs::GS_LOOPCOUNT_ONCE) {
    selectByData(loopCombo_, 3);  // play once (U-63) — was shown as "Keep original"
  } else if (s.loopcount == 0) {
    selectByData(loopCombo_, 1);  // forever (VP-1: emits --loopcount=0)
  } else if (s.loopcount > 0) {
    selectByData(loopCombo_, 2);  // loop N times
    setSpin(loopSpin_, s.loopcount);
  } else {
    selectByData(loopCombo_, 0);  // keep original
  }
  selectByData(disposalCombo_, s.disposal);  // -1 and 0..7, all representable (DS-10)
  unoptimizeCheck_->setChecked(s.unoptimize);
  // -1 is inside the range now (DS-07); setSpin clamps anything else, and the
  // clamped value is what the live pane shows, so a clamp cannot pass unnoticed.
  setSpin(threadsSpin_, s.threads);

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
