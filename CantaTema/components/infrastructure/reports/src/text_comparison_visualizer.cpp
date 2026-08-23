/**
 * @file text_comparison_visualizer.cpp
 * @brief Implementation of TextComparisonVisualizer dual-column HTML report generator.
 */

#include "reports/text_comparison_visualizer.hpp"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <cmath>

std::string TextComparisonVisualizer::escape_html(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (char c : str) {
        switch (c) {
            case '&':  result += "&amp;";  break;
            case '<':  result += "&lt;";   break;
            case '>':  result += "&gt;";   break;
            case '"':  result += "&quot;"; break;
            case '\'': result += "&#39;";  break;
            default:   result += c;        break;
        }
    }
    return result;
}

std::string TextComparisonVisualizer::format_timestamp(uint64_t ms) {
    uint64_t total_sec = ms / 1000;
    uint64_t min = total_sec / 60;
    uint64_t sec = total_sec % 60;
    uint64_t rem_ms = ms % 1000;

    std::ostringstream ss;
    ss << std::setfill('0') << std::setw(2) << min << ":"
       << std::setfill('0') << std::setw(2) << sec << "."
       << std::setfill('0') << std::setw(3) << rem_ms;
    return ss.str();
}

rst_code_e TextComparisonVisualizer::generate_html(const TextComparisonInput& input, std::string& out_html_content) const {
    std::ostringstream ss;

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm time_info{};
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&time_info, &now_c);
#else
    localtime_r(&now_c, &time_info);
#endif
    char date_buf[64];
    std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", &time_info);

    // 1. Comment at the beginning
    ss << "<!--\n"
       << "================================================================\n"
       << " CANTA TEMA - DUAL-COLUMN TEXT COMPARISON REPORT\n"
       << "================================================================\n"
       << " Document Title      : " << input.document_title << "\n"
       << " Reference Filepath  : " << input.reference_filepath << "\n"
       << " Audio Filepath      : " << input.audio_filepath << "\n"
       << " Whisper Model       : " << input.whisper_model << "\n"
       << " Llama Model         : " << input.llama_model << "\n"
       << " Overall Coverage    : " << std::fixed << std::setprecision(2) << input.overall_coverage_pct << " %\n"
       << " Total Ref Chunks    : " << input.total_ref_chunks << "\n"
       << " Mentioned Chunks    : " << input.mentioned_chunks << "\n"
       << " Not Clear Chunks    : " << input.not_clear_chunks << "\n"
       << " Not Mentioned Chunks: " << input.not_mentioned_chunks << "\n"
       << " Mentioned Threshold : >= " << std::fixed << std::setprecision(2) << input.threshold_mentioned << "\n"
       << " Not Clear Threshold : >= " << std::fixed << std::setprecision(2) << input.threshold_not_clear << "\n"
       << " Generated Date/Time : " << date_buf << "\n"
       << "================================================================\n"
       << "-->\n";

    // Diagnostic calculations
    float recall_score = static_cast<float>(input.overall_coverage_pct);
    float citation_score = input.rubric_scorecard.total_items > 0 ? input.rubric_scorecard.citation_accuracy_pct : 100.0f;
    float fluency_score = (input.diagnostic_scores.oral_fluency_score > 0.0f) ? input.diagnostic_scores.oral_fluency_score : 85.0f;
    float clarity_score = (input.diagnostic_scores.speech_clarity_score > 0.0f) ? input.diagnostic_scores.speech_clarity_score : 90.0f;
    float composite_score = (input.diagnostic_scores.overall_composite_score > 0.0f)
                            ? input.diagnostic_scores.overall_composite_score
                            : ((0.35f * recall_score) + (0.25f * citation_score) + (0.20f * fluency_score) + (0.20f * clarity_score));

    // Calculate 4-axis SVG points
    float cx = 90.0f;
    float cy = 80.0f;
    float r = 55.0f;

    float pt_recall_x = cx;
    float pt_recall_y = cy - (r * (recall_score / 100.0f));

    float pt_cit_x = cx + (r * (citation_score / 100.0f));
    float pt_cit_y = cy;

    float pt_flu_x = cx;
    float pt_flu_y = cy + (r * (fluency_score / 100.0f));

    float pt_cla_x = cx - (r * (clarity_score / 100.0f));
    float pt_cla_y = cy;

    // 2. HTML Document Structure with CSS & Vanilla JavaScript
    ss << "<!DOCTYPE html>\n"
       << "<html lang=\"en\">\n"
       << "<head>\n"
       << "  <meta charset=\"UTF-8\">\n"
       << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
       << "  <title>Dual-Column Text Comparison - " << escape_html(input.document_title) << "</title>\n"
       << "  <style>\n"
       << "    :root {\n"
       << "      --bg-color: #0b0f19;\n"
       << "      --card-bg: #151d2a;\n"
       << "      --card-hover: #1c2738;\n"
       << "      --text-color: #f8fafc;\n"
       << "      --text-muted: #94a3b8;\n"
       << "      --border-color: #273549;\n"
       << "      --lvl-mentioned-bg: rgba(16, 185, 129, 0.18);\n"
       << "      --lvl-mentioned-color: #34d399;\n"
       << "      --lvl-mentioned-border: #10b981;\n"
       << "      --lvl-notclear-bg: rgba(245, 158, 11, 0.18);\n"
       << "      --lvl-notclear-color: #fbbf24;\n"
       << "      --lvl-notclear-border: #f59e0b;\n"
       << "      --lvl-notmentioned-bg: rgba(239, 68, 68, 0.18);\n"
       << "      --lvl-notmentioned-color: #f87171;\n"
       << "      --lvl-notmentioned-border: #ef4444;\n"
       << "      --active-highlight-bg: rgba(56, 189, 248, 0.28);\n"
       << "      --active-highlight-border: #38bdf8;\n"
       << "    }\n"
       << "    * { box-sizing: border-box; }\n"
       << "    body {\n"
       << "      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;\n"
       << "      background-color: var(--bg-color);\n"
       << "      color: var(--text-color);\n"
       << "      margin: 0;\n"
       << "      padding: 0;\n"
       << "      height: 100vh;\n"
       << "      display: flex;\n"
       << "      flex-direction: column;\n"
       << "      overflow: hidden;\n"
       << "    }\n"
       << "    .top-bar {\n"
       << "      background-color: var(--card-bg);\n"
       << "      border-bottom: 1px solid var(--border-color);\n"
       << "      padding: 12px 24px;\n"
       << "      display: flex;\n"
       << "      justify-content: space-between;\n"
       << "      align-items: center;\n"
       << "      flex-wrap: wrap;\n"
       << "      gap: 16px;\n"
       << "    }\n"
       << "    .title-area h1 {\n"
       << "      margin: 0;\n"
       << "      font-size: 18px;\n"
       << "      color: #38bdf8;\n"
       << "    }\n"
       << "    .title-area p {\n"
       << "      margin: 4px 0 0 0;\n"
       << "      font-size: 12px;\n"
       << "      color: var(--text-muted);\n"
       << "    }\n"
       << "    .radar-stats-group {\n"
       << "      display: flex;\n"
       << "      align-items: center;\n"
       << "      gap: 16px;\n"
       << "    }\n"
       << "    .radar-container {\n"
       << "      background: rgba(11, 15, 25, 0.8);\n"
       << "      border: 1px solid var(--border-color);\n"
       << "      border-radius: 8px;\n"
       << "      padding: 4px 8px;\n"
       << "      display: flex;\n"
       << "      align-items: center;\n"
       << "    }\n"
       << "    .stats-area {\n"
       << "      display: flex;\n"
       << "      gap: 10px;\n"
       << "    }\n"
       << "    .stat-badge {\n"
       << "      padding: 6px 12px;\n"
       << "      border-radius: 8px;\n"
       << "      background: rgba(11, 15, 25, 0.7);\n"
       << "      border: 1px solid var(--border-color);\n"
       << "      font-size: 12px;\n"
       << "      display: flex;\n"
       << "      flex-direction: column;\n"
       << "      align-items: center;\n"
       << "    }\n"
       << "    .stat-badge .num {\n"
       << "      font-weight: 700;\n"
       << "      font-size: 15px;\n"
       << "    }\n"
       << "    .rubric-drawer {\n"
       << "      background-color: #111827;\n"
       << "      border-bottom: 1px solid var(--border-color);\n"
       << "      padding: 10px 24px;\n"
       << "      font-size: 12px;\n"
       << "    }\n"
       << "    .rubric-header {\n"
       << "      display: flex;\n"
       << "      justify-content: space-between;\n"
       << "      align-items: center;\n"
       << "      cursor: pointer;\n"
       << "      font-weight: 600;\n"
       << "      color: #93c5fd;\n"
       << "    }\n"
       << "    .rubric-filters {\n"
       << "      display: flex;\n"
       << "      gap: 8px;\n"
       << "      margin-top: 8px;\n"
       << "    }\n"
       << "    .rubric-btn {\n"
       << "      background: #1f2937;\n"
       << "      border: 1px solid var(--border-color);\n"
       << "      color: var(--text-color);\n"
       << "      padding: 4px 10px;\n"
       << "      border-radius: 6px;\n"
       << "      font-size: 11px;\n"
       << "      cursor: pointer;\n"
       << "    }\n"
       << "    .rubric-btn.active {\n"
       << "      background: #38bdf8;\n"
       << "      color: #0b0f19;\n"
       << "      font-weight: 700;\n"
       << "    }\n"
       << "    .rubric-list {\n"
       << "      display: flex;\n"
       << "      flex-wrap: wrap;\n"
       << "      gap: 8px;\n"
       << "      margin-top: 10px;\n"
       << "      max-height: 120px;\n"
       << "      overflow-y: auto;\n"
       << "    }\n"
       << "    .rubric-chip {\n"
       << "      padding: 4px 8px;\n"
       << "      border-radius: 6px;\n"
       << "      font-size: 11px;\n"
       << "      cursor: pointer;\n"
       << "      display: inline-flex;\n"
       << "      align-items: center;\n"
       << "      gap: 4px;\n"
       << "      border: 1px solid transparent;\n"
       << "    }\n"
       << "    .rubric-chip.satisfied {\n"
       << "      background: rgba(16, 185, 129, 0.15);\n"
       << "      color: #34d399;\n"
       << "      border-color: #10b981;\n"
       << "    }\n"
       << "    .rubric-chip.omitted {\n"
       << "      background: rgba(239, 68, 68, 0.15);\n"
       << "      color: #f87171;\n"
       << "      border-color: #ef4444;\n"
       << "    }\n"
       << "    .columns-wrapper {\n"
       << "      display: flex;\n"
       << "      flex: 1;\n"
       << "      overflow: hidden;\n"
       << "    }\n"
       << "    .column {\n"
       << "      flex: 1;\n"
       << "      display: flex;\n"
       << "      flex-direction: column;\n"
       << "      border-right: 1px solid var(--border-color);\n"
       << "      overflow: hidden;\n"
       << "    }\n"
       << "    .column:last-child {\n"
       << "      border-right: none;\n"
       << "    }\n"
       << "    .col-header {\n"
       << "      background: rgba(21, 29, 42, 0.95);\n"
       << "      padding: 12px 20px;\n"
       << "      font-weight: 600;\n"
       << "      font-size: 14px;\n"
       << "      border-bottom: 1px solid var(--border-color);\n"
       << "      display: flex;\n"
       << "      justify-content: space-between;\n"
       << "      align-items: center;\n"
       << "      color: #93c5fd;\n"
       << "    }\n"
       << "    .col-body {\n"
       << "      flex: 1;\n"
       << "      overflow-y: auto;\n"
       << "      padding: 20px;\n"
       << "      scroll-behavior: smooth;\n"
       << "    }\n"
       << "    .item-card {\n"
       << "      background-color: var(--card-bg);\n"
       << "      border: 1px solid var(--border-color);\n"
       << "      border-radius: 10px;\n"
       << "      padding: 14px 18px;\n"
       << "      margin-bottom: 14px;\n"
       << "      transition: all 0.2s ease;\n"
       << "      position: relative;\n"
       << "    }\n"
       << "    #ref-col .item-card {\n"
       << "      cursor: pointer;\n"
       << "    }\n"
       << "    #ref-col .item-card:hover {\n"
       << "      background-color: var(--card-hover);\n"
       << "      transform: translateX(4px);\n"
       << "      border-color: #38bdf8;\n"
       << "    }\n"
       << "    #transcript-col .item-card {\n"
       << "      cursor: default;\n"
       << "    }\n"
       << "    .lvl-mentioned {\n"
       << "      background-color: var(--lvl-mentioned-bg);\n"
       << "      border-left: 4px solid var(--lvl-mentioned-border);\n"
       << "    }\n"
       << "    .lvl-not-clear {\n"
       << "      background-color: var(--lvl-notclear-bg);\n"
       << "      border-left: 4px solid var(--lvl-notclear-border);\n"
       << "    }\n"
       << "    .lvl-not-mentioned {\n"
       << "      background-color: var(--lvl-notmentioned-bg);\n"
       << "      border-left: 4px solid var(--lvl-notmentioned-border);\n"
       << "    }\n"
       << "    #ref-col .item-card.selected-left,\n"
       << "    #ref-col .item-card.active-highlight {\n"
       << "      background-color: rgba(56, 189, 248, 0.25) !important;\n"
       << "      border: 2px solid #38bdf8 !important;\n"
       << "      box-shadow: 0 0 16px rgba(56, 189, 248, 0.5);\n"
       << "      transform: scale(1.015);\n"
       << "      z-index: 10;\n"
       << "    }\n"
       << "    @keyframes remark-pulse {\n"
       << "      0% { box-shadow: 0 0 10px rgba(245, 158, 11, 0.5); }\n"
       << "      50% { box-shadow: 0 0 24px rgba(245, 158, 11, 0.9); }\n"
       << "      100% { box-shadow: 0 0 10px rgba(245, 158, 11, 0.5); }\n"
       << "    }\n"
       << "    #transcript-col .item-card.remarked-right,\n"
       << "    #transcript-col .item-card.active-highlight {\n"
       << "      background-color: rgba(245, 158, 11, 0.25) !important;\n"
       << "      border: 2px solid #f59e0b !important;\n"
       << "      animation: remark-pulse 2s infinite ease-in-out;\n"
       << "      transform: scale(1.02);\n"
       << "      z-index: 10;\n"
       << "    }\n"
       << "    .card-meta {\n"
       << "      display: flex;\n"
       << "      justify-content: space-between;\n"
       << "      font-size: 11px;\n"
       << "      color: var(--text-muted);\n"
       << "      margin-bottom: 6px;\n"
       << "      font-weight: 500;\n"
       << "    }\n"
       << "    .card-text {\n"
       << "      font-size: 14px;\n"
       << "      line-height: 1.5;\n"
       << "      color: #f1f5f9;\n"
       << "    }\n"
       << "    .badge-tag {\n"
       << "      font-size: 10px;\n"
       << "      padding: 2px 6px;\n"
       << "      border-radius: 4px;\n"
       << "      font-weight: 600;\n"
       << "      text-transform: uppercase;\n"
       << "    }\n"
       << "  </style>\n"
       << "</head>\n"
       << "<body>\n"
       << "  <div class=\"top-bar\">\n"
       << "    <div class=\"title-area\">\n"
       << "      <h1>📊 Dual-Column Text Comparison Dashboard</h1>\n"
       << "      <p>Ref: " << escape_html(input.document_title) << " | Audio: " << escape_html(input.audio_filepath) << "</p>\n"
       << "    </div>\n"
       << "    <div class=\"radar-stats-group\">\n"
       << "      <!-- 4-AXIS SVG RADAR CHART -->\n"
       << "      <div class=\"radar-container\" title=\"4-Axis Diagnostic Radar (Recall, Citations, Fluency, Clarity)\">\n"
       << "        <svg width=\"180\" height=\"160\" viewBox=\"0 0 180 160\">\n"
       << "          <!-- Axis Rings -->\n"
       << "          <circle cx=\"90\" cy=\"80\" r=\"55\" fill=\"none\" stroke=\"#334155\" stroke-width=\"1\" stroke-dasharray=\"3,3\" />\n"
       << "          <circle cx=\"90\" cy=\"80\" r=\"28\" fill=\"none\" stroke=\"#1e293b\" stroke-width=\"1\" />\n"
       << "          <!-- Axis Cross Lines -->\n"
       << "          <line x1=\"90\" y1=\"25\" x2=\"90\" y2=\"135\" stroke=\"#334155\" stroke-width=\"1\" />\n"
       << "          <line x1=\"35\" y1=\"80\" x2=\"145\" y2=\"80\" stroke=\"#334155\" stroke-width=\"1\" />\n"
       << "          <!-- Candidate Radar Polygon -->\n"
       << "          <polygon points=\"" << pt_recall_x << "," << pt_recall_y << " "
                                      << pt_cit_x << "," << pt_cit_y << " "
                                      << pt_flu_x << "," << pt_flu_y << " "
                                      << pt_cla_x << "," << pt_cla_y << "\"\n"
       << "                   fill=\"rgba(56, 189, 248, 0.45)\" stroke=\"#38bdf8\" stroke-width=\"2\" />\n"
       << "          <!-- Axis Text Labels -->\n"
       << "          <text x=\"90\" y=\"18\" fill=\"#38bdf8\" font-size=\"9\" text-anchor=\"middle\" font-weight=\"bold\">Recall " << std::fixed << std::setprecision(0) << recall_score << "%</text>\n"
       << "          <text x=\"150\" y=\"83\" fill=\"#34d399\" font-size=\"9\" text-anchor=\"start\" font-weight=\"bold\">Cit " << std::setprecision(0) << citation_score << "%</text>\n"
       << "          <text x=\"90\" y=\"150\" fill=\"#fbbf24\" font-size=\"9\" text-anchor=\"middle\" font-weight=\"bold\">Fluency " << std::setprecision(0) << fluency_score << "%</text>\n"
       << "          <text x=\"30\" y=\"83\" fill=\"#a78bfa\" font-size=\"9\" text-anchor=\"end\" font-weight=\"bold\">Clarity " << std::setprecision(0) << clarity_score << "%</text>\n"
       << "        </svg>\n"
       << "      </div>\n"
       << "      <div class=\"stats-area\">\n"
       << "        <div class=\"stat-badge\" style=\"border-color: #38bdf8;\">\n"
       << "          <span class=\"num\" style=\"color: #38bdf8;\">" << std::fixed << std::setprecision(1) << input.overall_coverage_pct << "%</span>\n"
       << "          <span>Coverage</span>\n"
       << "        </div>\n"
       << "        <div class=\"stat-badge\" style=\"border-color: #818cf8;\">\n"
       << "          <span class=\"num\" style=\"color: #a5b4fc;\">" << std::fixed << std::setprecision(1) << composite_score << "%</span>\n"
       << "          <span>Composite</span>\n"
       << "        </div>\n"
       << "        <div class=\"stat-badge\" style=\"border-color: #10b981;\">\n"
       << "          <span class=\"num\" style=\"color: #34d399;\">" << input.mentioned_chunks << "</span>\n"
       << "          <span>Mentioned</span>\n"
       << "        </div>\n"
       << "        <div class=\"stat-badge\" style=\"border-color: #f59e0b;\">\n"
       << "          <span class=\"num\" style=\"color: #fbbf24;\">" << input.not_clear_chunks << "</span>\n"
       << "          <span>Not Clear</span>\n"
       << "        </div>\n"
       << "        <div class=\"stat-badge\" style=\"border-color: #ef4444;\">\n"
       << "          <span class=\"num\" style=\"color: #f87171;\">" << input.not_mentioned_chunks << "</span>\n"
       << "          <span>Omitted</span>\n"
       << "        </div>\n"
       << "      </div>\n"
       << "    </div>\n"
       << "  </div>\n";

    // Rubric Checklist Drawer
    if (input.rubric_scorecard.total_items > 0) {
        ss << "  <div class=\"rubric-drawer\">\n"
           << "    <div class=\"rubric-header\" id=\"rubric-toggle-btn\">\n"
           << "      <span>📋 Exam Rubric Checklist (" << input.rubric_scorecard.total_items << " Entities: "
           << input.rubric_scorecard.satisfied_items << " Verified [✓], " << input.rubric_scorecard.omitted_items << " Omitted [✗])</span>\n"
           << "      <span id=\"rubric-arrow\">▼</span>\n"
           << "    </div>\n"
           << "    <div id=\"rubric-content\">\n"
           << "      <div class=\"rubric-filters\">\n"
           << "        <button class=\"rubric-btn active\" data-filter=\"all\">All (" << input.rubric_scorecard.total_items << ")</button>\n"
           << "        <button class=\"rubric-btn\" data-filter=\"satisfied\">✓ Verified (" << input.rubric_scorecard.satisfied_items << ")</button>\n"
           << "        <button class=\"rubric-btn\" data-filter=\"omitted\">✗ Omitted (" << input.rubric_scorecard.omitted_items << ")</button>\n"
           << "      </div>\n"
           << "      <div class=\"rubric-list\">\n";

        for (const auto& item : input.rubric_scorecard.items) {
            std::string status_cls = item.is_satisfied ? "satisfied" : "omitted";
            std::string icon = item.is_satisfied ? "✓" : "✗";
            ss << "        <span class=\"rubric-chip " << status_cls << "\" data-status=\"" << status_cls
               << "\" data-ref-chunk=\"" << item.ref_chunk_index << "\">"
               << icon << " " << escape_html(item.entity_label) << "</span>\n";
        }

        ss << "      </div>\n"
           << "    </div>\n"
           << "  </div>\n";
    }

    ss << "  <div class=\"columns-wrapper\">\n"
       << "    <!-- LEFT COLUMN: REFERENCE DOCUMENT -->\n"
       << "    <div class=\"column\">\n"
       << "      <div class=\"col-header\">\n"
       << "        <span>📄 Reference Document</span>\n"
       << "        <span style=\"font-size: 12px; opacity: 0.8;\">" << input.reference_items.size() << " Chunks</span>\n"
       << "      </div>\n"
       << "      <div class=\"col-body\" id=\"ref-col\">\n";

    for (const auto& item : input.reference_items) {
        std::string lvl_cls = "lvl-not-mentioned";
        std::string status_label = "NOT MENTIONED";
        std::string status_color = "#f87171";
        if (item.coverage_status == coverage_level_e::MENTIONED) {
            lvl_cls = "lvl-mentioned";
            status_label = "MENTIONED";
            status_color = "#34d399";
        } else if (item.coverage_status == coverage_level_e::NOT_CLEAR) {
            lvl_cls = "lvl-not-clear";
            status_label = "NOT CLEAR";
            status_color = "#fbbf24";
        }

        std::string text_html;
        if (!item.word_alignment.reference_words.empty()) {
            std::ostringstream text_ss;
            for (size_t k = 0; k < item.word_alignment.reference_words.size(); ++k) {
                const auto& token = item.word_alignment.reference_words[k];
                if (token.status == WordDiffStatus::MATCHED) {
                    text_ss << "<span style=\"color: #34d399; font-weight: 500;\">" << escape_html(token.original_word) << "</span>";
                } else if (token.status == WordDiffStatus::PHONETIC_MISPRONUNCIATION) {
                    text_ss << "<span style=\"color: #fbbf24; text-decoration: underline wavy #f59e0b; background-color: rgba(245, 158, 11, 0.18); padding: 0 3px; border-radius: 3px;\" title=\"Phonetic match / Minor speech mispronunciation\">" << escape_html(token.original_word) << "</span>";
                } else if (token.status == WordDiffStatus::SEMANTIC_EQUIVALENCE) {
                    std::string tip = token.equivalent_phrase.empty() ? "Semantic Equivalence / Valid Paraphrase" : "Semantic Equivalence: Spoken as '" + escape_html(token.equivalent_phrase) + "'";
                    text_ss << "<span style=\"color: #60a5fa; text-decoration: underline dotted #3b82f6; background-color: rgba(59, 130, 246, 0.18); padding: 0 3px; border-radius: 3px; font-weight: 500;\" title=\"" << tip << "\">" << escape_html(token.original_word) << "</span>";
                } else if (token.is_legal_citation) {
                    text_ss << "⚠️ <span style=\"color: #ef4444; font-weight: 700; text-decoration: line-through; background-color: rgba(239, 68, 68, 0.25); padding: 0 4px; border-radius: 3px;\" title=\"Legal Article/Citation missing in spoken audio!\">" << escape_html(token.original_word) << "</span>";
                } else {
                    text_ss << "<span style=\"color: #f87171; text-decoration: line-through; background-color: rgba(239, 68, 68, 0.18); padding: 0 3px; border-radius: 3px;\" title=\"Word missing in spoken audio\">" << escape_html(token.original_word) << "</span>";
                }
                if (k + 1 < item.word_alignment.reference_words.size()) {
                    text_ss << " ";
                }
            }
            text_html = text_ss.str();
        } else {
            text_html = escape_html(item.text);
        }

        std::string citation_badge;
        if (item.word_alignment.has_missing_legal_citation) {
            std::string badge_label = input.active_domain_badge.empty() ? "⚠️ MISSING CITATION" : input.active_domain_badge;
            citation_badge = " <span class=\"badge-tag\" style=\"color: #ef4444; border: 1px solid #ef4444; background: rgba(239, 68, 68, 0.15);\">" + escape_html(badge_label) + "</span>";
        }

        float recall_display = (item.word_alignment.total_reference_weight > 0.0f) ? item.word_alignment.weighted_recall_score : item.word_recall_score;

        ss << "        <div class=\"item-card " << lvl_cls << "\" data-ref-id=\"" << item.id << "\" data-match-ts-idx=\"" << item.matched_transcript_index << "\">\n"
           << "          <div class=\"card-meta\">\n"
           << "            <span>Ref Chunk #" << (item.id + 1) << " (Weight: " << std::fixed << std::setprecision(1) << item.importance_weight << ")" << citation_badge << "</span>\n"
           << "            <span class=\"badge-tag\" style=\"color: " << status_color << "; border: 1px solid " << status_color << ";\">" << status_label << " (Sim: " << std::setprecision(0) << (item.similarity_score * 100.0f) << "% | W-Recall: " << (recall_display * 100.0f) << "%)</span>\n"
           << "          </div>\n"
           << "          <div class=\"card-text\">" << text_html << "</div>\n"
           << "        </div>\n";
    }

    ss << "      </div>\n"
       << "    </div>\n"
       << "    <!-- RIGHT COLUMN: VOICE TRANSCRIPT -->\n"
       << "    <div class=\"column\">\n"
       << "      <div class=\"col-header\">\n"
       << "        <span>🎙️ Voice Transcript</span>\n"
       << "        <span style=\"font-size: 12px; opacity: 0.8;\">" << input.transcript_items.size() << " Segments</span>\n"
       << "      </div>\n"
       << "      <div class=\"col-body\" id=\"transcript-col\">\n";

    for (const auto& ts : input.transcript_items) {
        ss << "        <div class=\"item-card\" data-ts-id=\"" << ts.id << "\" data-match-ref-idx=\"" << ts.primary_matched_ref_index << "\">\n"
           << "          <div class=\"card-meta\">\n"
           << "            <span>Voice Segment #" << (ts.id + 1) << " [" << format_timestamp(ts.start_time_ms) << " - " << format_timestamp(ts.end_time_ms) << "]</span>\n"
           << "            <span>Conf: " << std::fixed << std::setprecision(1) << (ts.confidence_score * 100.0f) << "%</span>\n"
           << "          </div>\n"
           << "          <div class=\"card-text\">" << escape_html(ts.text) << "</div>\n"
           << "        </div>\n";
    }

    ss << "      </div>\n"
       << "    </div>\n"
       << "  </div>\n"
       << "  <!-- Embedded Script for Left-side Selection, Focus & Rubric Filtering -->\n"
       << "  <script>\n"
       << "    document.addEventListener('DOMContentLoaded', () => {\n"
       << "      const tsCol = document.getElementById('transcript-col');\n"
       << "      const refCards = document.querySelectorAll('#ref-col .item-card');\n"
       << "\n"
       << "      function clearSelection() {\n"
       << "        document.querySelectorAll('.active-highlight, .selected-left, .remarked-right').forEach(el => {\n"
       << "          el.classList.remove('active-highlight', 'selected-left', 'remarked-right');\n"
       << "        });\n"
       << "      }\n"
       << "\n"
       << "      // Selection Interaction\n"
       << "      refCards.forEach(card => {\n"
       << "        card.addEventListener('click', () => {\n"
       << "          const isAlreadySelected = card.classList.contains('selected-left');\n"
       << "          clearSelection();\n"
       << "\n"
       << "          if (!isAlreadySelected) {\n"
       << "            card.classList.add('selected-left', 'active-highlight');\n"
       << "            const tsIdx = card.getAttribute('data-match-ts-idx');\n"
       << "            if (tsIdx !== null && tsIdx !== '-1') {\n"
       << "              const targetTs = document.querySelector(`#transcript-col .item-card[data-ts-id=\"${tsIdx}\"]`);\n"
       << "              if (targetTs) {\n"
       << "                targetTs.classList.add('remarked-right', 'active-highlight');\n"
       << "                const targetRect = targetTs.getBoundingClientRect();\n"
       << "                const containerRect = tsCol.getBoundingClientRect();\n"
       << "                const relativeTop = targetRect.top - containerRect.top + tsCol.scrollTop;\n"
       << "                const scrollToPos = Math.max(0, relativeTop - (tsCol.clientHeight / 2) + (targetTs.offsetHeight / 2));\n"
       << "                tsCol.scrollTo({ top: scrollToPos, behavior: 'smooth' });\n"
       << "              }\n"
       << "            }\n"
       << "          }\n"
       << "        });\n"
       << "      });\n"
       << "\n"
       << "      // Rubric Filter Buttons & Chunk Seeking\n"
       << "      const filterBtns = document.querySelectorAll('.rubric-btn');\n"
       << "      const chips = document.querySelectorAll('.rubric-chip');\n"
       << "      filterBtns.forEach(btn => {\n"
       << "        btn.addEventListener('click', () => {\n"
       << "          filterBtns.forEach(b => b.classList.remove('active'));\n"
       << "          btn.classList.add('active');\n"
       << "          const f = btn.getAttribute('data-filter');\n"
       << "          chips.forEach(chip => {\n"
       << "            if (f === 'all' || chip.getAttribute('data-status') === f) {\n"
       << "              chip.style.display = 'inline-flex';\n"
       << "            } else {\n"
       << "              chip.style.display = 'none';\n"
       << "            }\n"
       << "          });\n"
       << "        });\n"
       << "      });\n"
       << "\n"
       << "      chips.forEach(chip => {\n"
       << "        chip.addEventListener('click', () => {\n"
       << "          const refChunkId = chip.getAttribute('data-ref-chunk');\n"
       << "          if (refChunkId !== null) {\n"
       << "            const targetCard = document.querySelector(`#ref-col .item-card[data-ref-id=\"${refChunkId}\"]`);\n"
       << "            if (targetCard) {\n"
       << "              targetCard.click();\n"
       << "              targetCard.scrollIntoView({ behavior: 'smooth', block: 'center' });\n"
       << "            }\n"
       << "          }\n"
       << "        });\n"
       << "      });\n"
       << "    });\n"
       << "  </script>\n"
       << "</body>\n"
       << "</html>\n";

    out_html_content = ss.str();
    return RST_OK;
}
