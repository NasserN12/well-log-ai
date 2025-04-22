#include "../include/well_log.h"
#include "../include/ai_integration.h" 
#include <iostream>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <iomanip>

bool WellLog::loadFromCSV(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return false;
    }

    records.clear();

    std::string header;
    std::getline(file, header); // Read the header line

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string value;
        LogRecord record;

        //parse each field
        std::getline(ss, value, ',');
        record.depth = std::stod(value);

        std::getline(ss, value, ',');
        record.gamma_ray = std::stod(value);

        std::getline(ss, value, ',');
        record.neutron_density = std::stod(value);

        std::getline(ss, value, ',');
        record.resistivity = std::stod(value);

        std::getline(ss, value, ',');
        record.lithology = value;

        records.push_back(record);
    }

    return !records.empty();

}

double WellLog::getAverage(const std::string& parameter) const {
    if (records.empty()) return 0.0;
    
    double sum = 0.0;
    for (const auto& record : records) {
        sum += getValue(record, parameter);
    }
    
    return sum / records.size();
}

double WellLog::getStandardDeviation(const std::string& parameter) const {
    if (records.empty()) return 0.0;
    
    double mean = getAverage(parameter);
    double sumSquaredDiff = 0.0;
    
    for (const auto& record : records) {
        double diff = getValue(record, parameter) - mean;
        sumSquaredDiff += diff * diff;
    }
    
    return std::sqrt(sumSquaredDiff / records.size());
}

std::vector<WellLog::Anomaly> WellLog::detectAnomalies() const {
    // Check if OpenRouter/OpenAI integration is enabled and API key is available
    char* env_api_key = getenv("OPENAI_API_KEY");
    std::string api_key;
    
    if (env_api_key) {
        api_key = env_api_key;
    } else {
        std::ifstream keyFile("api_key.txt");
        if (keyFile.is_open()) {
            std::getline(keyFile, api_key);
            keyFile.close();
        }
    }
    
    if (!api_key.empty()) {
        // Use AI for anomaly detection
        OpenAIClient aiClient(api_key);
        return aiClient.detectAnomalies(*this);
    } else {
        // Return empty vector if no API key is available
        std::cout << "\nNo API key provided. Anomaly detection requires an OpenAI API key." << std::endl;
        return std::vector<Anomaly>();
    }
}

  

void WellLog::printBasicStatistics() const {
    if (records.empty()) {
        std::cout << "No records available." << std::endl;
        return;
    }

    std::cout << "\n=== Basic Statistics ===\n" << std::endl;
    std::cout << "Total Records: " << records.size() << std::endl;
    
    std::cout << "\nDepth (m):" << std::endl;
    std::cout << "  Min: " << getMinValue("depth") << std::endl;
    std::cout << "  Max: " << getMaxValue("depth") << std::endl;
    std::cout << "  Avg: " << std::fixed << std::setprecision(2) << getAverage("depth") << std::endl;
    std::cout << "  StdDev: " << std::fixed << std::setprecision(2) << getStandardDeviation("depth") << std::endl;
    
    std::cout << "\nGamma Ray (API):" << std::endl;
    std::cout << "  Min: " << getMinValue("gamma_ray") << std::endl;
    std::cout << "  Max: " << getMaxValue("gamma_ray") << std::endl;
    std::cout << "  Avg: " << std::fixed << std::setprecision(2) << getAverage("gamma_ray") << std::endl;
    std::cout << "  StdDev: " << std::fixed << std::setprecision(2) << getStandardDeviation("gamma_ray") << std::endl;
    
    std::cout << "\nNeutron Density (g/cc):" << std::endl;
    std::cout << "  Min: " << getMinValue("neutron_density") << std::endl;
    std::cout << "  Max: " << getMaxValue("neutron_density") << std::endl;
    std::cout << "  Avg: " << std::fixed << std::setprecision(2) << getAverage("neutron_density") << std::endl;
    std::cout << "  StdDev: " << std::fixed << std::setprecision(2) << getStandardDeviation("neutron_density") << std::endl;
    
    std::cout << "\nResistivity (ohm·m):" << std::endl;
    std::cout << "  Min: " << getMinValue("resistivity") << std::endl;
    std::cout << "  Max: " << getMaxValue("resistivity") << std::endl;
    std::cout << "  Avg: " << std::fixed << std::setprecision(2) << getAverage("resistivity") << std::endl;
    std::cout << "  StdDev: " << std::fixed << std::setprecision(2) << getStandardDeviation("resistivity") << std::endl;

    // Display detected anomalies
    std::vector<Anomaly> anomalies = detectAnomalies();
    if (!anomalies.empty()) {
        std::cout << "\n=== Detected Anomalies ===\n" << std::endl;
        for (const auto& anomaly : anomalies) {
            std::cout << "Depth " << anomaly.depth << "m: " << anomaly.description 
                      << " (" << anomaly.parameter << " = " << anomaly.value << ")" << std::endl;
        }
    }
}

double WellLog::getValue(const LogRecord& record, const std::string& parameter) {
    if (parameter == "depth") return record.depth;
    if (parameter == "gamma_ray") return record.gamma_ray;
    if (parameter == "neutron_density") return record.neutron_density;
    if (parameter == "resistivity") return record.resistivity;
    throw std::invalid_argument("Invalid parameter name");
}

double WellLog::getMaxValue(const std::string& parameter) const {
    if (records.empty()) return 0.0;
    
    auto maxElement = std::max_element(records.begin(), records.end(),
        [&](const LogRecord& a, const LogRecord& b) {
            return getValue(a, parameter) < getValue(b, parameter);
        });
    
    return getValue(*maxElement, parameter);
}

double WellLog::getMinValue(const std::string& parameter) const {
    if (records.empty()) return 0.0;
    
    auto minElement = std::min_element(records.begin(), records.end(),
        [&](const LogRecord& a, const LogRecord& b) {
            return getValue(a, parameter) < getValue(b, parameter);
        });
    
    return getValue(*minElement, parameter);
}