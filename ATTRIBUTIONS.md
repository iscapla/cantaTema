# 📜 Third-Party Attributions & Licenses

CantaTema relies on several open-source libraries, frameworks, and engine backends. We gratefully acknowledge the authors and contributors of the following software packages.

---

## 🛠️ Third-Party Dependencies Table

| Library / Tool | Version | License | Primary Purpose | Project / Copyright Holder |
|----------------|---------|---------|-----------------|----------------------------|
| **Whisper.cpp** | `1.8.3` | [MIT](https://opensource.org/licenses/MIT) | Local offline speech-to-text inference engine | Georgi Gerganov & whisper.cpp contributors |
| **llama.cpp** | Latest | [MIT](https://opensource.org/licenses/MIT) | Local GGUF vector text embeddings engine | Georgi Gerganov & llama.cpp contributors |
| **Faiss** | Latest | [MIT](https://opensource.org/licenses/MIT) | C++ vector similarity indexing & nearest-neighbor search | Meta Platforms, Inc. |
| **SDL3** | `3.4.0` | [zlib](https://www.zlib.net/zlib_license.html) | Cross-platform audio recording, playback, and stream handling | Sam Lantinga & SDL contributors |
| **Opus Codec** | `1.5.2` | [BSD-3-Clause](https://opensource.org/licenses/BSD-3-Clause) | High-efficiency audio compression and stream framing | Xiph.Org Foundation, Skype Limited, Octasic |
| **MuPDF** | Latest | [AGPL-3.0](https://www.gnu.org/licenses/agpl-3.0.html) / Commercial | PDF text extraction and style attribute parsing | Artifex Software, Inc. |
| **SQLite** | Latest | [Public Domain](https://sqlite.org/copyright.html) | Embedded transactional database engine | D. Richard Hipp & SQLite contributors |
| **TagLib** | `2.1.1` | [LGPL-2.1](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html) / [MPL-1.1](https://www.mozilla.org/en-US/MPL/1.1/) | Audio file metadata extraction | Scott Wheeler & TagLib contributors |
| **utf8cpp** | `4.0.9` | [BSL-1.0](https://www.boost.org/LICENSE_1_0.txt) | UTF-8 Unicode string validation and manipulation | Nemanja Trifunovic |
| **libcurl** | `curl-8_18_0` | [curl License](https://curl.se/docs/copyright.html) (MIT-like) | HTTP stream handling for Hugging Face model downloads | Daniel Stenberg & libcurl contributors |
| **spdlog** | `1.16.0` | [MIT](https://opensource.org/licenses/MIT) | Fast, thread-safe C++ logging framework | Gabi Melman & spdlog contributors |
| **fmt** | `12.1.0` | [MIT](https://opensource.org/licenses/MIT) | Modern string formatting library | Victor Zverovich & fmt contributors |
| **cli** | `2.2.0` | [MIT](https://opensource.org/licenses/MIT) | Interactive terminal menu system (Asio backend) | Daniele Pallastrelli |
| **SimpleIni** | `4.25` | [MIT](https://opensource.org/licenses/MIT) | INI configuration file parser (`system.ini`) | Brodie Thiesfield |
| **GoogleTest / GMock** | `1.17.0` | [BSD-3-Clause](https://opensource.org/licenses/BSD-3-Clause) | C++ unit testing and mock object framework | Google LLC |

---

## ⚖️ License Summaries & Legal Notices

### MIT License
Used by Whisper.cpp, llama.cpp, Faiss, spdlog, fmt, cli, and SimpleIni.  
*Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files, to deal in the Software without restriction...*

### zlib License
Used by SDL3.  
*This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software...*

### BSD 3-Clause License
Used by Opus Codec and GoogleTest.  
*Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met...*

### AGPL-3.0 License / Commercial
Used by MuPDF (Artifex Software, Inc.).  
*Free Software distribution under GNU AGPLv3 or commercial licensing options.*

### Public Domain
Used by SQLite.  
*All of the code and documentation in SQLite has been dedicated to the public domain by the authors.*
