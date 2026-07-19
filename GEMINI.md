# Gemini Agent Instructions

## 🤖 Role and Persona
You are an Expert C++ Developer and Software Architect. Your task is to assist in building "CantaTema", a modern educational desktop/terminal application designed to support students preparing for competitive examinations and academic assessments. The platform allows users to practice speaking topics aloud, records their voice using Opus compression and XOR encryption, and compares their spoken explanations against reference texts (loaded from PDFs via MuPDF) using locally run Whisper.cpp models.

## 🛠 Tech Stack
- **Language:** C++23 (configured with standard `-std=c++23`). For MSVC, the `/utf-8` compile option is forced to ensure proper Spanish/European character support. Unicode wide-character definitions (`UNICODE`, `_UNICODE`, `_WCHAR_T`) are defined.
- **Build System:** CMake (minimum version 3.20).
- **Database:** SQLite (integrated via custom cmake wrapper `add_sqlite3_library`).
- **Audio Engine & Libraries:**
  - **SDL3:** (v3.4.0) Used for the audio recording and playback system, including device queries and stream handling.
  - **Opus:** (v1.5.2) Used for high-quality audio compression.
  - **TagLib:** (v2.1.1) and **utf8cpp** (v4.0.9) for audio metadata tagging and UTF-8 handling.
- **Text & PDF Processing:**
  - **MuPDF:** Used to parse reference materials in PDF format and extract raw text for comparison.
- **Speech Recognition & ML:**
  - **Whisper.cpp:** (v1.8.3) For offline, local speech-to-text transcription. Built with Vulkan acceleration for Windows/Linux and Metal acceleration for Apple systems.
  - **libcurl:** (curl-8_18_0) Minimal/HTTP-only build used to verify model availability and download GGML model weights (`ggml-*.bin`) directly from Hugging Face.
- **Configuration:**
  - **SimpleIni:** (v4.25) Header-only library used to parse and manage the `system.ini` configuration.
- **Utilities & Logging:**
  - **fmt:** (v12.1.0) and **spdlog** (v1.16.0) for efficient, modern string formatting and console/file logging.
  - **cli:** (v2.2.0) with `asio` backend for building interactive command-line menus.
- **Testing:**
  - **GoogleTest (gtest/gmock):** (v1.17.0) with tests registered under CTest via a custom helper (`create_test_exec`).

## 📂 Target Project Structure
The project is modularized into discrete CMake components using a custom `create_infra_library` helper. The architecture is divided into the following layout:
- `CantaTema/apps/`: Contains executable targets.
  - `terminal/`: Core terminal console application handling terminal CLI loop, session, record loops, and speech commands.
- `CantaTema/components/core/`: Basic primitives, configurations, and models.
  - `primitives/`: Domain models (`User`, `Category`, `Subject`, `PracticeEvent`), logging utilities (`utils_logger`), thread pools (`utils_thread_pool`), and paths manager (`tool_paths`).
  - `configuration/`: `ConfigurationSystem` wrapper loaded from `system.ini` which sets default limits (e.g. max sound/text file size, default allowed extensions).
  - `models/`: `ManagerWhisper` used to check availability and download models from Hugging Face.
- `CantaTema/components/business/`: Pure business logic.
  - `operations/`: Decoupled entities operation classes (`operation_user`, `operation_category`, `operation_subject`, `operation_user_metrics`, `operation_practice_event`) which coordinate filesystem and database handlers.
  - `session/`: Facade class `Session` representing the logged-in user state and acting as the main interface to the business operation layers.
- `CantaTema/components/infrastructure/`: Subsystem integrations.
  - `database/`: Repository classes using SQLite to save/retrieve user metrics, categories, subjects, and practice events.
  - `file_handler/`: Helper classes for standard filesystem interactions, extension type parsing, sound metadata checking, and PDF/Text reading.
  - `sound_system/`: SDL3 audio playback and recording wrapped in `SoundSystem` class using Opus codec and custom XOR-encryption logic.
  - `speech_recognition/`: Transcription interface `ISpeechRecognition` to integrate local Whisper-based decoding.
- `docs/`: Diagram drafts, notes, and schemas.
- `cmake/`: Installation templates and FetchContent wrappers (e.g., `add_sqlite3_library.cmake`, `library_install_test.cmake`).

## 🏗 Architectural Guidelines
- **Strict Separation of Concerns:** Core domain models must be kept clean in `core/primitives`. Database operations must be isolated inside `infrastructure/database`. Do not import database header files directly into business operations; always interact via interfaces or repositories.
- **Modular Component Isolation:** Every component must be built as a distinct target (e.g., `CORE::PRIMITIVES`, `INFRASTRUCTURE::DATABASE`) by invoking the custom `create_infra_library` macro in its `CMakeLists.txt`.
- **Facade Pattern:** The `Session` class acts as the single facade to hold session context (the logged-in user) and route calls to specific business operations.
- **Filesystem Base Paths:** The system calculates storage paths via `ToolPath::get_base_path()`. In debug/non-release modes, this points to the current directory (`std::filesystem::current_path()`). In release mode (`NDEBUG` defined), it resolves to the OS-specific application directory using `SDL_GetPrefPath`. Always resolve resource and database paths through `ToolPath` functions rather than hardcoded string parameters.

## 🔊 Audio Processing & Cryptography (sound_system/)
- **Audio Format & Container:** Captured audio is encoded to Opus and structured within an Ogg stream container.
- **XOR Encryption Overlay:** For security and privacy, audio files are saved with a custom byte-level XOR masking scheme. 
- **Encryption Implementation:** Any disk writing or reading in `SoundSystem` must pass through the `xor_process` method using `secure_fwrite` and `secure_fread`. Do not call standard `std::fwrite` or `std::fread` directly when writing or reading Opus files to disk, as they will bypass encryption/decryption, causing corruption.
- **Audio Timestamps:** Monitor recording and playback progress in milliseconds using `get_recording_timestamp()` and `get_playing_timestamp()` respectively.

## 🤖 Whisper Speech Recognition & Model Management (models/ & speech_recognition/)
- **Whisper Models:** Models are stored in `ToolPath::get_path_for_models_whisper()`.
- **Hugging Face Downloader:** `ManagerWhisper` retrieves available Whisper model manifests and downloads model binaries (`ggml-*.bin`) directly from Hugging Face (`https://huggingface.co/ggerganov/whisper.cpp`).
- **Non-blocking Downloads:** Network downloads use libcurl callback streams. Callers can supply a `DownloadProgressCallback` to receive updates (`total_bytes`, `downloaded_bytes`, `file_name`) to display progressive loading indicators.
- **Speech Recognition Interface:** The `ISpeechRecognition` interface handles the transcription tasks. Implementing classes must accept an audio path, manage task states (`IDLE`, `PROCESSING`, `COMPLETED`, `ERROR`), and save the resulting text.

## 📏 Coding Standards & Quality
- **C++ Standards Compliance:** All code must conform strictly to standard C++23. Do not use compiler-specific language extensions unless wrapped in preprocessor tags.
- **Resource Management:** Prefer smart pointers (`std::unique_ptr` and `std::shared_ptr`) to enforce RAII. Avoid manual calls to `delete` or `free` unless dealing directly with C library pointers (like SDL3 context structures or curl pointers) that require specialized cleaners.
- **Doxygen Documentation:** All classes, public methods, and functions must be documented with descriptive Doxygen-style blocks.
- **Logger Usage:** System logs must write to the unified `logger` instance (configured via `utils_logger.hpp`), categorized by log severity levels (`info`, `debug`, `warn`, `error`).
- **Testing Requirements:** Every newly added or modified logic must have comprehensive unit tests written with the GoogleTest framework, placed under the appropriate component's `test` folder. Mocks (under `mocks` folder) should be used to isolate classes under test.
  - **Code Coverage:** Code coverage is enabled using GCC/Clang `--coverage` options.
    - **Enforcement:** Production source files (`.h`, `.hpp`, `.c`, `.cpp` containing production logic, excluding tests, mocks, and external dependencies) must maintain at least **90% coverage**.
    - **Execution Commands:**
      ```powershell
      cmake -B build -DENABLE_COVERAGE=ON
      cmake --build build --config Debug
      ctest --test-dir build -C Debug -T coverage
      ```

## 💻 Terminal CLI Interaction & Commands
To run the interactive shell of the CantaTema project:
1. Ensure the project is compiled: `cmake --build build --config Debug`
2. Run the executable: `.\build\bin\Terminal_CLI\Terminal_CLI.exe`
3. Execute CLI commands directly. Commands are organized into menus:
   - **Authentication / Initialization Flow:**
     - `test_start`: Purge database and populate it with test categories, subjects, and users.
     - `db_purge`: Completely clean database records.
     - `user user_add <username> <password>`: Create a new user account.
     - `user_identify <username> <password>`: Authenticate as a user and initialize the active user session.
     - `user user_get`: Display details of the active user.
     - `metrics`: Retrieve metrics of the authenticated user.
   - **Category Management (`category` menu):**
     - `category category_add <name>`: Create a new study category.
     - `category category_get`: View all categories registered under the active user.
   - **Subject Management (`subject` menu):**
     - `subject subject_add <category_id> <name>`: Register a new study subject under a category.
     - `subject subject_get_by_user`: Get all subjects of the current user.
   - **Practice & Recording Loop (`practice` menu):**
     - `practice add_recorded <subject_id> <name>`: Register a practice session and trigger SDL3 audio capture.
     - `practice play <practice_id>`: Decrypt (XOR) and play back the recorded practice event using SDL3 playback streams.
   - **Whisper Models (`whisper` menu):**
     - `whisper models`: Check locally available and download-ready GGML models.
     - `whisper download <model_name>`: Fetch the specified GGML model weight from Hugging Face.

## 🛑 Agent Operational Rules (CRITICAL)
1. **No Hallucinations:** Do not fabricate libraries, guess headers, or invent dummy methods to bypass compilation failures. If an external API is unknown, check the official documentation or look at similar implementations in the codebase.
2. **Ask for Clarification:** If a design decision or project requirement is ambiguous, stop and ask the user for clarification.
3. **Minimal Modifications:** Change only what is strictly necessary to solve a bug or implement a feature. Avoid large formatting rewrites that make code reviews difficult.
4. **Verifying Code Correctness:** Every modification must compile and pass all tests. Run the test suite by executing:
   ```powershell
   cmake -B build
   cmake --build build --config Debug
   ctest --test-dir build -C Debug
   ```
5. **Temporary Scripts:** Create any test scripts or debug binaries inside a `scratch/` folder at the root directory of the workspace.

## 📝 Documentation Guidelines
- **Interface & Schema Changes:** If database schemas, configuration files, or core component interfaces change, update the documentation in the `docs` folder or add relevant annotations.

## 🗺 Development Plan

### Phase 1: Core Primitives & Logging Setup
- **1.1 Logging Utility:** Integrate `fmt` and `spdlog` for unified file and console logging.
- **1.2 Domain Entities:** Define structured classes for `User`, `Category`, `Subject`, `PracticeEvent`, and `UserMetrics`.
- **1.3 Storage Paths:** Set up the `ToolPath` utility for directory resolution.

### Phase 2: Configuration & Model Downloads
- **2.1 Configuration System:** Initialize `simpleini` to create and load `system.ini` with custom text/sound size and extensions parameters.
- **2.2 Models Manager:** Build the `ManagerWhisper` network downloader to fetch GGML model weights using libcurl progress callbacks.

### Phase 3: Infrastructure (Database & File Handling)
- **3.1 SQLite Database:** Setup `DB_Main` repository classes to store users, subjects, categories, practice logs, and metrics.
- **3.2 File Handler:** Implement extension checkers, PDF raw text extractor (MuPDF), and generic sound parameters validation.

### Phase 4: Audio Engine (Sound System)
- **4.1 SDL3 Audio integration:** Setup capture and playback streams with custom Opus encoding parameters.
- **4.2 Encrypted Storage:** Implement custom Ogg container writing with inline byte-level XOR masking.

### Phase 5: Business Operations & Session Facade
- **5.1 Business Operations:** Implement category, subject, user, and practice operations to link DB layer with filesystem.
- **5.2 State Facade:** Build the `Session` facade to handle user logins and state transitions.

### Phase 6: CLI Terminal Menu Interface
- **6.1 CLI Shell:** Use `cli::cli` with Asio to set up menus for user login, registration, and subject management.
- **6.2 CLI Practice Loop:** Create options for recording practice sessions, viewing history, and checking file limits.

### Phase 7: Whisper Integration & Validation
- **7.1 Speech Recognition Engine:** Implement `ISpeechRecognition` using the Whisper.cpp engine.
- **7.2 Transcription Flow:** Link the recording output with the Whisper transcription task.
- **7.3 Comparison Logic:** Build the text comparison algorithm matching user speech against PDF reference text.

### Phase 8: Optimization & Final Polish
- **8.1 Multi-threading:** Move speech transcription to background thread pools.
- **8.2 Verification:** Perform memory leak checking, compile tests on Windows/Linux, and run end-to-end user flows.
