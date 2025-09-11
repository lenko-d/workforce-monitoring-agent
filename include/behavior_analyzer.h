#ifndef BEHAVIOR_ANALYZER_H
#define BEHAVIOR_ANALYZER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <chrono>
#include <functional>
#include <memory>

// Forward declaration for LLM analyzer
class LLMBehaviorAnalyzer;

struct BehaviorPattern {
    std::string user;
    std::string pattern_type;  // "normal", "suspicious", "anomalous"
    double confidence_score;   // 0.0 to 1.0
    std::string description;
    std::chrono::system_clock::time_point timestamp;
};

struct UserProfile {
    std::string user_id;
    std::unordered_map<std::string, double> baseline_metrics;
    std::vector<BehaviorPattern> recent_patterns;
    double risk_score;  // 0.0 to 1.0
};

class BehaviorAnalyzer {
public:
    BehaviorAnalyzer();
    ~BehaviorAnalyzer();

    // LLM Integration
    void enableLLMAnalysis(bool enable);
    void setLLMProvider(const std::string& provider);  // "openai", "anthropic", "local"
    void setLLMAPIKey(const std::string& provider, const std::string& api_key);
    void setLLMModel(const std::string& provider, const std::string& model);

    // Core analysis methods
    void analyzeActivity(const std::string& user, const std::string& activity_type,
                        const std::unordered_map<std::string, double>& metrics);
    void updateUserProfile(const std::string& user, const UserProfile& profile);
    UserProfile getUserProfile(const std::string& user);
    std::vector<BehaviorPattern> getRecentPatterns(const std::string& user, int limit = 10);
    void setAnomalyCallback(std::function<void(const BehaviorPattern&)> callback);

    // LLM-specific methods
    void startLLMAnalysis();
    void stopLLMAnalysis();
    void requestLLMAnalysis(const std::string& user);
    void generateSecurityRecommendations(const std::string& user);

private:
    void detectAnomalies(const std::string& user, const std::unordered_map<std::string, double>& current_metrics);
    void updateBaseline(const std::string& user, const std::unordered_map<std::string, double>& metrics);
    double calculateRiskScore(const std::string& user);
    bool isAnomalous(const std::unordered_map<std::string, double>& current,
                    const std::unordered_map<std::string, double>& baseline, double threshold = 0.7);

    // LLM callback handler
    void handleLLMInsight(const struct LLMBehaviorInsight& insight);

    std::unordered_map<std::string, UserProfile> user_profiles_;
    std::function<void(const BehaviorPattern&)> anomaly_callback_;
    std::deque<BehaviorPattern> pattern_history_;

    // LLM components
    bool llm_enabled_;
    std::unique_ptr<LLMBehaviorAnalyzer> llm_analyzer_;
};

#endif // BEHAVIOR_ANALYZER_H
