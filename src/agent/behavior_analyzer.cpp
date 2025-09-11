#include "behavior_analyzer.h"
#include "llm_behavior_analyzer.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

BehaviorAnalyzer::BehaviorAnalyzer()
    : llm_enabled_(false),
      llm_analyzer_(std::make_unique<LLMBehaviorAnalyzer>()) {
    // Set up LLM insight callback
    llm_analyzer_->setInsightCallback(
        [this](const LLMBehaviorInsight& insight) {
            this->handleLLMInsight(insight);
        }
    );
}

BehaviorAnalyzer::~BehaviorAnalyzer() {
    if (llm_analyzer_ && llm_analyzer_->isRunning()) {
        llm_analyzer_->stopAnalysis();
    }
}

void BehaviorAnalyzer::analyzeActivity(const std::string& user, const std::string& activity_type,
                                     const std::unordered_map<std::string, double>& metrics) {
    // Detect anomalies
    detectAnomalies(user, metrics);

    // Update baseline metrics
    updateBaseline(user, metrics);

    // Create behavior pattern
    BehaviorPattern pattern;
    pattern.user = user;
    pattern.confidence_score = calculateRiskScore(user);
    pattern.timestamp = std::chrono::system_clock::now();

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");

    if (pattern.confidence_score > 0.7) {
        pattern.pattern_type = "suspicious";
        pattern.description = "Suspicious activity detected: " + activity_type +
                            " (confidence: " + std::to_string(pattern.confidence_score) + ")";
    } else if (pattern.confidence_score > 0.5) {
        pattern.pattern_type = "anomalous";
        pattern.description = "Anomalous behavior detected: " + activity_type +
                            " (confidence: " + std::to_string(pattern.confidence_score) + ")";
    } else {
        pattern.pattern_type = "normal";
        pattern.description = "Normal activity: " + activity_type;
    }

    // Store pattern
    pattern_history_.push_back(pattern);
    if (pattern_history_.size() > 1000) {  // Keep last 1000 patterns
        pattern_history_.pop_front();
    }

    // Update user profile
    if (user_profiles_.find(user) == user_profiles_.end()) {
        user_profiles_[user] = UserProfile{user, {}, {}, 0.0};
    }
    user_profiles_[user].recent_patterns.push_back(pattern);
    user_profiles_[user].risk_score = pattern.confidence_score;

    // Trigger callback for anomalies
    if (pattern.pattern_type != "normal" && anomaly_callback_) {
        anomaly_callback_(pattern);
    }
}

void BehaviorAnalyzer::updateUserProfile(const std::string& user, const UserProfile& profile) {
    user_profiles_[user] = profile;
}

UserProfile BehaviorAnalyzer::getUserProfile(const std::string& user) {
    if (user_profiles_.find(user) != user_profiles_.end()) {
        return user_profiles_[user];
    }
    return UserProfile{user, {}, {}, 0.0};
}

std::vector<BehaviorPattern> BehaviorAnalyzer::getRecentPatterns(const std::string& user, int limit) {
    std::vector<BehaviorPattern> user_patterns;

    for (const auto& pattern : pattern_history_) {
        if (pattern.user == user) {
            user_patterns.push_back(pattern);
            if (user_patterns.size() >= static_cast<size_t>(limit)) {
                break;
            }
        }
    }

    return user_patterns;
}

void BehaviorAnalyzer::setAnomalyCallback(std::function<void(const BehaviorPattern&)> callback) {
    anomaly_callback_ = callback;
}

void BehaviorAnalyzer::detectAnomalies(const std::string& user,
                                      const std::unordered_map<std::string, double>& current_metrics) {
    if (user_profiles_.find(user) == user_profiles_.end()) {
        return;  // No baseline to compare against
    }

    const auto& baseline = user_profiles_[user].baseline_metrics;

    if (isAnomalous(current_metrics, baseline)) {
        // Anomaly detected - this will be handled in analyzeActivity
    }
}

void BehaviorAnalyzer::updateBaseline(const std::string& user,
                                    const std::unordered_map<std::string, double>& metrics) {
    if (user_profiles_.find(user) == user_profiles_.end()) {
        user_profiles_[user] = UserProfile{user, metrics, {}, 0.0};
        return;
    }

    auto& profile = user_profiles_[user];
    auto& baseline = profile.baseline_metrics;

    // Simple exponential moving average for baseline update
    const double alpha = 0.1;  // Learning rate

    for (const auto& [key, value] : metrics) {
        if (baseline.find(key) == baseline.end()) {
            baseline[key] = value;
        } else {
            baseline[key] = alpha * value + (1 - alpha) * baseline[key];
        }
    }
}

double BehaviorAnalyzer::calculateRiskScore(const std::string& user) {
    if (user_profiles_.find(user) == user_profiles_.end()) {
        return 0.0;
    }

    const auto& profile = user_profiles_[user];
    double risk_score = 0.0;

    // Calculate risk based on recent patterns
    int suspicious_count = 0;
    int anomalous_count = 0;
    int total_recent = 0;

    for (const auto& pattern : profile.recent_patterns) {
        if (++total_recent > 10) break;  // Last 10 patterns

        if (pattern.pattern_type == "suspicious") {
            suspicious_count++;
        } else if (pattern.pattern_type == "anomalous") {
            anomalous_count++;
        }
    }

    if (total_recent > 0) {
        risk_score = (suspicious_count * 0.8 + anomalous_count * 0.4) / total_recent;
    }

    return std::min(risk_score, 1.0);
}

bool BehaviorAnalyzer::isAnomalous(const std::unordered_map<std::string, double>& current,
                                  const std::unordered_map<std::string, double>& baseline,
                                  double threshold) {
    if (baseline.empty()) return false;

    double total_deviation = 0.0;
    int metrics_count = 0;

    for (const auto& [key, current_value] : current) {
        auto baseline_it = baseline.find(key);
        if (baseline_it != baseline.end()) {
            double baseline_value = baseline_it->second;
            if (baseline_value != 0) {
                double deviation = std::abs(current_value - baseline_value) / baseline_value;
                total_deviation += deviation;
                metrics_count++;
            }
        }
    }

    if (metrics_count == 0) return false;

    double average_deviation = total_deviation / metrics_count;
    return average_deviation > threshold;
}

// LLM Integration Methods
void BehaviorAnalyzer::enableLLMAnalysis(bool enable) {
    llm_enabled_ = enable;
    if (enable && !llm_analyzer_->isRunning()) {
        llm_analyzer_->startAnalysis();
    } else if (!enable && llm_analyzer_->isRunning()) {
        llm_analyzer_->stopAnalysis();
    }
}

void BehaviorAnalyzer::setLLMProvider(const std::string& provider) {
    // This would be stored and used when initializing the LLM analyzer
    // For now, we'll assume it's set directly on the analyzer
}

void BehaviorAnalyzer::setLLMAPIKey(const std::string& provider, const std::string& api_key) {
    if (llm_analyzer_) {
        llm_analyzer_->setAPIKey(provider, api_key);
    }
}

void BehaviorAnalyzer::setLLMModel(const std::string& provider, const std::string& model) {
    if (llm_analyzer_) {
        llm_analyzer_->setModel(provider, model);
    }
}

void BehaviorAnalyzer::startLLMAnalysis() {
    if (llm_enabled_ && llm_analyzer_ && !llm_analyzer_->isRunning()) {
        llm_analyzer_->startAnalysis();
    }
}

void BehaviorAnalyzer::stopLLMAnalysis() {
    if (llm_analyzer_ && llm_analyzer_->isRunning()) {
        llm_analyzer_->stopAnalysis();
    }
}

void BehaviorAnalyzer::requestLLMAnalysis(const std::string& user) {
    if (!llm_enabled_ || !llm_analyzer_) {
        return;
    }

    // Get user context and send to LLM analyzer
    if (user_profiles_.find(user) != user_profiles_.end()) {
        const auto& profile = user_profiles_[user];

        // Convert behavior patterns to activity strings
        std::vector<std::string> activities;
        for (const auto& pattern : profile.recent_patterns) {
            activities.push_back(pattern.description);
        }

        // Send to LLM analyzer
        llm_analyzer_->analyzeUserBehavior(user, activities, profile.baseline_metrics);
    }
}

void BehaviorAnalyzer::generateSecurityRecommendations(const std::string& user) {
    if (!llm_enabled_ || !llm_analyzer_) {
        return;
    }

    llm_analyzer_->generateSecurityRecommendations(user);
}

void BehaviorAnalyzer::handleLLMInsight(const LLMBehaviorInsight& insight) {
    // Convert LLM insight to behavior pattern for consistency
    BehaviorPattern pattern;
    pattern.user = insight.user;
    pattern.confidence_score = insight.confidence_score;
    pattern.timestamp = insight.timestamp;

    // Map LLM insight type to behavior pattern type
    if (insight.insight_type == "alert" || insight.severity == "critical" || insight.severity == "high") {
        pattern.pattern_type = "suspicious";
    } else if (insight.insight_type == "pattern" || insight.severity == "medium") {
        pattern.pattern_type = "anomalous";
    } else {
        pattern.pattern_type = "normal";
    }

    // Create description from LLM insight
    pattern.description = "[" + insight.insight_type + "] " + insight.description +
                        " (LLM confidence: " + std::to_string(insight.confidence_score) + ")";

    // Store the pattern
    pattern_history_.push_back(pattern);
    if (pattern_history_.size() > 1000) {
        pattern_history_.pop_front();
    }

    // Update user profile
    if (user_profiles_.find(insight.user) != user_profiles_.end()) {
        user_profiles_[insight.user].recent_patterns.push_back(pattern);
        user_profiles_[insight.user].risk_score = std::max(
            user_profiles_[insight.user].risk_score,
            insight.confidence_score
        );
    }

    // Trigger callback if this is an anomaly
    if (pattern.pattern_type != "normal" && anomaly_callback_) {
        anomaly_callback_(pattern);
    }

    // Log LLM insights
    std::cout << "[LLM Analysis] User: " << insight.user
              << " | Type: " << insight.insight_type
              << " | Severity: " << insight.severity
              << " | Confidence: " << insight.confidence_score
              << " | Description: " << insight.description << std::endl;

    if (!insight.recommendations.empty()) {
        std::cout << "[LLM Recommendations]:";
        for (const auto& rec : insight.recommendations) {
            std::cout << " " << rec << " |";
        }
        std::cout << std::endl;
    }
}
