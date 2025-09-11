#include "llm_behavior_analyzer.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>
#include <nlohmann/json.hpp>

// For HTTP requests to LLM APIs
#include <curl/curl.h>
#include <openssl/ssl.h>

// For local model support
#ifdef USE_LOCAL_MODELS
#include <torch/torch.h>
#include <transformers/models/auto/modeling_auto.h>
#include <transformers/models/auto/tokenization_auto.h>
#endif

using json = nlohmann::json;

namespace {
    // cURL write callback
    size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
        size_t total_size = size * nmemb;
        output->append(static_cast<char*>(contents), total_size);
        return total_size;
    }

    // Helper function to get current timestamp as string
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
}

LLMBehaviorAnalyzer::LLMBehaviorAnalyzer()
    : running_(false),
      real_time_enabled_(false),
      llm_provider_("openai"),
      openai_model_("gpt-4"),
      anthropic_model_("claude-3-sonnet-20240229"),
      analysis_interval_(300) {  // 5 minutes default
}

LLMBehaviorAnalyzer::~LLMBehaviorAnalyzer() {
    stopAnalysis();
}

void LLMBehaviorAnalyzer::setAPIKey(const std::string& provider, const std::string& api_key) {
    if (provider == "openai") {
        openai_api_key_ = api_key;
    } else if (provider == "anthropic") {
        anthropic_api_key_ = api_key;
    }
}

void LLMBehaviorAnalyzer::setModel(const std::string& provider, const std::string& model) {
    if (provider == "openai") {
        openai_model_ = model;
    } else if (provider == "anthropic") {
        anthropic_model_ = model;
    }
}

void LLMBehaviorAnalyzer::setAnalysisInterval(int seconds) {
    analysis_interval_ = seconds;
}

void LLMBehaviorAnalyzer::enableRealTimeAnalysis(bool enable) {
    real_time_enabled_ = enable;
}

void LLMBehaviorAnalyzer::startAnalysis() {
    if (running_) return;

    running_ = true;
    analysis_thread_ = std::thread(&LLMBehaviorAnalyzer::analysisLoop, this);
}

void LLMBehaviorAnalyzer::stopAnalysis() {
    running_ = false;
    if (analysis_thread_.joinable()) {
        analysis_thread_.join();
    }
}

bool LLMBehaviorAnalyzer::isRunning() const {
    return running_;
}

void LLMBehaviorAnalyzer::analyzeUserBehavior(const std::string& user_id,
                                            const std::vector<std::string>& activities,
                                            const std::unordered_map<std::string, double>& metrics) {
    std::lock_guard<std::mutex> lock(data_mutex_);

    // Update user context
    if (user_contexts_.find(user_id) == user_contexts_.end()) {
        user_contexts_[user_id] = UserBehaviorContext{user_id, {}, {}, {}, std::chrono::system_clock::now()};
    }

    auto& context = user_contexts_[user_id];
    context.recent_activities.insert(context.recent_activities.end(),
                                   activities.begin(), activities.end());
    context.behavior_metrics = metrics;
    context.last_analysis = std::chrono::system_clock::now();

    // Keep only recent activities (last 100)
    if (context.recent_activities.size() > 100) {
        context.recent_activities.erase(
            context.recent_activities.begin(),
            context.recent_activities.end() - 100
        );
    }

    // Add to analysis queue if real-time analysis is enabled
    if (real_time_enabled_) {
        pending_analyses_.push_back(user_id);
    }
}

void LLMBehaviorAnalyzer::addBehaviorData(const std::string& user_id, const std::string& activity) {
    std::lock_guard<std::mutex> lock(data_mutex_);

    if (user_contexts_.find(user_id) == user_contexts_.end()) {
        user_contexts_[user_id] = UserBehaviorContext{user_id, {}, {}, {}, std::chrono::system_clock::now()};
    }

    user_contexts_[user_id].recent_activities.push_back(activity);

    // Keep only recent activities
    if (user_contexts_[user_id].recent_activities.size() > 100) {
        user_contexts_[user_id].recent_activities.erase(
            user_contexts_[user_id].recent_activities.begin()
        );
    }
}

void LLMBehaviorAnalyzer::analyzeRiskPatterns(const std::string& user_id) {
    if (user_contexts_.find(user_id) == user_contexts_.end()) {
        return;
    }

    std::string prompt = buildAnalysisPrompt(user_id);
    std::string response;

    try {
        if (llm_provider_ == "openai") {
            response = analyzeWithOpenAI(prompt);
        } else if (llm_provider_ == "anthropic") {
            response = analyzeWithAnthropic(prompt);
        } else if (llm_provider_ == "local") {
            response = analyzeWithLocalModel(prompt);
        }

        if (!response.empty()) {
            LLMBehaviorInsight insight = parseLLMResponse(response, user_id);
            storeInsight(insight);

            if (insight_callback_) {
                insight_callback_(insight);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "LLM analysis error for user " << user_id << ": " << e.what() << std::endl;
    }
}

void LLMBehaviorAnalyzer::generateSecurityRecommendations(const std::string& user_id) {
    if (user_contexts_.find(user_id) == user_contexts_.end()) {
        return;
    }

    std::string prompt = R"(
Based on the following user behavior data, generate specific security recommendations:

User: )" + user_id + R"(
Behavior Data: )" + formatBehaviorData(user_id) + R"(

Please provide:
1. Specific security recommendations
2. Risk mitigation strategies
3. Monitoring suggestions
4. Policy adjustments if needed

Format as JSON with keys: recommendations, risk_level, actions
)";

    try {
        std::string response;
        if (llm_provider_ == "openai") {
            response = analyzeWithOpenAI(prompt);
        } else if (llm_provider_ == "anthropic") {
            response = analyzeWithAnthropic(prompt);
        }

        if (!response.empty()) {
            LLMBehaviorInsight insight = parseLLMResponse(response, user_id);
            insight.insight_type = "recommendation";
            storeInsight(insight);

            if (insight_callback_) {
                insight_callback_(insight);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Recommendation generation error for user " << user_id << ": " << e.what() << std::endl;
    }
}

std::string LLMBehaviorAnalyzer::analyzeWithOpenAI(const std::string& prompt) {
    if (openai_api_key_.empty()) {
        throw std::runtime_error("OpenAI API key not set");
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    std::string response;
    std::string json_payload = R"({
        "model": ")" + openai_model_ + R"(",
        "messages": [
            {
                "role": "system",
                "content": "You are an expert cybersecurity analyst specializing in behavioral analysis and insider threat detection. Analyze user behavior patterns and provide detailed insights."
            },
            {
                "role": "user",
                "content": ")" + prompt + R"("
            }
        ],
        "max_tokens": 1000,
        "temperature": 0.3
    })";

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("Authorization: Bearer " + openai_api_key_).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/chat/completions");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error("OpenAI API request failed: " + std::string(curl_easy_strerror(res)));
    }

    // Parse response
    try {
        json response_json = json::parse(response);
        if (response_json.contains("choices") && !response_json["choices"].empty()) {
            return response_json["choices"][0]["message"]["content"];
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse OpenAI response: " + std::string(e.what()));
    }

    return "";
}

std::string LLMBehaviorAnalyzer::analyzeWithAnthropic(const std::string& prompt) {
    if (anthropic_api_key_.empty()) {
        throw std::runtime_error("Anthropic API key not set");
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    std::string response;
    std::string json_payload = R"({
        "model": ")" + anthropic_model_ + R"(",
        "max_tokens": 1000,
        "messages": [
            {
                "role": "user",
                "content": ")" + prompt + R"("
            }
        ]
    })";

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");
    headers = curl_slist_append(headers, ("x-api-key: " + anthropic_api_key_).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.anthropic.com/v1/messages");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error("Anthropic API request failed: " + std::string(curl_easy_strerror(res)));
    }

    // Parse response
    try {
        json response_json = json::parse(response);
        if (response_json.contains("content") && !response_json["content"].empty()) {
            return response_json["content"][0]["text"];
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse Anthropic response: " + std::string(e.what()));
    }

    return "";
}

std::string LLMBehaviorAnalyzer::analyzeWithLocalModel(const std::string& prompt) {
#ifdef USE_LOCAL_MODELS
    // Local model implementation would go here
    // This is a placeholder for local LLM integration
    return "Local model analysis not implemented yet";
#else
    throw std::runtime_error("Local model support not compiled in");
#endif
}

std::string LLMBehaviorAnalyzer::buildAnalysisPrompt(const std::string& user_id) {
    std::string behavior_data = formatBehaviorData(user_id);

    std::string prompt = R"(
Analyze the following user behavior data for security risks and anomalies:

User ID: )" + user_id + R"(
Behavior Data:
)" + behavior_data + R"(

Please provide a detailed analysis including:
1. Risk assessment (Low/Medium/High/Critical)
2. Specific behavioral patterns identified
3. Potential security concerns
4. Confidence level in your analysis
5. Recommended actions

Format your response as JSON with the following structure:
{
    "risk_level": "low|medium|high|critical",
    "confidence_score": 0.0-1.0,
    "patterns": ["pattern1", "pattern2"],
    "concerns": ["concern1", "concern2"],
    "analysis": "detailed analysis text",
    "recommendations": ["rec1", "rec2"]
}
)";

    return prompt;
}

std::string LLMBehaviorAnalyzer::formatBehaviorData(const std::string& user_id) {
    if (user_contexts_.find(user_id) == user_contexts_.end()) {
        return "No behavior data available";
    }

    const auto& context = user_contexts_[user_id];
    std::stringstream ss;

    ss << "Recent Activities (" << context.recent_activities.size() << "):\n";
    for (size_t i = 0; i < context.recent_activities.size() && i < 20; ++i) {
        ss << "- " << context.recent_activities[context.recent_activities.size() - 1 - i] << "\n";
    }

    ss << "\nBehavior Metrics:\n";
    for (const auto& [key, value] : context.behavior_metrics) {
        ss << "- " << key << ": " << value << "\n";
    }

    ss << "\nRisk Indicators:\n";
    for (const auto& indicator : context.risk_indicators) {
        ss << "- " << indicator << "\n";
    }

    return ss.str();
}

LLMBehaviorInsight LLMBehaviorAnalyzer::parseLLMResponse(const std::string& response, const std::string& user_id) {
    LLMBehaviorInsight insight;
    insight.user = user_id;
    insight.timestamp = std::chrono::system_clock::now();

    try {
        json response_json = json::parse(response);

        insight.severity = response_json.value("risk_level", "medium");
        insight.confidence_score = response_json.value("confidence_score", 0.5);
        insight.analysis = response_json.value("analysis", "Analysis completed");

        if (response_json.contains("patterns")) {
            insight.description = "Detected patterns: ";
            for (const auto& pattern : response_json["patterns"]) {
                insight.description += pattern.get<std::string>() + ", ";
            }
            insight.description = insight.description.substr(0, insight.description.size() - 2);
        }

        if (response_json.contains("recommendations")) {
            for (const auto& rec : response_json["recommendations"]) {
                insight.recommendations.push_back(rec.get<std::string>());
            }
        }

        // Determine insight type based on severity
        if (insight.severity == "critical" || insight.severity == "high") {
            insight.insight_type = "alert";
        } else if (!insight.recommendations.empty()) {
            insight.insight_type = "recommendation";
        } else {
            insight.insight_type = "pattern";
        }

    } catch (const std::exception& e) {
        // Fallback parsing for non-JSON responses
        insight.severity = "medium";
        insight.confidence_score = 0.5;
        insight.analysis = response;
        insight.description = "LLM analysis completed";
        insight.insight_type = "pattern";
    }

    return insight;
}

void LLMBehaviorAnalyzer::storeInsight(const LLMBehaviorInsight& insight) {
    std::lock_guard<std::mutex> lock(data_mutex_);

    insights_history_.push_back(insight);

    // Keep only last 1000 insights
    if (insights_history_.size() > 1000) {
        insights_history_.pop_front();
    }
}

void LLMBehaviorAnalyzer::analysisLoop() {
    while (running_) {
        try {
            performBehavioralAnalysis();
        } catch (const std::exception& e) {
            std::cerr << "Analysis loop error: " << e.what() << std::endl;
        }

        // Sleep for analysis interval
        std::this_thread::sleep_for(std::chrono::seconds(analysis_interval_));
    }
}

void LLMBehaviorAnalyzer::performBehavioralAnalysis() {
    std::lock_guard<std::mutex> lock(data_mutex_);

    // Analyze users who have pending analysis or haven't been analyzed recently
    auto now = std::chrono::system_clock::now();

    for (auto& [user_id, context] : user_contexts_) {
        auto time_since_analysis = std::chrono::duration_cast<std::chrono::seconds>(
            now - context.last_analysis).count();

        // Analyze if it's been more than the interval or if in pending queue
        auto it = std::find(pending_analyses_.begin(), pending_analyses_.end(), user_id);
        if (it != pending_analyses_.end() || time_since_analysis >= analysis_interval_) {
            analyzeRiskPatterns(user_id);
            context.last_analysis = now;

            if (it != pending_analyses_.end()) {
                pending_analyses_.erase(it);
            }
        }
    }
}

UserBehaviorContext LLMBehaviorAnalyzer::getUserContext(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(data_mutex_);

    if (user_contexts_.find(user_id) != user_contexts_.end()) {
        return user_contexts_[user_id];
    }

    return UserBehaviorContext{user_id, {}, {}, {}, std::chrono::system_clock::now()};
}

void LLMBehaviorAnalyzer::updateUserContext(const std::string& user_id, const UserBehaviorContext& context) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    user_contexts_[user_id] = context;
}

std::vector<LLMBehaviorInsight> LLMBehaviorAnalyzer::getRecentInsights(const std::string& user_id, int limit) {
    std::lock_guard<std::mutex> lock(data_mutex_);

    std::vector<LLMBehaviorInsight> user_insights;

    for (const auto& insight : insights_history_) {
        if (insight.user == user_id) {
            user_insights.push_back(insight);
            if (user_insights.size() >= static_cast<size_t>(limit)) {
                break;
            }
        }
    }

    return user_insights;
}

void LLMBehaviorAnalyzer::setInsightCallback(std::function<void(const LLMBehaviorInsight&)> callback) {
    insight_callback_ = callback;
}
