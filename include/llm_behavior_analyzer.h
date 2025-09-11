#ifndef LLM_BEHAVIOR_ANALYZER_H
#define LLM_BEHAVIOR_ANALYZER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

struct LLMBehaviorInsight {
    std::string user;
    std::string insight_type;  // "risk", "pattern", "recommendation", "alert"
    std::string severity;      // "low", "medium", "high", "critical"
    double confidence_score;   // 0.0 to 1.0
    std::string description;
    std::string analysis;
    std::vector<std::string> recommendations;
    std::chrono::system_clock::time_point timestamp;
};

struct UserBehaviorContext {
    std::string user_id;
    std::vector<std::string> recent_activities;
    std::unordered_map<std::string, double> behavior_metrics;
    std::vector<std::string> risk_indicators;
    std::chrono::system_clock::time_point last_analysis;
};

class LLMBehaviorAnalyzer {
public:
    LLMBehaviorAnalyzer();
    ~LLMBehaviorAnalyzer();

    // Configuration
    void setAPIKey(const std::string& provider, const std::string& api_key);
    void setModel(const std::string& provider, const std::string& model);
    void setAnalysisInterval(int seconds);
    void enableRealTimeAnalysis(bool enable);

    // Analysis methods
    void analyzeUserBehavior(const std::string& user_id,
                           const std::vector<std::string>& activities,
                           const std::unordered_map<std::string, double>& metrics);
    void analyzeRiskPatterns(const std::string& user_id);
    void generateSecurityRecommendations(const std::string& user_id);

    // Data management
    void addBehaviorData(const std::string& user_id, const std::string& activity);
    void updateUserContext(const std::string& user_id, const UserBehaviorContext& context);
    UserBehaviorContext getUserContext(const std::string& user_id);

    // Results and callbacks
    std::vector<LLMBehaviorInsight> getRecentInsights(const std::string& user_id, int limit = 10);
    void setInsightCallback(std::function<void(const LLMBehaviorInsight&)> callback);

    // Analysis control
    void startAnalysis();
    void stopAnalysis();
    bool isRunning() const;

private:
    // LLM providers
    std::string analyzeWithOpenAI(const std::string& prompt);
    std::string analyzeWithAnthropic(const std::string& prompt);
    std::string analyzeWithLocalModel(const std::string& prompt);

    // Analysis methods
    void performBehavioralAnalysis();
    void analyzeActivityPatterns(const std::string& user_id);
    void detectAnomalousSequences(const std::string& user_id);
    void assessRiskLevel(const std::string& user_id);
    void generateRecommendations(const std::string& user_id);

    // Helper methods
    std::string buildAnalysisPrompt(const std::string& user_id);
    std::string formatBehaviorData(const std::string& user_id);
    LLMBehaviorInsight parseLLMResponse(const std::string& response, const std::string& user_id);
    void storeInsight(const LLMBehaviorInsight& insight);

    // Threading and synchronization
    void analysisLoop();
    std::thread analysis_thread_;
    std::atomic<bool> running_;
    std::atomic<bool> real_time_enabled_;
    std::mutex data_mutex_;

    // Configuration
    std::string llm_provider_;  // "openai", "anthropic", "local"
    std::string openai_api_key_;
    std::string anthropic_api_key_;
    std::string openai_model_;
    std::string anthropic_model_;
    int analysis_interval_;  // seconds

    // Data storage
    std::unordered_map<std::string, UserBehaviorContext> user_contexts_;
    std::deque<LLMBehaviorInsight> insights_history_;
    std::function<void(const LLMBehaviorInsight&)> insight_callback_;

    // Analysis queue
    std::vector<std::string> pending_analyses_;
};

#endif // LLM_BEHAVIOR_ANALYZER_H
