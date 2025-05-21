#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <chrono>

// Structure to store vehicle data
struct VehicleData {
    std::string id;
    std::string sensor_id;
    double lat;
    double lon;
    double heading;
    std::chrono::system_clock::time_point timestamp;
    int global_vehicle_id;
};

// Function to calculate distance between two points using Haversine formula (approximation of geodesic)
double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371000; // Earth radius in meters
    double phi1 = lat1 * M_PI / 180;
    double phi2 = lat2 * M_PI / 180;
    double deltaPhi = (lat2 - lat1) * M_PI / 180;
    double deltaLambda = (lon2 - lon1) * M_PI / 180;

    double a = sin(deltaPhi/2) * sin(deltaPhi/2) +
               cos(phi1) * cos(phi2) *
               sin(deltaLambda/2) * sin(deltaLambda/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    return R * c;
}

// Function to check if two vehicles match based on distance, time and heading
bool isMatch(const VehicleData& v1, const VehicleData& v2, double maxDistance = 10.0, 
             int maxSeconds = 5, double maxHeadingDiff = 30.0) {
    double dist = calculateDistance(v1.lat, v1.lon, v2.lat, v2.lon);
    
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        v1.timestamp - v2.timestamp).count();
    int timeDiff = std::abs(duration);
    
    double headingDiff = std::abs(v1.heading - v2.heading);
    
    return (dist <= maxDistance && timeDiff <= maxSeconds && headingDiff <= maxHeadingDiff);
}

// Function to parse datetime string to time_point
std::chrono::system_clock::time_point parseDateTime(const std::string& datetime) {
    std::tm tm = {};
    std::istringstream ss(datetime);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    
    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    
    // Check if there are milliseconds in the timestamp
    size_t pos = datetime.find('.');
    if (pos != std::string::npos) {
        std::string ms_str = datetime.substr(pos + 1, 3);
        int ms = std::stoi(ms_str);
        tp += std::chrono::milliseconds(ms);
    }
    
    return tp;
}

// Function to split a string by delimiter
std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

int main() {
    std::string inputFile = "data/vehicle_objects_10min_nodimensions.csv";
    std::string outputFile = "data/matched_vehicle_data.csv";
    
    // Read the CSV file
    std::ifstream file(inputFile);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << inputFile << std::endl;
        return 1;
    }
    
    std::vector<VehicleData> vehicleData;
    std::string line;
    
    // Read header
    std::getline(file, line);
    std::vector<std::string> headers = split(line, ',');
    
    // Find column indices
    int idIdx = -1, sensorIdIdx = -1, latIdx = -1, lonIdx = -1, 
        headingIdx = -1, timestampIdx = -1;
    
    for (size_t i = 0; i < headers.size(); i++) {
        if (headers[i] == "id") idIdx = i;
        else if (headers[i] == "sensor_id") sensorIdIdx = i;
        else if (headers[i] == "lat") latIdx = i;
        else if (headers[i] == "lon") lonIdx = i;
        else if (headers[i] == "heading") headingIdx = i;
        else if (headers[i] == "timestamp") timestampIdx = i;
    }
    
    if (idIdx == -1 || sensorIdIdx == -1 || latIdx == -1 || lonIdx == -1 || 
        headingIdx == -1 || timestampIdx == -1) {
        std::cerr << "Missing required columns in CSV" << std::endl;
        return 1;
    }
    
    // Read data rows
    std::chrono::system_clock::time_point startTime;
    bool isFirstRow = true;
    
    while (std::getline(file, line)) {
        std::vector<std::string> values = split(line, ',');
        
        if (values.size() > std::max({idIdx, sensorIdIdx, latIdx, lonIdx, headingIdx, timestampIdx})) {
            VehicleData vehicle;
            vehicle.id = values[idIdx];
            vehicle.sensor_id = values[sensorIdIdx];
            vehicle.lat = std::stod(values[latIdx]);
            vehicle.lon = std::stod(values[lonIdx]);
            vehicle.heading = std::stod(values[headingIdx]);
            vehicle.timestamp = parseDateTime(values[timestampIdx]);
            vehicle.global_vehicle_id = -1;  // Initialize with no match
            
            if (isFirstRow) {
                startTime = vehicle.timestamp;
                isFirstRow = false;
            }
            
            vehicleData.push_back(vehicle);
        }
    }
    
    file.close();
    
    std::cout << "Filtering data to only include the first 5 minutes" << std::endl;
    
    // Calculate end time (start time + 10 minutes)
    auto endTime = startTime + std::chrono::minutes(10);
    
    // Filter by time
    std::vector<VehicleData> filteredData;
    for (const auto& vehicle : vehicleData) {
        if (vehicle.timestamp >= startTime && vehicle.timestamp <= endTime) {
            filteredData.push_back(vehicle);
        }
    }
    
    std::cout << "Filtering data for vista-p90-2 and vista-p90-3" << std::endl;
    
    // Filter by sensor_id
    std::vector<VehicleData> groupData;
    for (const auto& vehicle : filteredData) {
        if (vehicle.sensor_id == "vista-p90-2" || vehicle.sensor_id == "vista-p90-3") {
            groupData.push_back(vehicle);
        }
    }
    
    // Sort by timestamp
    std::sort(groupData.begin(), groupData.end(), 
        [](const VehicleData& a, const VehicleData& b) {
            return a.timestamp < b.timestamp;
        });
    
    std::cout << "Separating data for each sensor" << std::endl;
    
    // Separate data for each sensor
    std::vector<VehicleData> vistaP90_2;
    std::vector<VehicleData> vistaP90_3;
    
    for (const auto& vehicle : groupData) {
        if (vehicle.sensor_id == "vista-p90-2") {
            vistaP90_2.push_back(vehicle);
        } else if (vehicle.sensor_id == "vista-p90-3") {
            vistaP90_3.push_back(vehicle);
        }
    }
    
    std::cout << "Matching" << std::endl;
    
    // Create matches
    std::map<std::pair<std::string, std::string>, int> matches;
    int globalId = 1;
    
    for (const auto& row2 : vistaP90_2) {
        for (const auto& row3 : vistaP90_3) {
            if (isMatch(row2, row3)) {
                std::cout << "Matching " << row2.id << " from vista-p90-2 with " 
                          << row3.id << " from vista-p90-3" << std::endl;
                
                std::pair<std::string, std::string> key2(row2.id, row2.sensor_id);
                std::pair<std::string, std::string> key3(row3.id, row3.sensor_id);
                
                if (matches.find(key2) == matches.end() && matches.find(key3) == matches.end()) {
                    std::cout << "Assigning global ID " << globalId << " to " 
                              << "(" << key2.first << ", " << key2.second << ") and "
                              << "(" << key3.first << ", " << key3.second << ")" << std::endl;
                    
                    matches[key2] = globalId;
                    matches[key3] = globalId;
                    globalId++;
                }
            }
        }
    }
    
    std::cout << "Assigning global vehicle IDs to the original data" << std::endl;
    
    // Assign global vehicle IDs
    for (auto& vehicle : groupData) {
        std::pair<std::string, std::string> key(vehicle.id, vehicle.sensor_id);
        if (matches.find(key) != matches.end()) {
            vehicle.global_vehicle_id = matches[key];
        }
    }
    
    // Write results to CSV
    std::ofstream outFile(outputFile);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open output file: " << outputFile << std::endl;
        return 1;
    }
    
    // Write header with additional global_vehicle_id column
    outFile << line << "id,sensor_id,lat,lon,heading,timestamp,global_vehicle_id" << std::endl;
    
    // Write all data to file
    for (const auto& vehicle : groupData) {
        // This is a simplified output - in a real implementation you would need to 
        // reconstruct the entire row with all original columns plus the new global_vehicle_id
        outFile << vehicle.id << "," << vehicle.sensor_id << "," 
                << vehicle.lat << "," << vehicle.lon << "," 
                << vehicle.heading << ",";
        
        // Format timestamp for output
        auto timeT = std::chrono::system_clock::to_time_t(vehicle.timestamp);
        std::tm tm = *std::localtime(&timeT);
        outFile << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << ",";
        
        outFile << (vehicle.global_vehicle_id >= 0 ? std::to_string(vehicle.global_vehicle_id) : "") 
                << std::endl;
    }
    
    outFile.close();
    std::cout << "Results written to " << outputFile << std::endl;
    
    return 0;
}
