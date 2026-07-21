# CantaTema - Comprehensive Implementation Roadmap & Phases

This document details the multi-phase implementation roadmap for **CantaTema**. Requirements have been extracted from `README.md`, `GEMINI.md`, and `docs/dev.md`.

To ensure seamless execution by secondary AI models or developers, the project is divided into **9 main phases** and **21 self-contained subphases**. Each subphase includes explicit architectural layers, target components, technical specifications, error-handling edge cases, and strict **TDD/unit testing requirements (maintaining >= 90% file code coverage alongside code implementation)**.

---

## ❓ Frequently Asked Questions & Core Concepts

### Q1: When is whisper.cpp called to convert audio to text?
- `ISpeechRecognition` / `WhisperSpeechRecognition` (under `infrastructure/speech_recognition`) is invoked during the **Coverage Analysis pipeline** (`OperationCoverage`).
- When the user triggers `coverage analyze <practice_id>` (or via `Session::analyze_practice_coverage`), the system retrieves the practice audio (`.opus`), decrypts & decodes it using `ISoundSystem` / `SoundHandler` into 16kHz mono PCM samples, passes the PCM buffer to `ISpeechRecognition::submit_task(audio_file_path)` (which executes whisper.cpp decoding using the language inherited from the parent `Subject`), and retrieves structured transcript chunks with timestamps and confidence scores.

### Q2: Are we converting audio to PDF? Why not compare plain text?
- **No, we do not convert audio to PDF.**
- The system compares **plain text vs. plain text** (specifically, their semantic vector embeddings):
  1. Spoken audio (`.opus`) → Transcribed into **Spoken Text** via whisper.cpp.
  2. Reference material (`.pdf` / `.txt`) → Extracted into **Reference Text & Sentence Chunks** via MuPDF / `TextFileHandler`.
  3. Spoken Text & Reference Text → Converted into **Vector Embeddings** via llama.cpp (`multilingual-e5-large`).
  4. Nearest-neighbor similarity search (Faiss) compares the **spoken text chunks against reference text chunks**.
- PDF is simply the document format used to store official exam study material.

### Q3: How does Automatic Model Selection (`AUTO`) and Per-Execution Model Overriding work?
- **Config Setting `AUTO`:** When `system.ini` keys (`[WHISPER] default_model` or `[EMBEDDINGS] default_model`) are set to `"AUTO"` (or `"auto"`), `ManagerModels` automatically inspects locally available models on disk and system hardware, selecting the optimal model (e.g. preferring `small` if present, falling back to `tiny`).
- **Per-Execution Override:** Model selection and parameters are passed on execution calls (`OperationCoverage::analyze_practice_coverage(practice_id, whisper_model, llama_model, similarity_threshold, language)`). If specified, they override the default/AUTO INI configuration for that single execution run.

### Q4: How is language and reference file linked to a Subject?
- **Language & Reference File Inheritance:** Every `PracticeEvent` (audio practice) belongs to a `Subject`. The `Subject` primitive holds both the reference document `filepath` and the study `language` (e.g. `"es"`, `"en"`).
- When running `coverage analyze <practice_id>`, the audio automatically inherits the language and reference document from its parent `Subject`. Language can also be overridden per execution.

### Q5: How is Analysis History and Configuration Snapshots stored?
- Every analysis run creates a **new Analysis Execution Record** with a unique `analysis_execution_id`.
- The database stores a historic log of all analysis executions per practice session, including a complete JSON snapshot of the execution parameters (models used, similarity threshold, language, importance weights, coverage %, speed, clarity, and pacing scores).

---

## 🏗️ Architectural Overview & Layering

```
┌─────────────────────────────────────────────┐
│              CLI / Terminal App              │  ← Top layer (apps/terminal/)
├─────────────────────────────────────────────┤
│                  Session                    │  ← Facade for business logic
├─────────────────────────────────────────────┤
│                Operations                   │  ← Business rules & coordination
├─────────────────────────────────────────────┤
│              Infrastructure                 │  ← Database, Sound, Files, Speech,
│                                             │    Embeddings, Similarity
├─────────────────────────────────────────────┤
│          Core (shared foundation)           │  ← Primitives, Configuration, Models
└─────────────────────────────────────────────┘
```

> **Testing Policy:** Every subphase MUST implement unit tests (GoogleTest/GMock) alongside production code changes. No subphase is complete until production source files maintain **>= 90% code coverage**.

---

## Phase 1: Foundation, Database & Core Configuration Upgrades

### Subphase 1.1: Configuration System Extension (`system.ini`)
* **Goal:** Expand system configuration settings to support text embedding models, similarity search, importance weighting parameters, PDF page limits, hard recording duration limits, and `AUTO` model selection modes.
* **Layer:** `Core` (`CantaTema/components/core/configuration`)
* **Files to Modify:**
  - `CantaTema/components/core/configuration/include/configuration/configuration_system.hpp`
  - `CantaTema/components/core/configuration/src/configuration_system.cpp`
  - `CantaTema/components/core/configuration/test/test_configuration_system.cpp`
  - `system.ini` (workspace root)
* **Technical Details:**
  - Add new sections and keys to `system.ini`:
    - `[WHISPER]`: `default_model` (string, default `"AUTO"`).
    - `[EMBEDDINGS]`: `default_model` (string, default `"AUTO"`), `gpu_offload_layers` (int, default `0`).
    - `[COVERAGE]`: `similarity_threshold` (float, default `0.75`), `importance_weight_bold` (float, default `1.5`), `importance_weight_italic` (float, default `1.2`), `importance_weight_underline` (float, default `1.3`), `importance_weight_bg_color` (float, default `1.4`).
    - `[USER_LIMITS]`: `max_pdf_page_count` (int, default `100`), `max_recording_duration_minutes` (int, default `30`).
  - Update `ConfigurationSystem` getter methods to read and validate these parameters with safe fallbacks.
* **Testing & Coverage Requirements:**
  - Write unit tests in `test_configuration_system.cpp` for loading new INI sections and handling default fallbacks.
  - Maintain **>= 90% code coverage** on `configuration_system.cpp`.

---

### Subphase 1.2: Unified Model Manager & Automatic Model Selection (`ManagerModels`)
* **Goal:** Implement `ManagerModels` supporting both Whisper speech models (`ggml-*.bin`) and llama.cpp embedding models (`*.gguf`), including automatic model resolution (`AUTO`/`auto`), partial download cleanup, and Hugging Face downloads.
* **Layer:** `Core` (`CantaTema/components/core/models` and `CantaTema/components/core/primitives`)
* **Files to Create / Modify / Delete:**
  - **[NEW]** `CantaTema/components/core/models/include/models/manager_models.hpp`
  - **[NEW]** `CantaTema/components/core/models/src/manager_models.cpp`
  - **[NEW]** `CantaTema/components/core/models/test/test_manager_models.cpp`
  - **[DELETE]** `CantaTema/components/core/models/include/models/manager_whisper.hpp`
  - **[DELETE]** `CantaTema/components/core/models/src/manager_whisper.cpp`
  - **[MODIFY]** `CantaTema/components/core/primitives/include/primitives/tool_paths.hpp`
  - **[MODIFY]** `CantaTema/components/core/primitives/src/tool_paths.cpp`
* **Technical Details:**
  - Add `ToolPath::get_path_for_models_llama()` alongside existing Whisper model path functions.
  - Implement `auto_select_whisper_model()` and `auto_select_llama_model()` methods:
    - If config is `"AUTO"` (case-insensitive), scan local storage (`models/whisper` and `models/llama`).
    - Select optimal locally downloaded model (preference order: `small` -> `base` -> `tiny`). If no local model exists, return default downloadable target name.
  - Support download tracking via `DownloadProgressCallback` (`std::function<void(size_t total_bytes, size_t downloaded_bytes, const std::string& file_name)>`).
  - Implement cleanup for partial/corrupted downloads if transfer is interrupted.
* **Testing & Coverage Requirements:**
  - Write unit tests in `test_manager_models.cpp` covering model enumeration, `AUTO` resolution logic, path generation, error cleanup, and mock download streams.
  - Maintain **>= 90% code coverage** on `manager_models.cpp` and `tool_paths.cpp`.

---

### Subphase 1.3: Subject Primitive Language Field & Database Schema Extension (`IDatabase`)
* **Goal:** Extend `Subject` primitive with language field, extend SQLite database schema with `analysis_executions` table for historic report logging and config snapshots, and expose methods via `IDatabase`.
* **Layer:** `Core` & `Infrastructure` (`primitives`, `database`)
* **Files to Create / Modify:**
  - **[MODIFY]** `CantaTema/components/core/primitives/include/primitives/subject.hpp`
  - **[MODIFY]** `CantaTema/components/core/primitives/src/subject.cpp`
  - **[NEW]** `CantaTema/components/infrastructure/database/include/database/i_database.hpp`
  - **[NEW]** `CantaTema/components/infrastructure/database/include/database/db_coverage.hpp`
  - **[NEW]** `CantaTema/components/infrastructure/database/src/db_coverage.cpp`
  - **[NEW]** `CantaTema/components/infrastructure/database/mocks/mock_database.hpp`
  - **[MODIFY]** `CantaTema/components/infrastructure/database/src/db_subject.cpp`
  - **[MODIFY]** `CantaTema/components/infrastructure/database/src/db_main.cpp`
  - **[MODIFY]** `CantaTema/components/infrastructure/database/test/CMakeLists.txt`
* **Technical Details:**
  - `Subject` Primitive Update: Add `std::string language = "es";` with `get_language()` and `set_language()`. Update `subjects` DB table schema to add `language TEXT NOT NULL DEFAULT 'es'`.
  - Database Table `analysis_executions` Schema:
    `id INTEGER PRIMARY KEY AUTOINCREMENT`,
    `practice_id INTEGER NOT NULL`,
    `analysis_execution_id TEXT NOT NULL`,
    `coverage_percentage REAL`,
    `speed_score REAL`,
    `clarity_score REAL`,
    `pacing_score REAL`,
    `whisper_model TEXT`,
    `llama_model TEXT`,
    `language TEXT`,
    `similarity_threshold REAL`,
    `config_snapshot_json TEXT`,
    `report_json TEXT`,
    `created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP`
  - Expose `IDatabase` interface CRUD methods for persisting new analysis execution runs and querying execution history lists per `practice_id`.
* **Testing & Coverage Requirements:**
  - Write unit tests in `t_db_coverage.cxx` and `t_db_subject.cxx` verifying table migrations, language fields, historic execution insertion, and JSON config snapshot queries.
  - Maintain **>= 90% code coverage** on `db_coverage.cpp` and `db_subject.cpp`.

---

## Phase 2: PDF Reference Ingestion, Formatting Weights & Page Limits

### Subphase 2.1: Review Existing MuPDF Extraction & Formatting Integration
* **Goal:** Review and leverage existing rich text extraction (`TextFileHandler`, `IExtensionType`, `TextSpan` with `is_bold`, `is_italic`, `font_size`, `text_color`, `is_highlighted`, `highlight_color`) already implemented in `infrastructure/file_handler`, ensuring clean sentence-level text aggregation across spans.
* **Layer:** `Infrastructure` (`CantaTema/components/infrastructure/file_handler`)
* **Files to Modify / Audit:**
  - `CantaTema/components/infrastructure/file_handler/include/file_handler/text_handler.hpp`
  - `CantaTema/components/infrastructure/file_handler/src/text_handler.cpp`
  - `CantaTema/components/infrastructure/file_handler/include/file_handler/i_extension_type.hpp`
  - `CantaTema/components/infrastructure/file_handler/test/test_text_handler.cpp`
* **Technical Details:**
  - Verify existing `extract_rich_text()`, `find_bold()`, `find_italic()`, and `find_highlight()` implementations in `TextFileHandler`.
  - Ensure MuPDF text span extraction returns complete formatting metadata for downstream chunk weighting.
* **Testing & Coverage Requirements:**
  - Expand `test_text_handler.cpp` to validate rich text extraction on sample PDFs containing bold/italic/highlighted terms.
  - Maintain **>= 90% code coverage** on `text_handler.cpp`.

---

### Subphase 2.2: PDF Page Limit Enforcement, Sentence Chunking & Formatting Weights
* **Goal:** Create a PDF chunk extractor component that enforces max PDF page limits from `system.ini`, converts extracted rich text spans into sentence chunks with computed importance weights, and handles empty PDFs cleanly.
* **Layer:** `Infrastructure` (`CantaTema/components/infrastructure/file_handler`)
* **Files to Create:**
  - **[NEW]** `CantaTema/components/infrastructure/file_handler/include/file_handler/pdf_chunk_extractor.hpp`
  - **[NEW]** `CantaTema/components/infrastructure/file_handler/src/pdf_chunk_extractor.cpp`
  - **[NEW]** `CantaTema/components/infrastructure/file_handler/test/test_pdf_chunk_extractor.cpp`
* **Technical Details:**
  - PDF Page Limit Enforcement: Check `get_number_of_pages()`. If page count exceeds `max_pdf_page_count` from `system.ini`, return error `RST_ERR_EXCEEDS_PAGE_LIMIT`.
  - Define `PdfChunk` struct containing: `chunk_id`, `text`, `sentence_index`, `importance_weight`, `is_bold`, `is_italic`, `is_underlined`, `has_bg_color`.
  - Sentence boundary parsing: split on sentence boundaries (`. `, `? `, `! `, `\n`) while keeping headings and bullet points intact.
  - Calculate `importance_weight = base_weight * bold_mult * italic_mult * underline_mult * bg_color_mult` using multipliers from `system.ini`.
  - Handle edge cases: If extracted text is empty or image-only without selectable text, return error code `RST_ERR_EMPTY_FILE`.
* **Testing & Coverage Requirements:**
  - Write unit tests in `test_pdf_chunk_extractor.cpp` for page limit error triggers, sentence boundary splitting, formatting flag aggregation, weight calculation logic, and empty PDF handling.
  - Maintain **>= 90% code coverage** on `pdf_chunk_extractor.cpp`.

---

## Phase 3: Text Embedding Subsystem (`infrastructure/embeddings`)

### Subphase 3.1: Interface, CMake Setup & Mocking
* **Goal:** Define the `IEmbeddingEngine` abstract interface, set up the `INFRASTRUCTURE::EMBEDDINGS` CMake target, and build unit test mocks.
* **Layer:** `Infrastructure` (`CantaTema/components/infrastructure/embeddings`)
* **Files to Create:**
  - **[NEW]** `CantaTema/components/infrastructure/embeddings/CMakeLists.txt`
  - **[NEW]** `CantaTema/components/infrastructure/embeddings/include/embeddings/i_embedding_engine.hpp`
  - **[NEW]** `CantaTema/components/infrastructure/embeddings/mocks/mock_embedding_engine.hpp`
* **Technical Details:**
  - Declare `IEmbeddingEngine` interface:
    - `virtual bool load_model(const std::filesystem::path& model_path) = 0;`
    - `virtual std::vector<float> generate_embedding(const std::string& text) = 0;`
    - `virtual std::vector<std::vector<float>> generate_embeddings_batch(const std::vector<std::string>& texts) = 0;`
    - `virtual size_t get_embedding_dimension() const = 0;`
  - Implement GMock `MockEmbeddingEngine` for testing upper layers.
* **Testing & Coverage Requirements:**
  - Verify CMake target compiles and mock interface integrates cleanly with GoogleTest.

---

### Subphase 3.2: llama.cpp Embedding Engine Implementation & Unit Tests
* **Goal:** Implement `LlamaEmbeddingEngine` wrapping llama.cpp in embedding mode for vectorizing text chunks using `multilingual-e5-large` in GGUF format, along with comprehensive unit tests.
* **Layer:** `Infrastructure` (`CantaTema/components/infrastructure/embeddings`)
* **Files to Create:**
  - **[NEW]** `CantaTema/components/infrastructure/embeddings/include/embeddings/llama_embedding_engine.hpp`
  - **[NEW]** `CantaTema/components/infrastructure/embeddings/src/llama_embedding_engine.cpp`
  - **[NEW]** `CantaTema/components/infrastructure/embeddings/test/test_llama_embedding_engine.cpp`
* **Technical Details:**
  - Initialize llama.cpp context with `embedding = true`. Accept model path/name dynamically per load invocation.
  - Support GPU offloading via `gpu_offload_layers` (`-ngl`) from `system.ini`.
  - Normalize output embedding vectors to unit length (L2 normalization) for cosine similarity search.
* **Testing & Coverage Requirements:**
  - Write unit tests in `test_llama_embedding_engine.cpp` for model loading, single/batch vector generation, L2 normalization, and invalid input handling.
  - Maintain **>= 90% code coverage** on `llama_embedding_engine.cpp`.

---

## Phase 4: Similarity Search Subsystem (`infrastructure/similarity`)

### Subphase 4.1: Interface, CMake Setup & Mocking
* **Goal:** Define `ISimilaritySearch` interface, establish `INFRASTRUCTURE::SIMILARITY` CMake target, and create mock classes.
* **Layer:** `Infrastructure` (`CantaTema/components/infrastructure/similarity`)
* **Files to Create:**
  - **[NEW]** `CantaTema/components/infrastructure/similarity/CMakeLists.txt`
  - **[NEW]** `CantaTema/components/infrastructure/similarity/include/similarity/i_similarity_search.hpp`
  - **[NEW]** `CantaTema/components/infrastructure/similarity/mocks/mock_similarity_search.hpp`
* **Technical Details:**
  - Define `SimilarityResult` struct (`pdf_chunk_index`, `best_transcript_chunk_index`, `similarity_score`, `is_mentioned`, `weighted_missed_score`).
  - Declare `ISimilaritySearch` interface methods for indexing embedding vectors and querying nearest neighbors with raw numeric threshold parameters.
* **Testing & Coverage Requirements:**
  - Verify CMake target compiles and mock interface links cleanly.

---

### Subphase 4.2: Faiss Similarity Search Implementation & Unit Tests
* **Goal:** Implement `FaissSimilaritySearch` wrapping the Faiss C++ API (`IndexFlatIP` / `IndexHNSWFlat`) and write unit tests.
* **Layer:** `Infrastructure` (`CantaTema/components/infrastructure/similarity`)
* **Files to Create:**
  - **[NEW]** `CantaTema/components/infrastructure/similarity/include/similarity/faiss_similarity_search.hpp`
  - **[NEW]** `CantaTema/components/infrastructure/similarity/src/faiss_similarity_search.cpp`
  - **[NEW]** `CantaTema/components/infrastructure/similarity/test/test_faiss_similarity_search.cpp`
* **Technical Details:**
  - Index transcript embedding vectors into a Faiss Inner Product index.
  - Query each PDF chunk embedding against the index to retrieve the top-1 nearest transcript chunk.
  - Classify chunk as `is_mentioned = (similarity_score >= similarity_threshold)`.
  - Compute `weighted_missed_score = importance_weight * (1.0 - similarity_score)` for unmatched chunks.
* **Testing & Coverage Requirements:**
  - Write unit tests in `test_faiss_similarity_search.cpp` testing vector indexing, similarity threshold filtering with raw floats, and missed score formulas.
  - Maintain **>= 90% code coverage** on `faiss_similarity_search.cpp`.

---

## Phase 5: Sound System Limits, Voice Analyzer & Speech Recognition

### Subphase 5.1: Max Recording Duration Limit & Sound System Hard Limit Enforcements
* **Goal:** Update `SoundSystem` / `SoundHandler` to enforce hard max practice recording limits (`max_recording_duration_minutes`) configured in `system.ini`.
* **Layer:** `Infrastructure` (`CantaTema/components/infrastructure/sound_system`)
* **Files to Modify:**
  - `CantaTema/components/infrastructure/sound_system/include/sound_system/sound_system.hpp`
  - `CantaTema/components/infrastructure/sound_system/src/sound_system.cpp`
  - `CantaTema/components/infrastructure/sound_system/test/test_sound_system.cpp`
* **Technical Details:**
  - Monitor recording stream timestamp (`get_recording_timestamp()`).
  - Automatically terminate recording and finalize Ogg Opus stream if elapsed duration reaches `max_recording_duration_minutes * 60 * 1000` ms.
* **Testing & Coverage Requirements:**
  - Write unit tests verifying recording timer cut-off and stream finalization under mock time triggers.
  - Maintain **>= 90% code coverage** on `sound_system.cpp`.

---

### Subphase 5.2: Whisper Language Inheritance, Decryption Stream & Timestamps
* **Goal:** Extend `ISpeechRecognition` / `WhisperSpeechRecognition` to accept dynamic language & model parameters, decode XOR-encrypted Opus streams, and output word-level timestamps/log-probabilities.
* **Layer:** `Infrastructure` (`CantaTema/components/infrastructure/speech_recognition`)
* **Files to Modify:**
  - `CantaTema/components/infrastructure/speech_recognition/include/speech_recognition/i_speech_recognition.hpp`
  - `CantaTema/components/infrastructure/speech_recognition/include/speech_recognition/whisper_speech_recognition.hpp`
  - `CantaTema/components/infrastructure/speech_recognition/src/whisper_speech_recognition.cpp`
  - `CantaTema/components/infrastructure/speech_recognition/test/test_whisper_speech_recognition.cpp`
* **Technical Details:**
  - Audio Stream Handling: Decrypt XOR masking and decode `.opus` practice file via `ISoundSystem` / `SoundHandler` to 16kHz mono 32-bit float PCM samples before passing to whisper.cpp.
  - Support language parameter (e.g. `-l es` or `-l en`). If empty, inherit from parent `Subject::get_language()`.
  - Configure Whisper flags for token log-probabilities (`-lpt`) and word timestamps (`-wt`).
  - Output `std::vector<TranscriptSegment>` containing segment text, start/end timestamps (ms), and average token confidence.
* **Testing & Coverage Requirements:**
  - Update unit tests in `test_whisper_speech_recognition.cpp` to verify structured segment outputs, timestamps, XOR-decrypted audio input, language flag passing, and model override parameters.
  - Maintain **>= 90% code coverage** on `whisper_speech_recognition.cpp`.

---

### Subphase 5.3: Voice Quality Metrics Calculation Engine & Unit Tests
* **Goal:** Implement calculation engine for speech rate, clarity, and pacing variance normalized to a 0–100 scale, with complete unit test suite.
* **Layer:** `Infrastructure` (`CantaTema/components/infrastructure/speech_recognition`)
* **Files to Create:**
  - **[NEW]** `CantaTema/components/infrastructure/speech_recognition/include/speech_recognition/voice_quality_analyzer.hpp`
  - **[NEW]** `CantaTema/components/infrastructure/speech_recognition/src/voice_quality_analyzer.cpp`
  - **[NEW]** `CantaTema/components/infrastructure/speech_recognition/test/test_voice_quality_analyzer.cpp`
* **Technical Details:**
  - **Speech Rate:** Words/tokens per second derived from consecutive segment timestamps.
  - **Clarity Score:** Normalized average token log-probabilities across transcript segments.
  - **Pacing Variance:** Standard deviation of pause durations between consecutive spoken segments.
  - Normalize all scores to 0–100 scale.
* **Testing & Coverage Requirements:**
  - Write unit tests in `test_voice_quality_analyzer.cpp` evaluating edge case segment sequences (fast speech, long pauses, poor confidence).
  - Maintain **>= 90% code coverage** on `voice_quality_analyzer.cpp`.

---

## Phase 6: Audio-to-PDF Coverage Analyzer Operations (`operation_coverage`)

### Subphase 6.1: Operation Interface & Setup
* **Goal:** Create `IOperationCoverage` interface and `OperationCoverage` skeleton in business layer using constructor dependency injection.
* **Layer:** `Operations` (`CantaTema/components/business/operations`)
* **Files to Create / Modify:**
  - **[NEW]** `CantaTema/components/business/operations/include/operations/i_operation_coverage.hpp`
  - **[NEW]** `CantaTema/components/business/operations/include/operations/operation_coverage.hpp`
  - **[NEW]** `CantaTema/components/business/operations/src/operation_coverage.cpp`
  - **[MODIFY]** `CantaTema/components/business/operations/CMakeLists.txt`
* **Technical Details:**
  - Declare analysis execution interface accepting per-run configuration parameters:
    `virtual rst_code_e analyze_practice_coverage(int practice_id, const std::string& whisper_model = "", const std::string& llama_model = "", float similarity_threshold = 0.0f, const std::string& language = "", std::string& out_analysis_execution_id) = 0;`
  - Inject infrastructure interfaces: `IFileHandler`, `ISoundSystem`, `ISpeechRecognition`, `IEmbeddingEngine`, `ISimilaritySearch`, `IDatabase`.
* **Testing & Coverage Requirements:**
  - Ensure component compiles cleanly and links mock dependencies in build scripts.

---

### Subphase 6.2: Coverage Pipeline Orchestration, Historic Execution Logging & Config Snapshots
* **Goal:** Implement the full coverage pipeline, generate structured JSON reports, record historic analysis executions with full configuration JSON snapshots in SQLite, and write comprehensive mock unit tests.
* **Layer:** `Operations` (`CantaTema/components/business/operations`)
* **Files to Create / Modify:**
  - `CantaTema/components/business/operations/include/operations/operation_coverage.hpp`
  - `CantaTema/components/business/operations/src/operation_coverage.cpp`
  - **[NEW]** `CantaTema/components/business/operations/test/test_operation_coverage.cpp`
* **Technical Details:**
  - Pipeline workflow:
    1. Look up `PracticeEvent` -> `Subject`: Get reference `filepath` and default `language`. Use language override if specified.
    2. Check PDF page count against `max_pdf_page_count`. Fail with `RST_ERR_EXCEEDS_PAGE_LIMIT` if exceeded.
    3. Resolve models (overrides vs `AUTO` vs INI defaults) and raw float `similarity_threshold`.
    4. Extract reference text chunks & weights via `IFileHandler` & `PdfChunkExtractor`.
    5. Transcribe decrypted Opus audio via `ISpeechRecognition` & `SoundSystem`.
    6. Generate embeddings via `IEmbeddingEngine`.
    7. Index embeddings and search via `ISimilaritySearch`.
    8. Calculate voice metrics via `VoiceQualityAnalyzer`.
    9. Build full JSON config snapshot & detailed JSON coverage report.
    10. Generate unique `analysis_execution_id` and persist historic entry to SQLite via `IDatabase::save_coverage_analysis_execution`.
* **Testing & Coverage Requirements:**
  - Write unit tests in `test_operation_coverage.cpp` using GMock objects, testing historic execution creation, config snapshots, language inheritance, and page limit error triggers.
  - Maintain **>= 90% code coverage** on `operation_coverage.cpp`.

---

## Phase 7: Business Facade & Session Integration

### Subphase 7.1: Session Facade Extension, Subject Language & Historic Execution Queries
* **Goal:** Update `Session` facade class in `business/session` to expose subject language management, coverage analysis triggering, and execution history retrieval.
* **Layer:** `Session` (`CantaTema/components/business/session`)
* **Files to Modify:**
  - `CantaTema/components/business/session/include/session/session.hpp`
  - `CantaTema/components/business/session/src/session.cpp`
  - `CantaTema/components/business/session/test/test_session.cpp`
* **Technical Details:**
  - Add session facade methods:
    - `rst_code_e set_subject_language(int subject_id, const std::string& language);`
    - `rst_code_e analyze_practice_coverage(int practice_id, std::string& out_execution_id, const std::string& whisper_model = "", const std::string& llama_model = "", float similarity_threshold = 0.0f, const std::string& language = "");`
    - `rst_code_e get_analysis_executions_for_practice(int practice_id, std::string& executions_list_json);`
    - `rst_code_e get_analysis_execution_details(const std::string& execution_id, std::string& report_json, std::string& config_json);`
  - Maintain active user session state and authorization checks.
* **Testing & Coverage Requirements:**
  - Update unit tests in `test_session.cpp` for coverage analysis delegation, language settings, and historic execution queries.
  - Maintain **>= 90% code coverage** on `session.cpp`.

---

## Phase 8: Terminal CLI Commands & User Interface

### Subphase 8.1: Subject Language CLI Commands & Model Management
* **Goal:** Update CLI subject management menu to support setting subject language and model management.
* **Layer:** `CLI` (`CantaTema/apps/terminal/`)
* **Files to Modify:**
  - `CantaTema/apps/terminal/src/cli_menu_subject.cpp`
  - `CantaTema/apps/terminal/include/cli_menu_subject.hpp`
  - `CantaTema/apps/terminal/src/cli_menu_models.cpp`
  - `CantaTema/apps/terminal/include/cli_menu_models.hpp`
* **Technical Details:**
  - Commands:
    - `subject subject_set_language <subject_id> <language>`: Sets language (e.g. `es`, `en`) for a subject.
    - `models list`, `models download <model_name>`.
* **Testing & Verification:**
  - Test command execution in interactive terminal CLI shell.

---

### Subphase 8.2: Coverage Analysis CLI Menu (`coverage`)
* **Goal:** Implement interactive `coverage` CLI menu supporting multi-run analysis execution logging, history inspection, and parameter overrides.
* **Layer:** `CLI` (`CantaTema/apps/terminal/`)
* **Files to Create / Modify:**
  - **[NEW]** `CantaTema/apps/terminal/include/cli_menu_coverage.hpp`
  - **[NEW]** `CantaTema/apps/terminal/src/cli_menu_coverage.cpp`
  - **[MODIFY]** `CantaTema/apps/main_terminal.cpp`
* **Technical Details:**
  - Commands:
    - `coverage analyze <practice_id> [whisper_model] [llama_model] [threshold] [lang]`: Runs analysis, generates new `analysis_execution_id`, and stores config snapshot.
    - `coverage history <practice_id>`: Lists all past analysis runs for a practice session with timestamps, models used, and coverage %.
    - `coverage report <execution_id>`: Displays formatted terminal output of coverage %, speed, clarity, and pacing scores for a specific execution run.
* **Testing & Verification:**
  - Perform manual E2E CLI verification: `user_identify` → `subject subject_add 1 topic1 example_data/topic1.pdf` → `subject subject_set_language 1 es` → `practice add_recorded 1 session_01` → `coverage analyze 1` → `coverage history 1` → `coverage report <execution_id>`.

---

## Phase 9: System Integration, Doxygen & Final Documentation

### Subphase 9.1: End-to-End System Integration & Coverage Enforcement Audit
* **Goal:** Audit all project components to verify that 100% of unit tests pass and every production source file satisfies the mandatory **90% code coverage threshold**.
* **Layer:** `QA` / `Testing`
* **Execution Commands:**
  ```powershell
  cmake -B build -DENABLE_COVERAGE=ON
  cmake --build build --config Debug
  ctest --test-dir build -C Debug -T coverage
  ```
* **Acceptance Criteria:**
  - All unit tests pass in CTest.
  - Code coverage report confirms **>= 90% coverage** on all production `.h`, `.hpp`, `.c`, and `.cpp` files.

---

### Subphase 9.2: Doxygen API Documentation & Schema Updates
* **Goal:** Complete Doxygen comment blocks across all public headers and update project documentation.
* **Layer:** `Documentation`
* **Files to Modify:**
  - All public headers across `core`, `infrastructure`, `operations`, `session`.
  - `docs/dev.md` and database schema files in `docs/schemas/`.
* **Acceptance Criteria:**
  - All public methods, classes, and structs documented with Doxygen comment blocks (`/** ... */`).
