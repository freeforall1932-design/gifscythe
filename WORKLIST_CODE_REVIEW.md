# Gifscythe Worklist - Code Review Findings

**Generated:** Auto-generated from codebase review  
**Status:** Pending Verification & Implementation  
**Priority Legend:** P0 (Critical/Blocker) | P1 (High/Before v1.0) | P2 (Medium/Post-v1.0) | P3 (Low/Enhancement)

---

## 🔴 P0 - CRITICAL BLOCKERS (Fix Immediately)

### 1. CLI Engine Path Resolution Broken
- **File:** `src/cli/main.cpp` (lines 70, 90-113)
- **Issue:** Default engine path resolves to incorrect relative path when `--engine` flag not provided
- **Impact:** CLI fails to run gifsicle without explicit path argument
- **Current Code:**
  ```cpp
  std::string engine_path = "working_code/gifscythe/release/0.1.0/gifsicle";
  ```
- **Expected:** Should resolve to repo root + `/working_code/gifscythe/release/0.1.0/gifsicle`
- **Proposed Fix:** Implement `resolve_engine_path()` function that extracts repo root from settings path
- **Verification Steps:**
  - [ ] Run `./gifscythe-cli --settings examples/settings.json --input test.gif --output out.gif` without `--engine` flag
  - [ ] Confirm it finds and executes the correct gifsicle binary
  - [ ] Test from multiple working directories (repo root, examples/, home dir)
- **Estimated Effort:** 30 minutes

### 2. GUI Process Execution Blocks UI Thread
- **File:** `src/qtui/MainWindow.cpp` (lines 122-138)
- **Issue:** Uses synchronous `waitForStarted()` and `waitForFinished()` calls
- **Impact:** UI freezes during processing; no progress feedback; cannot cancel operations
- **Current Code:**
  ```cpp
  if (!proc->waitForStarted(3000)) { /* error */ }
  if (!proc->waitForFinished(60000)) { /* timeout */ }
  ```
- **Expected:** Async execution with signals for started, progress, finished, error
- **Proposed Fix:** Replace with signal/slot connections:
  - `QProcess::started` → Update status bar
  - `QProcess::readyReadStandardOutput` → Parse progress updates
  - `QProcess::finished(int, ExitStatus)` → Handle completion/errors
  - Add cancel button connected to `QProcess::kill()`
- **Verification Steps:**
  - [ ] Process large GIF (>10MB) and confirm UI remains responsive
  - [ ] Verify progress bar updates during processing
  - [ ] Test cancel button mid-operation
  - [ ] Confirm proper error handling on gifsicle failure
- **Estimated Effort:** 2 hours

### 3. Windows Cross-Compilation Fails
- **File:** `scripts/build_gifsicle.sh`, `.github/workflows/ci.yml`
- **Issue:** mingw-w64 not installed; Windows build job fails (noted in SESSION_HANDOFF.md)
- **Impact:** Cannot produce Windows binaries; CI red on Windows matrix
- **Current State:** Script requires `x86_64-w64-mingw32-g++` which is missing
- **Options:**
  - Option A: Install mingw-w64 on CI runner and dev machines
  - Option B: Defer Windows builds until proper tooling available (update WORKLIST.md)
  - Option C: Use native Windows MSVC build instead of cross-compilation
- **Verification Steps:**
  - [ ] Attempt to run `./scripts/build_gifsicle.sh windows` on clean system
  - [ ] Check if mingw-w64 packages are available for your distro
  - [ ] Test alternative: native Windows build with Visual Studio
- **Estimated Effort:** 4-8 hours (depending on approach)

---

## 🟠 P1 - HIGH PRIORITY (Before v1.0.0 Release)

### 4. GUI Missing Most GifsicleSettings Controls
- **Files:** `src/qtui/MainWindow.cpp`, `src/core/GifsicleSettings.h`
- **Issue:** Only 3 of 20+ settings exposed in UI (optimization level, lossy, output file)
- **Missing Controls:**
  - [ ] Resize/Scale options (width, height, scale-x, scale-y, resize-fit)
  - [ ] Rotation/Flipping (rotate-90, rotate-180, rotate-270, flip-vertical, flip-horizontal)
  - [ ] Crop functionality (crop X,Y+W,H)
  - [ ] Frame timing (delay, delay-multiplier, disposal method)
  - [ ] Color controls (colors, dither, no-dither, color-method)
  - [ ] Gamma correction
  - [ ] Background color selector
  - [ ] Transparent color picker
  - [ ] Strip options (comments, names, extensions, all extra)
  - [ ] Mode selection (merge, batch, explode)
  - [ ] Loop count control
  - [ ] Resize method (box, blur, sample, scanline)
- **Impact:** Users cannot access most gifsicle features through GUI
- **Verification Steps:**
  - [ ] Compare UI widgets against all fields in `GifsicleSettings` struct
  - [ ] Create mockup/wireframe for expanded controls panel
  - [ ] Implement each control with proper signal/slot connections
  - [ ] Test each control generates correct command-line argument
- **Estimated Effort:** 16-24 hours

### 5. No Input/Actions/Output Tab Flow (XNConvert-style)
- **File:** `src/qtui/MainWindow.cpp`
- **Issue:** Current UI is single-panel; doesn't follow planned 3-tab workflow
- **Expected Structure:**
  - **Tab 1: Input** - File list, drag-drop, add/remove/clear, file count/size display
  - **Tab 2: Actions** - All transformation controls grouped by category (Optimize, Resize, Crop, Colors, etc.)
  - **Tab 3: Output** - Destination folder, filename pattern, format options, overwrite behavior
- **Impact:** Poor UX for batch operations; doesn't match project vision (PROJECT_VISION.md)
- **Verification Steps:**
  - [ ] Design tab layout matching XNConvert reference screenshots
  - [ ] Implement QTabWidget with 3 tabs
  - [ ] Migrate existing controls to appropriate tabs
  - [ ] Add missing sections per tab specification
  - [ ] Test tab navigation and state persistence
- **Estimated Effort:** 8-12 hours

### 6. Drag-and-Drop Not Implemented
- **File:** `src/qtui/MainWindow.cpp`
- **Issue:** `setAcceptDrops(true)` called but no `dragEnterEvent()` or `dropEvent()` handlers
- **Impact:** Users cannot drag files onto window as intended
- **Required Handlers:**
  ```cpp
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;
  ```
- **Expected Behavior:**
  - Accept multiple image files (gif, png, webp, apng)
  - Add dropped files to input queue
  - Show visual feedback during drag hover
- **Verification Steps:**
  - [ ] Drag single GIF file onto window → Added to input list
  - [ ] Drag multiple files → All added
  - [ ] Drag unsupported file type → Rejected with cursor feedback
  - [ ] Drag from file manager and desktop
- **Estimated Effort:** 2-3 hours

### 7. No Before/After Preview Panel
- **File:** `src/qtui/MainWindow.cpp`
- **Issue:** No visual comparison of original vs processed image
- **Impact:** Users cannot verify changes before applying
- **Required Features:**
  - Side-by-side or split-view comparison
  - Zoom controls
  - Toggle between before/after
  - Display file size difference
  - Update preview on settings change (with debounce/throttle)
- **Implementation Notes:**
  - Use QLabel with QPixmap for display
  - Consider QGraphicsView for zoom/pan
  - Generate preview using temporary gifsicle execution
  - Cache preview to avoid re-processing on every tweak
- **Verification Steps:**
  - [ ] Load preview of original image
  - [ ] Apply settings and update preview
  - [ ] Test zoom in/out functionality
  - [ ] Verify preview updates within 500ms of setting change
- **Estimated Effort:** 6-8 hours

### 8. No Progress Bar or Cancel Functionality
- **File:** `src/qtui/MainWindow.cpp`
- **Issue:** No visual progress indicator; no way to cancel long operations
- **Required UI Elements:**
  - QProgressBar for overall progress
  - Cancel button (enabled during processing)
  - Status label showing current file/operation
  - Estimated time remaining (optional)
- **Backend Requirements:**
  - Parse gifsicle stdout for progress hints
  - Support cancellation via `QProcess::kill()`
  - Clean up temp files on cancel
- **Verification Steps:**
  - [ ] Process batch of 10+ files with progress bar updating
  - [ ] Click cancel mid-batch → Operation stops gracefully
  - [ ] Verify no orphaned temp files after cancel
  - [ ] Test with very large file (>50MB)
- **Estimated Effort:** 3-4 hours

### 9. Queue Management Features Missing
- **File:** `src/qtui/MainWindow.cpp`
- **Issue:** No way to remove individual files or clear queue
- **Required Features:**
  - Remove selected file(s) from queue
  - Clear all button
  - Move up/down for ordering (if sequential processing matters)
  - Display total file count and combined size
- **UI Components:**
  - QListWidget or QTableView for file list
  - Context menu (right-click) with remove option
  - Toolbar buttons: Remove, Clear All, Move Up, Move Down
- **Verification Steps:**
  - [ ] Add 5 files to queue
  - [ ] Select and remove 2 files
  - [ ] Clear all remaining files
  - [ ] Verify file count and size display updates correctly
- **Estimated Effort:** 2-3 hours

### 10. Output Folder Actions Missing
- **File:** `src/qtui/MainWindow.cpp`
- **Issue:** No "Open Folder" or "Show in Finder" functionality
- **Required Actions:**
  - Button to open output folder in file manager
  - Option to use same folder as input
  - Option to use custom folder with browse dialog
  - Filename pattern templating (e.g., `{name}_optimized.{ext}`)
- **Platform-Specific Code:**
  ```cpp
  QDesktopServices::openUrl(QUrl::fromLocalFile(outputFolder));
  ```
- **Verification Steps:**
  - [ ] Set output folder and click "Open Folder" → File manager opens
  - [ ] Test on Linux (Nautilus/Dolphin), Windows (Explorer), macOS (Finder)
  - [ ] Verify filename patterns work correctly
- **Estimated Effort:** 1-2 hours

### 11. Version String Hardcoded in GUI
- **File:** `src/qtui/MainWindow.cpp` (line 24)
- **Issue:** Window title uses hardcoded `"Gifscythe 0.1.0"`
- **Impact:** Version must be updated in multiple places on release
- **Proposed Fix:**
  - Create `src/core/Version.h`:
    ```cpp
    #ifndef GIFSYCYTHE_CORE_VERSION_H
    #define GIFSYCYTHE_CORE_VERSION_H
    #define GIFSYCYTHE_VERSION "0.1.0"
    #endif
    ```
  - Include in MainWindow.cpp: `setWindowTitle(QStringLiteral("Gifscythe ") + QString(GIFSYCYTHE_VERSION));`
  - Sync with `VERSION.md` file via build script or manual update
- **Verification Steps:**
  - [ ] Change version in Version.h
  - [ ] Rebuild and confirm window title updates
  - [ ] Check about dialog (if exists) also uses central version
- **Estimated Effort:** 30 minutes

### 12. Packaging Scripts Untested
- **Files:** `scripts/package_portable.sh`, `scripts/package_system.sh`
- **Issue:** Scripts created but not verified on clean systems (SESSION_HANDOFF.md)
- **Risk:** May miss dependencies or fail on fresh installs
- **Verification Steps:**
  - [ ] Test `package_portable.sh` on clean Linux VM (no Qt/gifscythe installed)
  - [ ] Test `package_system.sh` on clean Linux VM
  - [ ] Test Windows packaging on clean Windows VM
  - [ ] Verify all required DLLs/.so files bundled
  - [ ] Test running packaged app on clean system
  - [ ] Document any missing dependencies found
- **Estimated Effort:** 4-6 hours (includes VM setup)

### 13. No Input Validation in SettingsIO
- **File:** `src/core/SettingsIO.h` / `SettingsIO.cpp`
- **Issue:** No validation of JSON config files; malformed input could crash
- **Required Validation:**
  - Check required fields present
  - Validate enum values in range
  - Validate numeric ranges (e.g., colors 2-256, lossy 0-200)
  - Handle missing optional fields gracefully
  - Provide meaningful error messages
- **Verification Steps:**
  - [ ] Create malformed JSON files (missing fields, wrong types, out-of-range values)
  - [ ] Attempt to load each and verify graceful error handling
  - [ ] Confirm no crashes or undefined behavior
  - [ ] Add unit tests for invalid inputs
- **Estimated Effort:** 3-4 hours

---

## 🟡 P2 - MEDIUM PRIORITY (Post v1.0.0)

### 14. No Logging Framework
- **Issue:** Debugging relies on printf/QMessageBox
- **Required:** Integrate logging library (spdlog, glog, or Qt's QLoggingCategory)
- **Features Needed:**
  - Log levels (DEBUG, INFO, WARN, ERROR)
  - File and console output
  - Configurable log level at runtime
  - Thread-safe logging
- **Verification Steps:**
  - [ ] Replace all printf/debug outputs with logging calls
  - [ ] Configure log rotation (max file size, backup count)
  - [ ] Test log output in normal and error scenarios
- **Estimated Effort:** 4-6 hours

### 15. No Configuration Persistence
- **Issue:** Settings reset when app closes
- **Required:** Save/load user preferences
- **Settings to Persist:**
  - Last used input/output folders
  - Recent files list
  - Window size/position
  - Default optimization settings
  - Preferred output format
- **Storage Options:**
  - QSettings (Qt standard, cross-platform)
  - JSON config file in user home directory
- **Verification Steps:**
  - [ ] Change settings and close app
  - [ ] Reopen app and verify settings restored
  - [ ] Test on all target platforms (Linux, Windows, macOS)
- **Estimated Effort:** 3-4 hours

### 16. No Internationalization (i18n) Support
- **Issue:** All UI strings hardcoded in English
- **Required:** Qt Linguist integration for translations
- **Steps:**
  - Wrap all user-visible strings with `tr()`
  - Generate .ts files for each language
  - Create translation files (.qm)
  - Add language selector to preferences
- **Target Languages:** English, Spanish, French, German, Japanese (based on PROJECT_VISION global audience)
- **Verification Steps:**
  - [ ] Extract all translatable strings
  - [ ] Create at least one additional language file
  - [ ] Test language switching at runtime
  - [ ] Verify no hardcoded strings remain in UI code
- **Estimated Effort:** 8-12 hours (initial setup + English extraction)

### 17. Common RGBA Animation Frame Model (Deferred from WORKLIST.md)
- **Files:** To be created in `src/core/`
- **Issue:** No unified frame representation for multi-format support
- **Purpose:** Enable future WebP/APNG integration and frame editing
- **Required Features:**
  - RGBA pixel buffer per frame
  - Frame metadata (delay, disposal, coordinates)
  - Animation metadata (loop count, canvas size)
  - Efficient memory management for large animations
- **Design Considerations:**
  - Avoid format-specific libraries in core model
  - Support lazy loading for large files
  - Thread-safe access for preview generation
- **Verification Steps:**
  - [ ] Define Frame and Animation classes
  - [ ] Implement GIF loader using existing gifsicle integration
  - [ ] Write unit tests for frame manipulation
  - [ ] Benchmark memory usage with 100+ frame animations
- **Estimated Effort:** 16-24 hours

---

## 🟢 P3 - LOW PRIORITY / ENHANCEMENTS

### 18. Magic Numbers in GUI
- **File:** `src/qtui/MainWindow.cpp`
- **Issue:** Hardcoded timeouts like `waitForStarted(3000)`, `waitForFinished(60000)`
- **Fix:** Define named constants:
  ```cpp
  constexpr int PROCESS_START_TIMEOUT_MS = 3000;
  constexpr int PROCESS_FINISH_TIMEOUT_MS = 60000;
  constexpr int PREVIEW_UPDATE_DEBOUNCE_MS = 500;
  ```
- **Estimated Effort:** 30 minutes

### 19. No Unit Tests for SettingsIO
- **Issue:** Only `GifsicleCommand` has unit tests
- **Required:** Test coverage for:
  - Valid JSON parsing
  - Invalid JSON handling
  - Round-trip serialization (struct → JSON → struct)
  - Default value handling
- **Estimated Effort:** 4-6 hours

### 20. No Benchmarking Suite
- **Issue:** No performance baseline for optimization operations
- **Required:** Benchmark suite measuring:
  - Command generation time
  - Processing time per file size
  - Memory usage peaks
  - GUI responsiveness under load
- **Tools:** Google Benchmark, Qt Test framework
- **Estimated Effort:** 6-8 hours

### 21. No Installer Creation
- **Issue:** Only portable packages implemented
- **Required:** Platform-native installers
  - Linux: .deb, .rpm, AppImage
  - Windows: .msi, Inno Setup
  - macOS: .dmg, Homebrew formula
- **Estimated Effort:** 12-16 hours

### 22. Animated WebP Integration (Deferred from WORKLIST.md)
- **Prerequisites:** Item #17 (Frame Model) must be complete
- **Required Libraries:** libwebp with AnimEncoder/AnimDecoder
- **Features:**
  - Load animated WebP files
  - Save to WebP format
  - Convert GIF ↔ WebP
  - Expose WebP-specific controls (quality, method, alpha quality)
- **Estimated Effort:** 24-32 hours

### 23. APNG Integration (Deferred from WORKLIST.md)
- **Prerequisites:** Item #17 (Frame Model) must be complete
- **Required Libraries:** libpng with APNG patches
- **Features:**
  - Load APNG files
  - Save to APNG format
  - Convert GIF ↔ APNG
  - Expose APNG-specific controls (compression level, filter type)
- **Estimated Effort:** 24-32 hours

### 24. Frame Editor Features (Deferred from WORKLIST.md)
- **Prerequisites:** Item #17 (Frame Model) must be complete
- **Features:**
  - Frame timeline view
  - Reorder frames (drag-drop)
  - Delete frames
  - Insert new frames
  - Duplicate frames
  - Adjust individual frame delays
  - Preview animation with frame selection
- **Estimated Effort:** 32-40 hours

### 25. Batch Rename Templates
- **Issue:** Basic output naming only
- **Required:** Advanced templating system
- **Template Variables:**
  - `{name}` - Original filename without extension
  - `{ext}` - Original extension
  - `{counter}` - Sequential number
  - `{date:YYYYMMDD}` - Current date
  - `{width}`, `{height}` - Output dimensions
  - `{filesize}` - Output file size
- **Estimated Effort:** 4-6 hours

### 26. Preset System
- **Issue:** No way to save/load favorite setting combinations
- **Required:** Preset management
- **Features:**
  - Save current settings as named preset
  - Load preset from dropdown
  - Delete/rename presets
  - Import/export preset files
  - Pre-built presets: "Web Optimized", "Lossless", "Small Size", "High Quality"
- **Storage:** JSON files in user config directory
- **Estimated Effort:** 6-8 hours

### 27. System Tray Integration
- **Feature:** Minimize to system tray for long batch operations
- **Notifications:** Toast notification when batch completes
- **Platform Support:** Linux (libappindicator), Windows (QSystemTrayIcon), macOS (native)
- **Estimated Effort:** 3-4 hours

### 28. Keyboard Shortcuts
- **Issue:** No keyboard navigation
- **Required Shortcuts:**
  - Ctrl+O: Open files
  - Ctrl+S: Start processing
  - Ctrl+R: Remove selected
  - Ctrl+A: Select all
  - Ctrl+Z: Undo (if undo system implemented)
  - F5: Refresh preview
  - Esc: Cancel operation
- **Implementation:** QShortcut or QAction with shortcuts
- **Estimated Effort:** 2-3 hours

### 29. Dark Mode Support
- **Issue:** No theme switching
- **Required:** Light/dark mode toggle
- **Implementation:**
  - Qt palette customization
  - QSS (Qt Style Sheets) for dark theme
  - Auto-detect system theme preference
- **Estimated Effort:** 4-6 hours

### 30. Crash Reporting (Optional)
- **Feature:** Optional anonymous crash reporting
- **Tools:** Breakpad, Crashpad, or Qt-based solution
- **Privacy:** Must be opt-in with clear disclosure
- **Estimated Effort:** 8-12 hours

---

## 📝 VERIFICATION CHECKLIST FOR ALL ITEMS

For each item above, complete verification should include:

- [ ] **Code Review:** Changes reviewed by second developer (or self-review checklist)
- [ ] **Unit Tests:** Automated tests covering new functionality
- [ ] **Manual Testing:** Tested on all target platforms (Linux, Windows, macOS)
- [ ] **Documentation:** User docs and developer docs updated
- [ ] **Performance:** No significant regression in speed/memory
- [ ] **Accessibility:** Keyboard navigation, screen reader compatibility (where applicable)
- [ ] **Localization:** Strings wrapped for translation (if UI-facing)
- [ ] **Error Handling:** Graceful failure modes tested
- [ ] **Edge Cases:** Large files, empty inputs, special characters in paths tested

---

## 🎯 NEXT STEPS

1. **Prioritize:** Review this list and adjust priorities based on your goals
2. **Estimate:** Add time estimates if different from suggestions above
3. **Assign:** If working with team, assign items to developers
4. **Track:** Move items to separate IMPLEMENTED.md as completed
5. **Verify:** Don't mark complete until all verification steps pass

---

## 📌 NOTES

- This worklist was generated from comprehensive codebase review
- All findings should be independently verified before implementation
- Some items may be dependent on others (see prerequisites noted)
- Time estimates are approximate and assume single developer
- Priority ratings may change based on user feedback or business needs
- Items marked "Deferred" in original WORKLIST.md are placed in P2/P3 accordingly

**Remember from PROJECT_VISION.md:** Do NOT start WebP/APNG work (items 22-24) until the core GIF UI is complete and stable.
