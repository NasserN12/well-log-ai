#include <iostream>
#include <string>
#include <fstream>
#include "../include/well_log.h"
#include "../include/ai_integration.h"

int main(int argc, char *argv[]) {
    std::cout << "Well Log Analyzer v0.1" << std::endl;
    
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <data_file.csv> [api_key]" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    std::cout << "Reading data from: " << filename << std::endl;

    WellLog log;
    if (!log.loadFromCSV(filename)) {
        std::cerr << "Error loading data from file: " << filename << std::endl;
        return 1;
    }

    // Display basic statistics
    log.printBasicStatistics();
    
    // Check if API key is provided for AI analysis
    std::string api_key;
    if (argc >= 3) {
        api_key = argv[2];
    } else {
        // Try to read API key from environment variable or config file
        char* env_api_key = getenv("OPENAI_API_KEY");
        if (env_api_key) {
            api_key = env_api_key;
        } else {
            std::ifstream keyFile("api_key.txt");
            if (keyFile.is_open()) {
                std::getline(keyFile, api_key);
                keyFile.close();
            }
        }
    }
    
    // If we have an API key, perform AI analysis
    if (!api_key.empty()) {
        std::cout << "\n=== AI Well Log Analysis ===\n" << std::endl;
        OpenAIClient aiClient(api_key);
        
        // Add this block to detect anomalies explicitly
        std::cout << "Detecting anomalies..." << std::endl;
        std::vector<WellLog::Anomaly> anomalies = aiClient.detectAnomalies(log);
        if (!anomalies.empty()) {
            std::cout << "\n=== AI Detected Anomalies ===\n" << std::endl;
            for (const auto& anomaly : anomalies) {
                std::cout << "Depth " << anomaly.depth << "m: " << anomaly.description 
                          << " (" << anomaly.parameter << " = " << anomaly.value << ")" << std::endl;
            }
        } else {
            std::cout << "No anomalies detected by AI." << std::endl;
        }
        
        // Continue with general analysis
        std::cout << "\nRequesting analysis from OpenAI..." << std::endl;
        std::string analysis = aiClient.analyzeWellLog(log);
        
        std::cout << "\nAI Analysis Results:\n" << std::endl;
        std::cout << analysis << std::endl;
    } else {
        std::cout << "\nNo OpenAI API key provided. Skipping AI analysis." << std::endl;
        std::cout << "To include AI analysis, provide your API key as a command line argument," << std::endl;
        std::cout << "set the OPENAI_API_KEY environment variable, or create an api_key.txt file." << std::endl;
    }
    
    return 0;
}