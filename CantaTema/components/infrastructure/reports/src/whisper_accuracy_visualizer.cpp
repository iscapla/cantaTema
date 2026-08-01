#include "reports/whisper_accuracy_visualizer.hpp"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

std::string WhisperAccuracyVisualizer::escape_html(const std::string& str) {
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

std::string WhisperAccuracyVisualizer::format_timestamp(uint64_t ms) {
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

rst_code_e WhisperAccuracyVisualizer::generate_html(const WhisperAccuracyInput& input, std::string& out_html_content) const {
    std::ostringstream ss;

    // Get current date time string for top comment
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

    // 1. Comment at the beginning with all conversion information
    ss << "<!--\n"
       << "================================================================\n"
       << " CANTA TEMA - WHISPER TRANSCRIPTION ACCURACY REPORT\n"
       << "================================================================\n"
       << " Source Audio File    : " << input.audio_filepath << "\n"
       << " Whisper Model Used   : " << input.model_name << "\n"
       << " Language             : " << input.language << "\n"
       << " Total Audio Duration : " << format_timestamp(input.total_duration_ms) << " (" << input.total_duration_ms << " ms)\n"
       << " Processing Time      : " << input.processing_time_ms << " ms\n"
       << " Speech Rate (WPM)    : " << std::fixed << std::setprecision(1) << input.speech_rate_wpm << " WPM\n"
       << " Clarity Score        : " << std::fixed << std::setprecision(1) << input.clarity_score << " / 100\n"
       << " Overall Confidence   : " << std::fixed << std::setprecision(1) << (input.overall_confidence * 100.0f) << " %\n"
       << " Total Segments       : " << input.segments.size() << "\n"
       << " High Conf Threshold  : >= " << std::fixed << std::setprecision(1) << (input.config.high_confidence_threshold * 100.0f) << " %\n"
       << " Medium Conf Threshold: >= " << std::fixed << std::setprecision(1) << (input.config.medium_confidence_threshold * 100.0f) << " %\n"
       << " Low Conf Threshold   : <  " << std::fixed << std::setprecision(1) << (input.config.medium_confidence_threshold * 100.0f) << " %\n"
       << " Generated Date/Time  : " << date_buf << "\n"
       << "================================================================\n"
       << "-->\n";

    // 2. HTML Document Structure with CSS Styling
    ss << "<!DOCTYPE html>\n"
       << "<html lang=\"en\">\n"
       << "<head>\n"
       << "  <meta charset=\"UTF-8\">\n"
       << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
       << "  <title>Whisper Transcription Accuracy Report</title>\n"
       << "  <style>\n"
       << "    :root {\n"
       << "      --bg-color: #0f172a;\n"
       << "      --card-bg: #1e293b;\n"
       << "      --text-color: #f8fafc;\n"
       << "      --text-muted: #94a3b8;\n"
       << "      --border-color: #334155;\n"
       << "      --lvl-high-bg: rgba(16, 185, 129, 0.25);\n"
       << "      --lvl-high-color: #34d399;\n"
       << "      --lvl-high-border: #10b981;\n"
       << "      --lvl-med-bg: rgba(245, 158, 11, 0.25);\n"
       << "      --lvl-med-color: #fbbf24;\n"
       << "      --lvl-med-border: #f59e0b;\n"
       << "      --lvl-low-bg: rgba(239, 68, 68, 0.25);\n"
       << "      --lvl-low-color: #f87171;\n"
       << "      --lvl-low-border: #ef4444;\n"
       << "    }\n"
       << "    body {\n"
       << "      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;\n"
       << "      background-color: var(--bg-color);\n"
       << "      color: var(--text-color);\n"
       << "      margin: 0;\n"
       << "      padding: 24px;\n"
       << "      line-height: 1.6;\n"
       << "    }\n"
       << "    .container {\n"
       << "      max-width: 1000px;\n"
       << "      margin: 0 auto;\n"
       << "    }\n"
       << "    .header-card {\n"
       << "      background-color: var(--card-bg);\n"
       << "      border: 1px solid var(--border-color);\n"
       << "      border-radius: 12px;\n"
       << "      padding: 24px;\n"
       << "      margin-bottom: 24px;\n"
       << "      box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.3);\n"
       << "    }\n"
       << "    .header-card h1 {\n"
       << "      margin-top: 0;\n"
       << "      font-size: 24px;\n"
       << "      color: #38bdf8;\n"
       << "      border-bottom: 1px solid var(--border-color);\n"
       << "      padding-bottom: 12px;\n"
       << "    }\n"
       << "    .metrics-grid {\n"
       << "      display: grid;\n"
       << "      grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));\n"
       << "      gap: 16px;\n"
       << "      margin-top: 16px;\n"
       << "    }\n"
       << "    .metric-item {\n"
       << "      background: rgba(15, 23, 42, 0.6);\n"
       << "      padding: 12px 16px;\n"
       << "      border-radius: 8px;\n"
       << "      border: 1px solid var(--border-color);\n"
       << "    }\n"
       << "    .metric-label {\n"
       << "      font-size: 12px;\n"
       << "      color: var(--text-muted);\n"
       << "      text-transform: uppercase;\n"
       << "      letter-spacing: 0.5px;\n"
       << "    }\n"
       << "    .metric-value {\n"
       << "      font-size: 18px;\n"
       << "      font-weight: 600;\n"
       << "      margin-top: 4px;\n"
       << "      color: #f1f5f9;\n"
       << "    }\n"
       << "    .legend {\n"
       << "      display: flex;\n"
       << "      gap: 16px;\n"
       << "      margin-bottom: 16px;\n"
       << "      flex-wrap: wrap;\n"
       << "    }\n"
       << "    .legend-badge {\n"
       << "      padding: 6px 12px;\n"
       << "      border-radius: 6px;\n"
       << "      font-size: 13px;\n"
       << "      font-weight: 500;\n"
       << "      display: flex;\n"
       << "      align-items: center;\n"
       << "      gap: 6px;\n"
       << "    }\n"
       << "    .badge-high {\n"
       << "      background-color: var(--lvl-high-bg);\n"
       << "      color: var(--lvl-high-color);\n"
       << "      border: 1px solid var(--lvl-high-border);\n"
       << "    }\n"
       << "    .badge-med {\n"
       << "      background-color: var(--lvl-med-bg);\n"
       << "      color: var(--lvl-med-color);\n"
       << "      border: 1px solid var(--lvl-med-border);\n"
       << "    }\n"
       << "    .badge-low {\n"
       << "      background-color: var(--lvl-low-bg);\n"
       << "      color: var(--lvl-low-color);\n"
       << "      border: 1px solid var(--lvl-low-border);\n"
       << "    }\n"
       << "    .transcript-body {\n"
       << "      background-color: var(--card-bg);\n"
       << "      border: 1px solid var(--border-color);\n"
       << "      border-radius: 12px;\n"
       << "      padding: 24px;\n"
       << "      line-height: 2.0;\n"
       << "      font-size: 16px;\n"
       << "    }\n"
       << "    .seg-span {\n"
       << "      display: inline-block;\n"
       << "      padding: 2px 6px;\n"
       << "      margin: 3px 2px;\n"
       << "      border-radius: 4px;\n"
       << "      cursor: pointer;\n"
       << "      transition: transform 0.15s ease, box-shadow 0.15s ease;\n"
       << "      position: relative;\n"
       << "    }\n"
       << "    .seg-span:hover {\n"
       << "      transform: translateY(-1px);\n"
       << "      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.4);\n"
       << "      z-index: 10;\n"
       << "    }\n"
       << "    .lvl-high {\n"
       << "      background-color: var(--lvl-high-bg);\n"
       << "      color: var(--lvl-high-color);\n"
       << "      border: 1px solid var(--lvl-high-border);\n"
       << "    }\n"
       << "    .lvl-medium {\n"
       << "      background-color: var(--lvl-med-bg);\n"
       << "      color: var(--lvl-med-color);\n"
       << "      border: 1px solid var(--lvl-med-border);\n"
       << "    }\n"
       << "    .lvl-low {\n"
       << "      background-color: var(--lvl-low-bg);\n"
       << "      color: var(--lvl-low-color);\n"
       << "      border: 1px solid var(--lvl-low-border);\n"
       << "    }\n"
       << "    .seg-time {\n"
       << "      font-size: 11px;\n"
       << "      opacity: 0.75;\n"
       << "      margin-right: 4px;\n"
       << "    }\n"
       << "  </style>\n"
       << "</head>\n"
       << "<body>\n"
       << "  <div class=\"container\">\n"
       << "    <div class=\"header-card\">\n"
       << "      <h1>🎙️ Whisper Transcription Accuracy Report</h1>\n"
       << "      <div class=\"metrics-grid\">\n"
       << "        <div class=\"metric-item\">\n"
       << "          <div class=\"metric-label\">Audio File</div>\n"
       << "          <div class=\"metric-value\" title=\"" << escape_html(input.audio_filepath) << "\">"
       << escape_html(input.audio_filepath.size() > 25 ? "..." + input.audio_filepath.substr(input.audio_filepath.size() - 22) : input.audio_filepath)
       << "</div>\n"
       << "        </div>\n"
       << "        <div class=\"metric-item\">\n"
       << "          <div class=\"metric-label\">Whisper Model</div>\n"
       << "          <div class=\"metric-value\">" << escape_html(input.model_name) << "</div>\n"
       << "        </div>\n"
       << "        <div class=\"metric-item\">\n"
       << "          <div class=\"metric-label\">Audio Duration</div>\n"
       << "          <div class=\"metric-value\">" << format_timestamp(input.total_duration_ms) << "</div>\n"
       << "        </div>\n"
       << "        <div class=\"metric-item\">\n"
       << "          <div class=\"metric-label\">Speech Rate</div>\n"
       << "          <div class=\"metric-value\">" << std::fixed << std::setprecision(1) << input.speech_rate_wpm << " WPM</div>\n"
       << "        </div>\n"
       << "        <div class=\"metric-item\">\n"
       << "          <div class=\"metric-label\">Overall Confidence</div>\n"
       << "          <div class=\"metric-value\" style=\"color: #34d399;\">" << std::fixed << std::setprecision(1) << (input.overall_confidence * 100.0f) << "%</div>\n"
       << "        </div>\n"
       << "        <div class=\"metric-item\">\n"
       << "          <div class=\"metric-label\">Clarity Score</div>\n"
       << "          <div class=\"metric-value\">" << std::fixed << std::setprecision(1) << input.clarity_score << " / 100</div>\n"
       << "        </div>\n"
       << "      </div>\n"
       << "    </div>\n"
       << "    <div class=\"legend\">\n"
       << "      <div class=\"legend-badge badge-high\">🟢 High Confidence (&ge; " << std::fixed << std::setprecision(0) << (input.config.high_confidence_threshold * 100.0f) << "%)</div>\n"
       << "      <div class=\"legend-badge badge-med\">🟡 Medium Confidence (" << std::fixed << std::setprecision(0) << (input.config.medium_confidence_threshold * 100.0f) << "% - " << std::fixed << std::setprecision(0) << (input.config.high_confidence_threshold * 100.0f - 1.0f) << "%)</div>\n"
       << "      <div class=\"legend-badge badge-low\">🔴 Low Confidence (&lt; " << std::fixed << std::setprecision(0) << (input.config.medium_confidence_threshold * 100.0f) << "%)</div>\n"
       << "    </div>\n"
       << "    <div class=\"transcript-body\">\n";

    for (size_t i = 0; i < input.segments.size(); ++i) {
        const auto& seg = input.segments[i];
        float conf = seg.confidence_score;
        std::string lvl_class = "lvl-low";
        if (conf >= input.config.high_confidence_threshold) {
            lvl_class = "lvl-high";
        } else if (conf >= input.config.medium_confidence_threshold) {
            lvl_class = "lvl-medium";
        }

        std::string time_str = format_timestamp(seg.start_time_ms) + " - " + format_timestamp(seg.end_time_ms);
        std::ostringstream title_ss;
        title_ss << "[" << time_str << "] Confidence: " << std::fixed << std::setprecision(1) << (conf * 100.0f)
                 << "% | Avg Logprob: " << std::setprecision(3) << seg.avg_logprob;

        ss << "      <span class=\"seg-span " << lvl_class << "\" title=\"" << escape_html(title_ss.str()) << "\">"
           << "<span class=\"seg-time\">[" << format_timestamp(seg.start_time_ms) << "]</span>"
           << escape_html(seg.text)
           << "</span>\n";
    }

    ss << "    </div>\n"
       << "  </div>\n"
       << "</body>\n"
       << "</html>\n";

    out_html_content = ss.str();
    return RST_OK;
}
