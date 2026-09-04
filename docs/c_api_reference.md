# CantaTema C ABI Native Engine Reference & Dart FFI Specification

## 1. Executive Summary & Purpose

This document specifies the complete **C ABI Middleware Layer** (`cantatema_bridge`) exporting native C functions (`extern "C"`) from the C++23 CantaTema native engine. This shared dynamic library (`.dll` on Windows, `.so` on Linux/Android, `.dylib` on macOS) provides the sole bridge between the native core engine and frontend client applications, specifically the cross-platform **Flutter UI application** via `dart:ffi`.

The specification defines all exported symbols, parameter constraints, memory ownership invariants, status codes, and JSON schemas. **Any agent or engineer implementing the Flutter client must follow this document as the authoritative contract.**

---

## 2. General ABI & Memory Management Invariants

### 2.1 Calling Convention & Linkage
- **Calling Convention:** Standard C calling convention (`cdecl` on x86, standard platform ABI on x64/ARM64).
- **Symbol Export:** Decorated with `CANTATEMA_API` (`__declspec(dllexport)` on Windows MSVC/MinGW, `__attribute__((visibility("default")))` on GCC/Clang).
- **Header File:** Located at `CantaTema/components/c_api/include/c_api/cantatema_c_api.h`.

### 2.2 String Encoding & Lifetime Rules
1. **UTF-8 Representation:** All text strings (`const char*`) passed into or returned by the C ABI must be valid null-terminated (`\0`) UTF-8 strings.
2. **Caller String Arguments (Inputs):** Memory for string parameters passed into C ABI functions is owned by the caller (Flutter/Dart). The C++ engine borrows the pointer during the synchronous execution of the call and never retains or frees it.
3. **Engine String Returns (Outputs):** Every `const char*` returned by the engine is allocated on the C runtime heap using `std::malloc`.
4. **Mandatory Deallocation (`canta_free_string`):** The caller **MUST** release every non-null returned `const char*` by calling `canta_free_string(ptr)`. Failure to do so causes a native memory leak.
5. **Thread Safety & In-Process Single Session Limit:**
   - The middleware enforces a **strict maximum of 1 active user session** per engine process.
   - All state mutations are synchronized internally using mutex locks (`g_engine_mutex`).
   - All user-dependent queries automatically operate within the context of the active authenticated session user.

### 2.3 Binary Buffer Streaming Rules
For binary audio streaming (`canta_read_audio_stream`):
- The caller pre-allocates a native buffer (e.g. `calloc<Uint8>(bufferSize)` in Dart FFI).
- The caller passes the buffer pointer, capacity, and pointers to receive the read count and EOF flag.
- The engine copies decrypted audio bytes directly into the buffer with **zero temporary files**.

---

## 3. Native Return Status Codes (`rst_code_e`)

Functions returning integer status codes conform to `rst_code_e` (defined in `primitives/definitions.hpp`). The textual identifier for any code can be retrieved dynamically via `canta_get_error_text(code)`.

| Code | Enumerator | Description |
|---|---|---|
| `0` | `RST_OK` | Operation completed successfully |
| `1` | `CONFIG_FILE` | Error loading or reading configuration file |
| `2` | `CONFIG_PARSE` | Error parsing configuration parameter |
| `3` | `DB_FAIL` | General SQLite database failure |
| `4` | `DB_NOT_FOUND` | Key or record not found in database |
| `5` | `DB_BAD_PARAM` | Invalid format, null pointer, or illegal argument |
| `6` | `CONSOLE_EXP` | Internal runtime console exception |
| `7` | `USER_ERROR` | General user operation error |
| `8` | `USER_NOT_FOUND` | User identifier does not exist |
| `9` | `USER_NO_AUTH` | Session is not authenticated or access denied |
| `10` | `USER_DUPLICATED` | Username / email already registered |
| `11` | `USER_METRICS_ERROR` | General user quota/metrics error |
| `12` | `USER_METRICS_NOT_FOUND` | Metrics record not found |
| `13` | `USER_METRICS_NOT_ENOUGH_SPACE` | User disk storage quota exceeded |
| `14` | `CATEGORY_ERROR` | General category management error |
| `15` | `CATEGORY_NOT_FOUND` | Topic category does not exist |
| `16` | `CATEGORY_DUPLICATED` | Topic category already exists |
| `17` | `SUBJECT_ERROR` | General topic / subject error |
| `18` | `SUBJECT_NOT_FOUND` | Study topic not found |
| `19` | `SUBJECT_DUPLICATED` | Study topic already exists |
| `20` | `TAG_ERROR` | Tagging system error |
| `21` | `TAG_NOT_FOUND` | Topic tag not found |
| `22` | `TAG_DUPLICATED` | Tag already assigned |
| `23` | `PRACTICE_EVENT_ERROR` | Audio recording or practice session error (e.g. device busy) |
| `24` | `PRACTICE_EVENT_NOT_FOUND` | Practice event ID does not exist |
| `25` | `PRACTICE_EVENT_ILLEGAL_CHANGE`| Illegal state transition (e.g. recorded to planned) |
| `26` | `PRACTICE_EVENT_DATE_MISSMATCH`| Invalid practice event timestamp |
| `27` | `PRACTICE_EVENT_NO_SOUND_LENGHT`| Audio file length cannot be determined |
| `28` | `FILE_NOT_FOUND` | File path does not exist on disk |
| `29` | `FILE_UPLOAD_ERROR` | File copying, moving, or encryption error |
| `30` | `FILE_FORMAT_ERROR` | Unsupported audio or document format |
| `34` | `SOUND_SYSTEM_NOT_RECORDING` | Audio capture stream is not running |
| `41` | `TASK_NOT_FOUND` | Analysis scheduler task not found |
| `999`| `UNKNOWN` | Unhandled internal native exception |

---

## 4. Complete Exported C ABI Functions Reference

### 4.1 Engine Lifecycle & Global Setup

#### `canta_init_engine`
```c
int32_t canta_init_engine(const char* storage_path, const char* config_json);
```
- **Description:** Initializes the native CantaTema engine, loads `system.ini`, opens SQLite database connections, and creates the default session facade. Limits the engine to exactly 1 active session.
- **Parameters:**
  - `storage_path`: Base directory for database, recordings, and models. Pass `NULL` to use standard OS app data path.
  - `config_json`: Optional JSON override configuration. Pass `NULL` for defaults.
- **Returns:** `int32_t` status code (`0` on success).

#### `canta_shutdown_engine`
```c
void canta_shutdown_engine(void);
```
- **Description:** Stops any active audio recording or playback streams, logs out the user, closes database connections, flushes background worker tasks, and resets native session state.

#### `canta_get_engine_version`
```c
const char* canta_get_engine_version(void);
```
- **Description:** Returns the semver version string of the engine (e.g., `"1.0.0"`).
- **Returns:** Heap-allocated UTF-8 string. **Must be freed with `canta_free_string`**.

#### `canta_free_string`
```c
void canta_free_string(const char* str);
```
- **Description:** Deallocates a heap-allocated UTF-8 C string returned by any C ABI function. Safe to call with `NULL`.

#### `canta_get_error_text`
```c
const char* canta_get_error_text(int32_t error_code);
```
- **Description:** Translates an integer `rst_code_e` into its enum string name (e.g. `USER_NO_AUTH`).
- **Returns:** Heap-allocated UTF-8 string. **Must be freed with `canta_free_string`**.

---

### 4.2 User Authentication & Profile

#### `canta_get_current_user_json`
```c
const char* canta_get_current_user_json(void);
```
- **Description:** Retrieves the profile of the currently authenticated candidate in the active session.
- **Returns:** JSON string of `UserProfile`, or `NULL` if not authenticated. **Must be freed with `canta_free_string`**.

#### `canta_login_user_json`
```c
const char* canta_login_user_json(const char* email_or_name, const char* password);
```
- **Description:** Authenticates credentials, initializes session state, and logs in the candidate.
- **Parameters:**
  - `email_or_name`: Username or email address.
  - `password`: Password string.
- **Returns:** JSON string of `UserProfile` on success, or `NULL` on invalid credentials. **Must be freed with `canta_free_string`**.

#### `canta_register_user_json`
```c
const char* canta_register_user_json(const char* email_or_name, const char* password, const char* display_name);
```
- **Description:** Creates a new candidate account, automatically splits `display_name` into first and last names, and authenticates the user into the active session.
- **Returns:** JSON string of `UserProfile` on success, or `NULL` on error. **Must be freed with `canta_free_string`**.

#### `canta_logout_user`
```c
int32_t canta_logout_user(void);
```
- **Description:** Clears the authenticated session user and resets session context to unauthenticated.
- **Returns:** `int32_t` status code (`0` on success).

---

### 4.3 Topic & Category Management

#### `canta_get_categories_json`
```c
const char* canta_get_categories_json(void);
```
- **Description:** Returns all topic categories for the authenticated user, including topic counts.
- **Returns:** JSON array of `TopicCategory` objects. **Must be freed with `canta_free_string`**.

#### `canta_get_all_topics_json`
```c
const char* canta_get_all_topics_json(void);
```
- **Description:** Returns all study topics / subjects belonging to the authenticated user.
- **Returns:** JSON array of `Topic` objects. **Must be freed with `canta_free_string`**.

#### `canta_get_topics_by_category_json`
```c
const char* canta_get_topics_by_category_json(const char* category_id);
```
- **Description:** Queries topics assigned to a specific category.
- **Parameters:** `category_id`: Numeric string representing category ID.
- **Returns:** JSON array of `Topic` objects. **Must be freed with `canta_free_string`**.

#### `canta_get_topic_by_id_json`
```c
const char* canta_get_topic_by_id_json(const char* topic_id);
```
- **Description:** Fetches a single topic by ID including reference syllabus PDF/text file path.
- **Returns:** JSON string of `Topic`, or `NULL` if not found. **Must be freed with `canta_free_string`**.

#### `canta_create_topic_json`
```c
const char* canta_create_topic_json(const char* topic_payload_json);
```
- **Description:** Adds a study topic under a category. If no syllabus document path is specified, the engine generates an internal syllabus placeholder.
- **Parameters:** `topic_payload_json`: Serialized JSON with `title`, `categoryName`/`categoryId`, optional `filePath`, `tags`, `language`.
- **Returns:** JSON string of created `Topic`. **Must be freed with `canta_free_string`**.

#### `canta_update_topic_json`
```c
int32_t canta_update_topic_json(const char* topic_payload_json);
```
- **Description:** Updates topic title, category, syllabus document path, or language.
- **Returns:** `int32_t` status code (`0` on success).

#### `canta_delete_topic`
```c
int32_t canta_delete_topic(const char* topic_id);
```
- **Description:** Removes a topic and its syllabus files.
- **Returns:** `int32_t` status code (`0` on success).

---

### 4.4 Audio Recording Studio & DSP Metering

#### `canta_start_recording_session`
```c
int32_t canta_start_recording_session(const char* topic_id);
```
- **Description:** Arms microphone capture for reciting the specified topic. Creates output directory structure and initializes Opus audio capture.
- **Returns:** `0` (`RST_OK`) on success, `23` (`PRACTICE_EVENT_ERROR`) if recording is already active, `9` (`USER_NO_AUTH`) if logged out.

#### `canta_pause_recording_session`
```c
int32_t canta_pause_recording_session(void);
```
- **Description:** Pauses audio capture buffer accumulation without terminating the recording session.
- **Returns:** `int32_t` status code (`0` on success).

#### `canta_resume_recording_session`
```c
int32_t canta_resume_recording_session(void);
```
- **Description:** Resumes active capture after pause.
- **Returns:** `int32_t` status code (`0` on success).

#### `canta_stop_recording_session`
```c
const char* canta_stop_recording_session(void);
```
- **Description:** Finalizes the audio capture, applies XOR masking encryption, creates a recorded `PracticeEvent` in the database, and returns the study session metadata.
- **Returns:** JSON string of created `StudySession`, or `NULL` if not recording. **Must be freed with `canta_free_string`**.

#### `canta_get_current_audio_amplitude`
```c
float canta_get_current_audio_amplitude(void);
```
- **Description:** Computes real-time RMS amplitude of incoming microphone PCM frames. Normalized between `0.0f` (silence) and `1.0f` (peak). Used for live visualizer waveforms.
- **Returns:** `float` normalized amplitude (returns `0.0f` when paused or idle).

#### `canta_poll_live_transcription`
```c
const char* canta_poll_live_transcription(void);
```
- **Description:** Polls incremental transcribed tokens from live Whisper STT.
- **Returns:** Newly transcribed text string, or `NULL` if none. **Must be freed with `canta_free_string`**.

---

### 4.5 Study Session History & Practice Events

#### `canta_create_study_session_json`
```c
const char* canta_create_study_session_json(const char* topic_id, const char* audio_path, uint32_t duration_seconds);
```
- **Description:** Imports or creates a study session record associated with a topic. Copies and encrypts external audio if provided.
- **Parameters:**
  - `topic_id`: Associated topic identifier.
  - `audio_path`: Path to an existing Opus audio file, or `NULL` to generate a recorded session placeholder.
  - `duration_seconds`: Recording duration in seconds.
- **Returns:** JSON string of created `StudySession`. **Must be freed with `canta_free_string`**.

#### `canta_get_sessions_for_topic_json`
```c
const char* canta_get_sessions_for_topic_json(const char* topic_id);
```
- **Description:** Queries recitation history for a topic, sorted chronologically descending.
- **Returns:** JSON array of `StudySession` objects. **Must be freed with `canta_free_string`**.

#### `canta_get_recent_sessions_json`
```c
const char* canta_get_recent_sessions_json(void);
```
- **Description:** Returns the global recent practice sessions feed across all topics for the candidate.
- **Returns:** JSON array of `StudySession` objects. **Must be freed with `canta_free_string`**.

#### `canta_get_session_by_id_json`
```c
const char* canta_get_session_by_id_json(const char* session_id);
```
- **Description:** Retrieves metadata for a single practice session by ID.
- **Returns:** JSON string of `StudySession`, or `NULL` if not found. **Must be freed with `canta_free_string`**.

#### `canta_delete_session`
```c
int32_t canta_delete_session(const char* session_id);
```
- **Description:** Deletes a practice session record and permanently deletes its encrypted audio file from disk.
- **Returns:** `int32_t` status code (`0` on success).

---

### 4.6 Speech Analysis & Syllabus Evaluation

#### `canta_generate_session_analysis_json`
```c
const char* canta_generate_session_analysis_json(const char* session_id);
```
- **Description:** Triggers the synchronous speech analysis and coverage evaluation pipeline:
  1. Decrypts Opus audio and converts to WAV.
  2. Transcribes voice via local Whisper STT with word timestamps.
  3. Extracts sentence chunks from reference PDF syllabus via MuPDF.
  4. Vectorizes chunks via Llama.cpp multilingual embeddings.
  5. Computes vector nearest-neighbor similarity via Faiss.
  6. Computes voice clarity, pacing, pause variance, and missed weighted sections.
- **Returns:** Comprehensive `AnalysisReport` JSON string. **Must be freed with `canta_free_string`**.

#### `canta_get_session_analysis_report_json`
```c
const char* canta_get_session_analysis_report_json(const char* session_id);
```
- **Description:** Retrieves cached `AnalysisReport` for a previously analyzed practice session.
- **Returns:** `AnalysisReport` JSON string, or `NULL` if not analyzed yet. **Must be freed with `canta_free_string`**.

---

### 4.7 Task Scheduler & Agenda Calendar

#### `canta_get_schedule_items_json`
```c
const char* canta_get_schedule_items_json(int32_t status_filter, const char* search_query);
```
- **Description:** Lists background analysis tasks.
- **Parameters:**
  - `status_filter`: Filter by status (`0`=waiting, `1`=processing, `2`=completed, `3`=cancelled, `-1`=all).
  - `search_query`: Filter by task ID substring, or `NULL` for all.
- **Returns:** JSON array of `ScheduleItem` objects. **Must be freed with `canta_free_string`**.

#### `canta_create_schedule_item_json`
```c
const char* canta_create_schedule_item_json(const char* schedule_payload_json);
```
- **Description:** Queues a background analysis task for an unanalyzed practice event.
- **Parameters:** JSON containing `{"practiceId": 123}`.
- **Returns:** Created `ScheduleItem` JSON string. **Must be freed with `canta_free_string`**.

#### `canta_update_schedule_item_status`
```c
int32_t canta_update_schedule_item_status(const char* item_id, int32_t new_status);
```
- **Description:** Updates status or cancels a background analysis task (passing `3` cancels execution).
- **Returns:** `int32_t` status code (`0` on success).

#### `canta_delete_schedule_item`
```c
int32_t canta_delete_schedule_item(const char* item_id);
```
- **Description:** Cancels and dequeues an analysis task.
- **Returns:** `int32_t` status code (`0` on success).

#### `canta_get_calendar_events_json`
```c
const char* canta_get_calendar_events_json(void);
```
- **Description:** Returns calendar events (both past recorded recitations and future scheduled cantes).
- **Returns:** JSON array of `CalendarEvent` objects. **Must be freed with `canta_free_string`**.

#### `canta_add_calendar_event_json`
```c
int32_t canta_add_calendar_event_json(const char* event_payload_json);
```
- **Description:** Schedules a future practice cante date on the study agenda.
- **Parameters:** JSON with `topicId`, unix timestamp `date`, optional `description`.
- **Returns:** `int32_t` status code (`0` on success).

#### `canta_remove_calendar_event`
```c
int32_t canta_remove_calendar_event(const char* event_id);
```
- **Description:** Cancels and removes a scheduled calendar event.
- **Returns:** `int32_t` status code (`0` on success).

---

### 4.8 Local AI Model Management

#### `canta_get_ai_models_json`
```c
const char* canta_get_ai_models_json(void);
```
- **Description:** Lists local and downloadable Whisper STT models (`ggml-*.bin`) and Llama.cpp embedding models (`*.gguf`).
- **Returns:** JSON array of `AiModelItem` objects. **Must be freed with `canta_free_string`**.

#### `canta_update_ai_model_json`
```c
int32_t canta_update_ai_model_json(const char* model_payload_json);
```
- **Description:** Sets the active model used for STT or embeddings in session configuration.
- **Returns:** `int32_t` status code (`0` on success).

---

### 4.9 Audio Playback & Streaming Range

#### `canta_start_playback`
```c
int32_t canta_start_playback(const char* session_id);
```
- **Description:** Plays back recorded practice audio directly through SDL3. Audio is decrypted on-the-fly via XOR deciphering.
- **Returns:** `int32_t` status code (`0` on success).

#### `canta_stop_playback`
```c
int32_t canta_stop_playback(void);
```
- **Description:** Stops active audio playback stream.
- **Returns:** `int32_t` status code (`0` on success).

#### `canta_read_audio_stream`
```c
int32_t canta_read_audio_stream(
    uint32_t session_id,
    uint64_t offset,
    uint32_t length,
    uint8_t* out_buffer,
    uint32_t* out_bytes_read,
    int32_t* out_is_eof
);
```
- **Description:** Streams decrypted audio bytes directly into a pre-allocated buffer with **zero temporary files**. Enables custom Dart audio playback engines (e.g. `just_audio` stream source).
- **Parameters:**
  - `session_id`: Practice session identifier.
  - `offset`: Byte offset within decrypted stream.
  - `length`: Maximum bytes to read.
  - `out_buffer`: Pointer to pre-allocated native byte array.
  - `out_bytes_read`: Receives number of bytes read.
  - `out_is_eof`: Set to `1` if end-of-file reached, `0` otherwise.
- **Returns:** `int32_t` status code (`0` on success).

---

## 5. JSON Schemas & Payload Examples

### 5.1 `UserProfile`
```json
{
  "id": "1",
  "name": "candidate_juan",
  "email": "juan@example.com",
  "displayName": "Juan Perez",
  "firstName": "Juan",
  "lastName": "Perez",
  "status": "active"
}
```

### 5.2 `TopicCategory`
```json
{
  "id": "1",
  "name": "Derecho Constitucional",
  "topicCount": 4
}
```

### 5.3 `Topic`
```json
{
  "id": "1",
  "categoryId": "1",
  "categoryName": "Derecho Constitucional",
  "title": "Tema 1: La Corona y el Rey",
  "filePath": "C:/Users/.../Tema_1.pdf",
  "language": "es",
  "tags": ["constitucional", "oposicion_justicia"],
  "mastery": 0.85,
  "totalSessions": 3
}
```

### 5.4 `StudySession`
```json
{
  "id": "10",
  "topicId": "1",
  "topicTitle": "Tema 1: La Corona y el Rey",
  "audioPath": "C:/Users/.../user_1/subject_1/10/rec.opus",
  "durationSeconds": 312,
  "status": "completed",
  "recordedDate": 1741190400,
  "executionId": "exec-9f8a-4b21"
}
```

### 5.5 `ScheduleItem` (Background Task)
```json
{
  "id": "task-8f92-ac",
  "practiceId": 10,
  "status": "waiting",
  "statusCode": 0,
  "progress": 0
}
```

### 5.6 `CalendarEvent`
```json
{
  "id": "15",
  "topicId": "1",
  "topicTitle": "Tema 1: La Corona y el Rey",
  "date": 1741363200,
  "status": "planned",
  "description": "Repaso primer cante ante preparador"
}
```

### 5.7 `AiModelItem`
```json
{
  "name": "multilingual-e5-large-q4_k_m",
  "type": "llama",
  "path": "C:/Users/.../models/multilingual-e5-large-q4_k_m.gguf",
  "isActive": true,
  "isDownloaded": true
}
```

### 5.8 `AnalysisReport`
```json
{
  "practiceId": 10,
  "overallCoverage": 87.5,
  "voiceQuality": {
    "speedWordsPerMinute": 142.3,
    "clarityScore": 91.0,
    "pauseVariance": 0.32
  },
  "coveredChunks": [
    {
      "text": "Las funciones constitucionales del Rey corresponden a la jefatura del Estado...",
      "importanceWeight": 1.5,
      "similarityScore": 0.92,
      "mentioned": true
    }
  ],
  "missedChunks": [
    {
      "text": "La sucesión en el trono seguirá el orden regular de primogenitura y representación...",
      "importanceWeight": 2.0,
      "similarityScore": 0.41,
      "mentioned": false,
      "weightedMissedScore": 1.18
    }
  ]
}
```

---

## 6. Dart `dart:ffi` Implementation Architecture for Flutter

### 6.1 Library Loading & Initialization
```dart
import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';

class CantaTemaBridge {
  static final DynamicLibrary _lib = _loadLibrary();

  static DynamicLibrary _loadLibrary() {
    if (Platform.isWindows) {
      return DynamicLibrary.open('cantatema_bridge.dll');
    } else if (Platform.isLinux || Platform.isAndroid) {
      return DynamicLibrary.open('libcantatema_bridge.so');
    } else if (Platform.isMacOS || Platform.isIOS) {
      return DynamicLibrary.open('libcantatema_bridge.dylib');
    }
    throw UnsupportedError('Unsupported operating platform');
  }

  // Bindings
  static final void Function(Pointer<Utf8>) freeString = _lib
      .lookup<NativeFunction<Void Function(Pointer<Utf8>)>>('canta_free_string')
      .asFunction();

  static final int Function(Pointer<Utf8>, Pointer<Utf8>) initEngine = _lib
      .lookup<NativeFunction<Int32 Function(Pointer<Utf8>, Pointer<Utf8>)>>('canta_init_engine')
      .asFunction();

  static final Pointer<Utf8> Function() getCurrentUser = _lib
      .lookup<NativeFunction<Pointer<Utf8> Function()>>('canta_get_current_user_json')
      .asFunction();

  static final double Function() getAmplitude = _lib
      .lookup<NativeFunction<Float Function()>>('canta_get_current_audio_amplitude')
      .asFunction();

  // Helper for receiving and freeing native string
  static String? consumeNativeString(Pointer<Utf8> ptr) {
    if (ptr.address == 0) return null;
    try {
      return ptr.toDartString();
    } finally {
      freeString(ptr);
    }
  }
}
```

### 6.2 Streaming Audio Bytes into Audio Source
```dart
import 'dart:typed_data';

class DecryptedAudioStreamer {
  static Uint8List? readChunk(int sessionId, int offset, int length) {
    final buffer = calloc<Uint8>(length);
    final bytesReadPtr = calloc<Uint32>();
    final isEofPtr = calloc<Int32>();

    try {
      final res = _readAudioStream(sessionId, offset, length, buffer, bytesReadPtr, isEofPtr);
      if (res != 0) return null;
      final bytesRead = bytesReadPtr.value;
      return Uint8List.fromList(buffer.asTypedList(bytesRead));
    } finally {
      calloc.free(buffer);
      calloc.free(bytesReadPtr);
      calloc.free(isEofPtr);
    }
  }
}
```
