# CantaTema - Developer Guide

Welcome to the development environment for **CantaTema**. This document guides you through configuring, compiling, testing, analyzing code coverage, and executing the interactive CLI application.

---

## 📋 Prerequisites

Before starting, ensure you have the following installed on your system:
- **Compiler:** GCC 13+, Clang 16+, or MSVC 2022 (Must support standard **C++23**).
- **Build Tool:** CMake (version 3.20 or higher).
- **Libraries & SDKs:**
  - **Windows (MinGW/MSYS2):** Recommended toolchain is `ucrt64` which provides `gcc`/`g++`, `cmake`, `pkg-config`, and required audio/networking backends.
  - **Vulkan SDK:** (Optional, but recommended on Windows/Linux for Whisper.cpp hardware acceleration).

---

## ⚙️ 1. Project Configuration

Configure the build system using CMake. Dependencies are fetched and compiled automatically via CMake's `FetchContent` mechanism.

### Standard Configuration
By default, this configures the project to build static binaries with testing enabled:
```powershell
cmake -B build
```

### Configuration with Coverage Enabled
To instrument binaries for code coverage analysis:
```powershell
cmake -B build -DENABLE_COVERAGE=ON
```

---

## 🛠️ 2. Compilation

Compile all targets, including the primary application and unit tests.

### Compile Debug Build (Default)
```powershell
cmake --build build --config Debug
```

### Compile Release Build (Optimized)
```powershell
cmake --build build --config Release
```

---

## 🧪 3. Running Unit Tests

CantaTema uses **GoogleTest** (gtest/gmock) for testing. Tests are registered under **CTest**.

### Execute All Tests
```powershell
ctest --test-dir build -C Debug
```

### Run Tests with Detailed Output
If a test fails and you need to inspect logs:
```powershell
ctest --test-dir build -C Debug -V
```

---

## 📊 4. Code Coverage Analysis

Code coverage requires compiling with coverage flags enabled (`-DENABLE_COVERAGE=ON`) and executing tests with CTest's coverage runner.

### Code Coverage Enforcement
- **Standard:** All production source files (`.h`, `.hpp`, `.c`, and `.cpp` containing logic, excluding tests, mocks, or external packages under `build/_deps`) must maintain at least **90% coverage**.

### Generate Coverage Reports
Run the following sequential commands:
1. Re-configure the project with coverage active:
   ```powershell
   cmake -B build -DENABLE_COVERAGE=ON
   ```
2. Clean previous build objects to force complete coverage instrumentation:
   ```powershell
   cmake --build build --target clean
   ```
3. Compile the targets:
   ```powershell
   cmake --build build --config Debug
   ```
4. Run tests and process coverage data using `gcov` (Note: Run this command from the **workspace root**):
   ```powershell
   ctest --test-dir build -C Debug -T coverage
   ```

*The aggregated coverage files will be located in the `build/Testing/` directory.*

---

## 💻 5. Executing the Tool

To run the interactive console shell application:

```powershell
.\build\bin\Terminal_CLI\Terminal_CLI.exe
```

### Recommended First-Time Execution Flow

1. **Populate Test Data:**
   Purge the local database and populate categories, subjects, and test users:
   ```text
   cli> test_start
   ```
2. **Authenticate Session:**
   Identify/login to start your practice session (creates local workspace directories):
   ```text
   cli> user_identify iscapla iscapla
   ```
3. **Register/Check Whisper Models:**
   See available models locally and on the Hugging Face repository:
   ```text
   cli> whisper models
   ```
   Download a model (e.g. `tiny`) to begin transcribing:
   ```text
   cli> whisper download tiny
   ```
4. **Recording/Playback Practice:**
   Enter the `practice` menu to start SDL3 recording:
   ```text
   cli> practice add_recorded 1 my_session
   cli> practice play 1
   ```

---

## 🔌 6. IDE Integration (VS Code / Antigravity)

To make the graphical **"Run Test with Coverage"** button work directly within the IDE:
1. Ensure your `.vscode/settings.json` includes the path to `gcov.exe`:
   ```json
   {
       "cmake.gcovpath": "C:\\msys64\\ucrt64\\bin\\gcov.exe"
   }
   ```
2. Trigger **CMake: Clean Configure** from the VS Code command palette (`Ctrl + Shift + P`) to reload the configuration.

---

## 📊 7. Generating Visual Coverage Reports (gcovr)

To generate visual HTML report details:
1. **Install gcovr:**
   - **Via MSYS2 UCRT64 Package Manager:**
     ```bash
     pacman -S mingw-w64-ucrt-x86_64-gcovr
     ```
   - **Via Python Pip (Universal):**
     ```bash
     pip install gcovr
     ```
2. **Generate HTML Report:**
   Run this at the workspace root (use the absolute path `/ucrt64/bin/gcovr.exe` if `gcovr` is not in your environment PATH):
   ```bash
   /ucrt64/bin/gcovr.exe -r . --filter CantaTema/ --html-details -o build/coverage.html
   ```
3. Open `build/coverage.html` in your browser.
