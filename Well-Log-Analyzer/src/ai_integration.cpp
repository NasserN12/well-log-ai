#include "../include/ai_integration.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iostream> 

using json = nlohmann::json;

// Helper function for curl to write response data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

OpenAIClient::OpenAIClient(const std::string& api_key, const std::string& model)
    : api_key(api_key), model(model.empty() ? "openai/gpt-3.5-turbo" : model) {}

std::string OpenAIClient::preparePrompt(const WellLog& log) {
    std::stringstream prompt;
    
    prompt << "Please analyze this well log data:\n\n";
    
    // Add statistical summary
    prompt << "Depth range: " << log.getMinValue("depth") << " to " << log.getMaxValue("depth") << " m\n";
    prompt << "Gamma Ray: avg " << log.getAverage("gamma_ray") << " API (range: " 
           << log.getMinValue("gamma_ray") << " - " << log.getMaxValue("gamma_ray") << ")\n";
    prompt << "Neutron Density: avg " << log.getAverage("neutron_density") << " g/cc (range: "
           << log.getMinValue("neutron_density") << " - " << log.getMaxValue("neutron_density") << ")\n";
    prompt << "Resistivity: avg " << log.getAverage("resistivity") << " ohm·m (range: "
           << log.getMinValue("resistivity") << " - " << log.getMaxValue("resistivity") << ")\n";
    
    // Don't call detectAnomalies() here to avoid circular dependency
    
    prompt << "\nPlease provide:\n";
    prompt << "1. An interpretation of the geological formations based on these logs\n";
    prompt << "2. Any potential drilling risks or areas of concern\n";
    prompt << "3. Recommendations for further analysis or logging\n";
    
    return prompt.str();
}

std::string OpenAIClient::makeRequest(const std::string& endpoint, const std::string& jsonPayload) {
    CURL* curl = curl_easy_init();
    std::string readBuffer;
    
    if (curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key).c_str());
        
        // Add OpenRouter required headers
        headers = curl_slist_append(headers, "HTTP-Referer: https://well-log-analyzer.local");
        headers = curl_slist_append(headers, "X-Title: Well Log Analyzer");
        
        curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            readBuffer = "Error: " + std::string(curl_easy_strerror(res));
        }
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    
    return readBuffer;
}

std::string OpenAIClient::analyzeWellLog(const WellLog& log) {
    //const std::string endpoint = "https://api.openai.com/v1/chat/completions";
    const std::string endpoint = "https://openrouter.ai/api/v1/chat/completions";

    
    // Prepare the prompt for the AI
    std::string prompt = preparePrompt(log);
    
    // Create JSON payload
    json payload = {
        {"model", "openai/gpt-3.5-turbo"}, // Specify the provider/model format
        {"messages", {
            {{"role", "system"}, {"content", "You are a petroleum engineering assistant specialized in well log analysis."}},
            {{"role", "user"}, {"content", prompt}}
        }},
        {"temperature", 0.7}
    };
    
    // Make the API request
    std::string response = makeRequest(endpoint, payload.dump());
    
    // Parse the response
    try {
        json responseJson = json::parse(response);
        if (responseJson.contains("error")) {
            return "API Error: " + responseJson["error"]["message"].get<std::string>();
        }
        
        return responseJson["choices"][0]["message"]["content"].get<std::string>();
    }
    catch (std::exception& e) {
        return "Error parsing response: " + std::string(e.what());
    }
}

std::vector<WellLog::Anomaly> OpenAIClient::detectAnomalies(const WellLog& log) {
    std::vector<WellLog::Anomaly> anomalies;
    
    // Create the prompt for anomaly detection
    std::stringstream prompt;
    prompt << "Analyze this well log data for anomalies. For each anomaly, return it in this exact format:\n";
    prompt << "ANOMALY|depth|parameter|value|description\n\n";
    
    // Add the data summary
    prompt << "Well log statistics:\n";
    prompt << "- Depth range: " << log.getMinValue("depth") << " to " << log.getMaxValue("depth") << " m\n";
    prompt << "- Gamma Ray: avg " << log.getAverage("gamma_ray") << " API (range: " 
           << log.getMinValue("gamma_ray") << " - " << log.getMaxValue("gamma_ray") << ")\n";
    prompt << "- Neutron Density: avg " << log.getAverage("neutron_density") << " g/cc (range: "
           << log.getMinValue("neutron_density") << " - " << log.getMaxValue("neutron_density") << ")\n";
    prompt << "- Resistivity: avg " << log.getAverage("resistivity") << " ohm·m (range: "
           << log.getMinValue("resistivity") << " - " << log.getMaxValue("resistivity") << ")\n";
    
    // Add sample records for context
    prompt << "\nSample records (first 10 or fewer):\n";
    const auto& records = log.getRecords();
    for (size_t i = 0; i < std::min(records.size(), size_t(10)); ++i) {
        prompt << "- Depth: " << records[i].depth 
               << ", GR: " << records[i].gamma_ray 
               << ", ND: " << records[i].neutron_density 
               << ", Res: " << records[i].resistivity 
               << ", Lith: " << records[i].lithology << "\n";
    }
    
    prompt << "\nIdentify any anomalies in the dataset based on your expertise in well log analysis.\n";
    prompt << "For each anomaly found, output exactly one line in this format: ";
    prompt << "ANOMALY|depth|parameter|value|description\n";
    prompt << "Only use gamma_ray, neutron_density, or resistivity for the parameter field.";
    
    // Create JSON payload and make the API request
    json payload = {
        {"model", "openai/gpt-3.5-turbo"}, // Could use a more sophisticated model if needed
        {"messages", {
            {{"role", "system"}, {"content", "You are a petroleum engineering expert specializing in well log anomaly detection. Respond only with anomalies in the specified format."}},
            {{"role", "user"}, {"content", prompt.str()}}
        }},
        {"temperature", 0.2} // Lower temperature for more deterministic responses
    };
    
    const std::string endpoint = "https://openrouter.ai/api/v1/chat/completions";
    std::string response = makeRequest(endpoint, payload.dump());
    
    try {
        json responseJson = json::parse(response);
        if (responseJson.contains("error")) {
            std::cerr << "API Error: " << responseJson["error"]["message"].get<std::string>() << std::endl;
            return anomalies;
        }
        
        std::string content = responseJson["choices"][0]["message"]["content"].get<std::string>();
        std::stringstream ss(content);
        std::string line;
        
        // Parse each line that starts with "ANOMALY|"
        while (std::getline(ss, line)) {
            if (line.substr(0, 8) == "ANOMALY|") {
                std::stringstream lineStream(line.substr(8)); // Skip the "ANOMALY|" prefix
                std::string part;
                WellLog::Anomaly anomaly;
                
                // Parse depth
                std::getline(lineStream, part, '|');
                anomaly.depth = std::stod(part);
                
                // Parse parameter
                std::getline(lineStream, part, '|');
                anomaly.parameter = part;
                
                // Parse value
                std::getline(lineStream, part, '|');
                anomaly.value = std::stod(part);
                
                // Parse description
                std::getline(lineStream, part);
                anomaly.description = part;
                
                anomalies.push_back(anomaly);
            }
        }
    }
    catch (std::exception& e) {
        std::cerr << "Error parsing anomaly response: " << e.what() << std::endl;
    }
    
    return anomalies;
}