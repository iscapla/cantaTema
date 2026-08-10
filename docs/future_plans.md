# CantaTema - Future Comparison Engine Architecture & Roadmap

This document defines the architectural vision, technical specifications, and prioritized implementation roadmap for CantaTema's text comparison engine. All future developments must adhere to the design guidelines and priority order defined below.

---

## 1. Multilingual Language Profile Abstraction (`ILanguageProfile`)

To ensure CantaTema functions cleanly across global languages without touching core algorithm code, all language-specific tokenization, UTF-8 normalization (accent stripping, Spanish `¿`, `¡`), stopword dictionaries, and sentence abbreviations are abstracted into an extensible **Language Profile System**.

### Architecture

```
 ┌─────────────────────────────────────────────────────────────┐
 │                ConfigurationSystem (system.ini)             │
 │                active_language = es | en | fr | de          │
 └──────────────────────────────┬──────────────────────────────┘
                                │
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                   LanguageProfileManager                    │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Retrieves active profile
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                  ILanguageProfile Interface                 │
 ├─────────────────────────────────────────────────────────────┤
 │ + get_language_code() : string                              │
 │ + normalize_word(input) : string                            │
 │ + is_stopword(word) : bool                                  │
 │ + is_abbreviation(word) : bool                              │
 └──────────────────────────────┬──────────────────────────────┘
                                │
          ┌─────────────────────┴─────────────────────┐
          ▼                                           ▼
 ┌───────────────────────────┐               ┌───────────────────────────┐
 │   SpanishLanguageProfile  │               │   EnglishLanguageProfile  │
 ├───────────────────────────┤               ├───────────────────────────┤
 │ - Spanish stopwords       │               │ - English stopwords       │
 │   (el, la, de, en, por)   │               │   (the, of, in, for, to)  │
 │ - UTF-8 accents (á, é, ñ) │               │ - English abbreviations   │
 │ - Non-ASCII punct (¿, ¡)  │               │   (sec, art, p, e.g., i.e.)│
 └───────────────────────────┘               └───────────────────────────┘
```

---

## 2. Dual Orthogonal Subject Domain Profiles (`IDomainProfile`)

Text comparison accuracy depends heavily on the academic subject being evaluated. A candidate taking a **Law** exam must cite specific article numbers and law IDs, whereas candidates in **Economics**, **Science**, **History**, or **General Literature** must cite key data, formulas, dates, or historical eras.

Evaluation uses **Dual Orthogonal Profile Composition (Language $\times$ Subject Domain)**.

### Supported Subject Domain Profiles

| Domain Key | Target Academic Area | High-Priority Citations & Terms | Custom Warning Badge Tag |
| :--- | :--- | :--- | :--- |
| **`law`** *(Default)* | Legal & Administrative Exams | `Artículo N`, `Ley N/Y`, `RD N/Y`, `Estatuto`, `Reglamento` | `⚠️ MISSING LEGAL CITATION` |
| **`economics`** | Macro/Micro Economics & Finance | `PIB`, `IPC`, `inflación`, `déficit`, `balance`, `monetarismo`, `fisiocracia` | `⚠️ MISSING ECONOMIC INDICATOR` |
| **`science`** | Physics, Chemistry, Biology | `ADN`, `ARN`, `ATP`, `mitocondria`, `isótopo`, `teorema`, SI Units (`kg`, `m/s²`, `Hz`, `mol`) | `⚠️ MISSING SCIENTIFIC TERM/UNIT` |
| **`history`** | World & National History | Dates, Centuries (`Siglo XV`, `Siglo XX`), Treaties (`Tratado de Versalles`), Dynasties | `⚠️ MISSING HISTORICAL ERA/DATE` |
| **`general`** | General Literature & Essays | Standard Proper Nouns, Numbers, and Narrative Vocabulary | `⚠️ MISSING KEYWORD` |

---

## 3. Configurable Token Weighting Hierarchy

Words are assigned linguistic importance weights during sequence alignment to ensure minor missing articles do not distort evaluation scores while omitting critical legal/domain citations triggers appropriate penalties.

### Token Weighting Table

| Category | Description | Default Multiplier |
| :--- | :--- | :---: |
| **Domain Citations & Enumerators** | Article numbers (`Artículo 7`), law IDs (`Ley 35/2006`), list labels (`a)`, `b)`, `1º`) | **4.0x** |
| **Numeric & Quantitative Entities** | Percentages (`50%`), monetary amounts (`180.000 euros`), time limits (`5 años`), dates | **3.0x** |
| **Domain Subject Keywords** | Capitalized proper nouns and specialized technical vocabulary | **2.0x** |
| **Standard Vocabulary** | General narrative verbs, nouns, and adjectives | **1.0x** |
| **Grammatical Stopwords & Articles** | Minor prepositions and articles (`el`, `la`, `los`, `las`, `un`, `una`, `de`, `en`, `por`) | **0.2x** |

### Weighted Recall Formula

$$\text{Weighted Recall Score} = \frac{\sum_{w \in \text{Matched Reference Words}} \text{Weight}(w)}{\sum_{w \in \text{All Reference Words}} \text{Weight}(w)} \times 100\%$$

---

## 4. SimpleIni Configuration Mapping (`system.ini`)

All active profiles and weighting multipliers are configurable via `system.ini`:

```ini
[COMPARISON_PROFILE]
active_language = es
active_domain = law
weight_citation = 4.0
weight_numeric = 3.0
weight_domain = 2.0
weight_standard = 1.0
weight_stopword = 0.2
```

---

## 5. Prioritized Implementation Roadmap (Highest Accuracy & Value ROI)

The roadmap below orders upcoming features by **accuracy gain, candidate value, and commercial impact**:

### 🎯 Priority 1: Multilingual Language Profile Abstraction (`ILanguageProfile`)
* **Value & Accuracy ROI**: Critical architectural foundation. Decouples Spanish-specific rules (`es`) from lower-level algorithms and enables instant support for English (`en`), French (`fr`), and German (`de`).

### 🎯 Priority 2: Dual Orthogonal Subject Domain Profiles (`IDomainProfile`)
* **Value & Accuracy ROI**: Extremely High. Introduces specialized evaluation profiles for `law`, `economics`, `science`, `history`, and `general` exams, adapting token weights and warning badges per subject.

### 🎯 Priority 3: CTC Forced Alignment Pass (WhisperX / Wav2Vec2 Integration)
* **Value & Accuracy ROI**: Very High. Standard ASR transcriptions suffer from timestamp drift. A 2nd-pass Connectionist Temporal Classification (CTC) alignment locks audio timecodes to PDF sentence boundaries with millisecond precision (industry standard used by WhisperX & MFA).

### 🎯 Priority 4: Phonetic & ASR Noise Compensation (Double Metaphone / Soundex Alignment)
* **Value & Accuracy ROI**: High. Prevents penalizing candidates when Whisper mishears an accented or slurred word (e.g. transcribing `extinción` instead of `excepción`). Classifies phonetically similar words as **"Minor Speech Mispronunciation" (Yellow / Soft Warning)** instead of content omissions.

### 🎯 Priority 5: Named Entity Precision Scorecard & Automated Exam Rubric Checklist
* **Value & Accuracy ROI**: High. Automatically extracts legal article numbers, law IDs, dates, percentages, and figures into an explicit **Rubric Checklist** in the HTML report (e.g. `[✓] Article 1`, `[✓] Article 2`, `[✗] Article 7 - Omitted`), providing clear evidence for candidates and examiners (used by Prova AI).

### 🎯 Priority 6: Multi-Axis Micro-Skill Diagnostic Radar (Pearson Versant Architecture)
* **Value & Accuracy ROI**: Medium-High. Evaluates candidate performance across 4 distinct micro-skill axes:
  1. `Content Recall %` (Weighted key concepts)
  2. `Legal Citation Accuracy %` (Article/Law IDs)
  3. `Oral Fluency WPM` (Delivery speed & cadence)
  4. `Speech Clarity %` (Acoustic quality)

### 🎯 Priority 7: Synonym & Paraphrase Semantic Equivalence Engine (LSA / Embedding Paraphrasing)
* **Value & Accuracy ROI**: Medium-High. Recognizes valid domain paraphrases (`constituir el hecho imponible` $\leftrightarrow$ `ser el presupuesto generador`) using Latent Semantic Analysis (LSA) and maps them to **Semantic Equivalence (Blue)**.

### 🎯 Priority 8: Interactive Click-to-Play Audio Timestamp Seeking
* **Value & Accuracy ROI**: Medium. Clicking any reference paragraph or transcript snippet in `dual_column_comparison_report.html` immediately seeks and plays back the exact Opus/WAV audio snippet.

### 🎯 Priority 9: Stemming & Lemmatization Matching
* **Value & Accuracy ROI**: Medium. Handles minor inflectional word variations (`impuesto` $\leftrightarrow$ `impuestos`, `regulada` $\leftrightarrow$ `regula`) without score distortion.

### 🎯 Priority 10: Speech Temporal Dynamics & Hesitation Correlation
* **Value & Accuracy ROI**: Medium-Low. Correlates silence gaps (> 5 seconds) with missed legal sections to highlight candidate hesitation and pinpoint topics requiring study reinforcement.

### 🎯 Priority 11: Hierarchical Document Outline Matching (Parent-Child DAG)
* **Value & Accuracy ROI**: Low-Medium. Enforces structural heading relationships (`Título I` $\rightarrow$ `Capítulo II` $\rightarrow$ `Artículo N`) to flag skipped sub-topics.
