#ifndef AI_INTEGRATION_H
#define AI_INTEGRATION_H

#include <string>
#include <vector>
#include "well_log.h"

class OpenAIClient {
private:
    std::string api_key;
    std::string model;
    
    // Internal helper methods
    std::string preparePrompt(const WellLog& log);
    std::string makeRequest(const std::string& endpoint, const std::string& jsonPayload);
    
public:
    // Constructor - takes API key and optional model name
    OpenAIClient(const std::string& api_key, const std::string& model = "gpt-3.5-turbo");
    
    // Main method to analyze well log data
    std::string analyzeWellLog(const WellLog& log);

    // Method to detect anomalies in well log data
    std::vector<WellLog::Anomaly> detectAnomalies(const WellLog& log);
};

#endif // AI_INTEGRATION_H