/**
 * @file rubric_checklist_extractor.cpp
 * @brief Implementation of rubric checklist extractor and multi-pass verification scorecard engine.
 */

#include "similarity/rubric_checklist_extractor.hpp"
#include "similarity/word_sequence_aligner.hpp"
#include "similarity/phonetic_matcher_manager.hpp"
#include "primitives/domain_profile_manager.hpp"
#include "primitives/domain_profiles.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>
#include <unordered_set>

std::string RubricChecklistExtractor::normalize_text(const std::string& input) {
    return WordSequenceAligner::normalize_word(input);
}

std::vector<RubricItem> RubricChecklistExtractor::extract_rubric_items(
    const std::vector<std::string>& ref_chunks,
    const std::string& domain_key
) {
    std::vector<RubricItem> items;
    auto domain_profile = DomainProfileManager::getInstance().get_profile(domain_key);

    std::unordered_set<std::string> seen_keys;

    for (size_t chunk_idx = 0; chunk_idx < ref_chunks.size(); ++chunk_idx) {
        const std::string& chunk_text = ref_chunks[chunk_idx];
        auto tokens = WordSequenceAligner::tokenize(chunk_text);

        for (size_t i = 0; i < tokens.size(); ++i) {
            std::string norm = WordSequenceAligner::normalize_word(tokens[i]);
            if (norm.empty()) continue;

            // 1. Check for Legal Article references (e.g. "Artículo 14", "Art. 7", "Artículo 2.1")
            if ((norm == "articulo" || norm == "art") && i + 1 < tokens.size()) {
                std::string next_orig = tokens[i + 1];
                std::string next_norm = WordSequenceAligner::normalize_word(next_orig);
                bool has_digit = false;
                for (char c : next_orig) {
                    if (std::isdigit(static_cast<unsigned char>(c))) { has_digit = true; break; }
                }
                if (has_digit || next_norm.length() <= 3) {
                    std::string raw = tokens[i] + " " + tokens[i + 1];
                    std::string key = "art_" + next_norm + "_" + std::to_string(chunk_idx);
                    if (seen_keys.find(key) == seen_keys.end()) {
                        seen_keys.insert(key);
                        RubricItem item;
                        item.item_id = items.size();
                        item.entity_type = RubricEntityType::LEGAL_ARTICLE;
                        item.raw_text = raw;
                        item.normalized_text = "articulo " + next_norm;
                        item.ref_chunk_index = chunk_idx;
                        item.entity_label = "Article " + next_orig;
                        items.push_back(item);
                    }
                    i++;
                    continue;
                }
            }

            // 2. Check for Law IDs / Statutory references (e.g. "Ley 39/2015", "RD 1/2020", "Decreto")
            if ((norm == "ley" || norm == "rd" || norm == "decreto" || norm == "estatuto" || norm == "reglamento") && i + 1 < tokens.size()) {
                std::string next_orig = tokens[i + 1];
                std::string next_norm = WordSequenceAligner::normalize_word(next_orig);
                std::string raw = tokens[i] + " " + next_orig;
                std::string key = "law_" + norm + "_" + next_norm + "_" + std::to_string(chunk_idx);
                if (seen_keys.find(key) == seen_keys.end()) {
                    seen_keys.insert(key);
                    RubricItem item;
                    item.item_id = items.size();
                    item.entity_type = RubricEntityType::LAW_ID;
                    item.raw_text = raw;
                    item.normalized_text = norm + " " + next_norm;
                    item.ref_chunk_index = chunk_idx;
                    item.entity_label = tokens[i] + " " + next_orig;
                    items.push_back(item);
                }
                i++;
                continue;
            }

            // 3. Check for Century or Date expressions (e.g. "Siglo XX", "1978")
            if (norm == "siglo" && i + 1 < tokens.size()) {
                std::string next_orig = tokens[i + 1];
                std::string next_norm = WordSequenceAligner::normalize_word(next_orig);
                std::string raw = tokens[i] + " " + next_orig;
                std::string key = "century_" + next_norm + "_" + std::to_string(chunk_idx);
                if (seen_keys.find(key) == seen_keys.end()) {
                    seen_keys.insert(key);
                    RubricItem item;
                    item.item_id = items.size();
                    item.entity_type = RubricEntityType::DATE_OR_ERA;
                    item.raw_text = raw;
                    item.normalized_text = "siglo " + next_norm;
                    item.ref_chunk_index = chunk_idx;
                    item.entity_label = "Century " + next_orig;
                    items.push_back(item);
                }
                i++;
                continue;
            }

            // 4-digit Year (e.g. 1978, 2020)
            if (norm.length() == 4 && (norm.rfind("18", 0) == 0 || norm.rfind("19", 0) == 0 || norm.rfind("20", 0) == 0)) {
                bool all_digits = true;
                for (char c : norm) {
                    if (!std::isdigit(static_cast<unsigned char>(c))) { all_digits = false; break; }
                }
                if (all_digits) {
                    std::string key = "year_" + norm + "_" + std::to_string(chunk_idx);
                    if (seen_keys.find(key) == seen_keys.end()) {
                        seen_keys.insert(key);
                        RubricItem item;
                        item.item_id = items.size();
                        item.entity_type = RubricEntityType::DATE_OR_ERA;
                        item.raw_text = tokens[i];
                        item.normalized_text = norm;
                        item.ref_chunk_index = chunk_idx;
                        item.entity_label = "Year " + norm;
                        items.push_back(item);
                    }
                    continue;
                }
            }

            // 4. Check for Numeric / Percentage metrics (e.g. "50%", "180.000 euros")
            if (tokens[i].find('%') != std::string::npos || norm == "porciento") {
                std::string key = "pct_" + norm + "_" + std::to_string(chunk_idx);
                if (seen_keys.find(key) == seen_keys.end()) {
                    seen_keys.insert(key);
                    RubricItem item;
                    item.item_id = items.size();
                    item.entity_type = RubricEntityType::NUMERIC_METRIC;
                    item.raw_text = tokens[i];
                    item.normalized_text = norm;
                    item.ref_chunk_index = chunk_idx;
                    item.entity_label = "Metric " + tokens[i];
                    items.push_back(item);
                }
                continue;
            }

            // 5. Check for Scientific Terms (e.g. ADN, ATP, Hz)
            if (norm == "adn" || norm == "arn" || norm == "atp" || norm == "mitocondria" || norm == "hz" || norm == "isótopo" || norm == "isotopo") {
                std::string key = "sci_" + norm + "_" + std::to_string(chunk_idx);
                if (seen_keys.find(key) == seen_keys.end()) {
                    seen_keys.insert(key);
                    RubricItem item;
                    item.item_id = items.size();
                    item.entity_type = RubricEntityType::SCIENTIFIC_TERM;
                    item.raw_text = tokens[i];
                    item.normalized_text = norm;
                    item.ref_chunk_index = chunk_idx;
                    item.entity_label = "Term " + tokens[i];
                    items.push_back(item);
                }
                continue;
            }

            // 6. Check for Enumerated Point (e.g. "a)", "b)", "1º", "2º")
            if (tokens[i].length() <= 3 && (tokens[i].find(')') != std::string::npos || tokens[i].find("º") != std::string::npos)) {
                std::string key = "enum_" + norm + "_" + std::to_string(chunk_idx);
                if (seen_keys.find(key) == seen_keys.end()) {
                    seen_keys.insert(key);
                    RubricItem item;
                    item.item_id = items.size();
                    item.entity_type = RubricEntityType::ENUMERATED_POINT;
                    item.raw_text = tokens[i];
                    item.normalized_text = norm;
                    item.ref_chunk_index = chunk_idx;
                    item.entity_label = "Point " + tokens[i];
                    items.push_back(item);
                }
                continue;
            }

            // 7. Domain-specific high-priority citations
            if (domain_profile && domain_profile->is_high_priority_citation(tokens[i])) {
                std::string key = "dom_" + norm + "_" + std::to_string(chunk_idx);
                if (seen_keys.find(key) == seen_keys.end()) {
                    seen_keys.insert(key);
                    RubricItem item;
                    item.item_id = items.size();
                    item.entity_type = RubricEntityType::DOMAIN_KEYWORD;
                    item.raw_text = tokens[i];
                    item.normalized_text = norm;
                    item.ref_chunk_index = chunk_idx;
                    item.entity_label = "Domain " + tokens[i];
                    items.push_back(item);
                }
                continue;
            }
        }
    }

    return items;
}

RubricScorecard RubricChecklistExtractor::evaluate_rubric(
    const std::vector<RubricItem>& rubric_items,
    const std::vector<std::string>& transcript_segments,
    bool enable_phonetic
) {
    RubricScorecard scorecard;
    scorecard.total_items = rubric_items.size();
    scorecard.items = rubric_items;

    if (scorecard.total_items == 0) {
        scorecard.citation_accuracy_pct = 100.0f;
        return scorecard;
    }

    // Pre-tokenize and normalize transcript segments
    struct NormalizedSegment {
        size_t index;
        std::string full_normalized;
        std::vector<std::string> words_norm;
    };

    std::vector<NormalizedSegment> norm_segments;
    norm_segments.reserve(transcript_segments.size());
    for (size_t idx = 0; idx < transcript_segments.size(); ++idx) {
        NormalizedSegment ns;
        ns.index = idx;
        auto words = WordSequenceAligner::tokenize(transcript_segments[idx]);
        for (const auto& w : words) {
            std::string nw = WordSequenceAligner::normalize_word(w);
            if (!nw.empty()) {
                ns.words_norm.push_back(nw);
                if (!ns.full_normalized.empty()) ns.full_normalized += " ";
                ns.full_normalized += nw;
            }
        }
        norm_segments.push_back(ns);
    }

    auto phonetic_matcher = enable_phonetic ? PhoneticMatcherManager::getInstance().get_active_matcher() : nullptr;

    for (auto& item : scorecard.items) {
        auto item_tokens = WordSequenceAligner::tokenize(item.normalized_text);
        if (item_tokens.empty()) continue;

        bool satisfied = false;
        int matched_idx = -1;
        float confidence = 0.0f;

        for (const auto& seg : norm_segments) {
            // Pass 1: Direct multi-word substring match
            if (seg.full_normalized.find(item.normalized_text) != std::string::npos) {
                satisfied = true;
                matched_idx = static_cast<int>(seg.index);
                confidence = 1.0f;
                break;
            }

            // Pass 2: All sub-tokens present in the same segment
            bool all_tokens_found = true;
            for (const auto& it_word : item_tokens) {
                bool word_found = false;
                for (const auto& seg_word : seg.words_norm) {
                    if (it_word == seg_word) {
                        word_found = true;
                        break;
                    }
                    // Pass 3: Phonetic compensation check
                    if (phonetic_matcher) {
                        auto p_res = phonetic_matcher->compare_words(it_word, seg_word);
                        if (p_res.is_match) {
                            word_found = true;
                            break;
                        }
                    }
                }
                if (!word_found) {
                    all_tokens_found = false;
                    break;
                }
            }

            if (all_tokens_found) {
                satisfied = true;
                matched_idx = static_cast<int>(seg.index);
                confidence = 0.90f;
                break;
            }
        }

        if (satisfied) {
            item.is_satisfied = true;
            item.matched_transcript_index = matched_idx;
            item.match_confidence = confidence;
            scorecard.satisfied_items++;
        } else {
            item.is_satisfied = false;
            item.matched_transcript_index = -1;
            item.match_confidence = 0.0f;
            scorecard.omitted_items++;
        }
    }

    if (scorecard.total_items > 0) {
        scorecard.citation_accuracy_pct = (static_cast<float>(scorecard.satisfied_items) / static_cast<float>(scorecard.total_items)) * 100.0f;
    }

    return scorecard;
}
