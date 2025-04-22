#ifndef WELL_LOG_H
#define WELL_LOG_H

#include <string>
#include <vector>

// Structure to store a single well log measurement
struct LogRecord {
    double depth;          // in meters
    double gamma_ray;      // in API
    double neutron_density; // in g/cc
    double resistivity;    // in ohm·m
    std::string lithology; // rock type
};

// Class to manage well log data and analysis
class WellLog {
private:
    std::vector<LogRecord> records;
    
public:
    // Load data from CSV file
    bool loadFromCSV(const std::string& filename);
    
    // Basic statistical analysis
    void printBasicStatistics() const;
    
    // Statistical calculation functions
    double getMinValue(const std::string& parameter) const;
    double getMaxValue(const std::string& parameter) const;
    double getAverage(const std::string& parameter) const;
    double getStandardDeviation(const std::string& parameter) const;

    // Detect anomalies based on thresholds
    struct Anomaly {
        double depth;
        std::string parameter;
        double value;
        std::string description;
    };

    // Find values outside expected ranges
    std::vector<Anomaly> detectAnomalies() const;

    // Generate summary for AI analysis
    std::string generateSummaryForAI() const;

    // Helper method for getting values
    static double getValue(const LogRecord& record, const std::string& parameter);

    // Accessor for records
    const std::vector<LogRecord>& getRecords() const { return records; }
};

#endif // WELL_LOG_H