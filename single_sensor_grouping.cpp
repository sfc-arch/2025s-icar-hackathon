#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <set>

struct VehicleData {
    std::string id;
    std::string sensor_id;
    double lat;
    double lon;
    double speed;
    double heading;
    std::chrono::system_clock::time_point timestamp;
};

double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371000;
    double phi1 = lat1 * M_PI / 180.0;
    double phi2 = lat2 * M_PI / 180.0;
    double delta_phi = (lat2 - lat1) * M_PI / 180.0;
    double delta_lambda = (lon2 - lon1) * M_PI / 180.0;
    double a = sin(delta_phi / 2) * sin(delta_phi / 2) +
               cos(phi1) * cos(phi2) * sin(delta_lambda / 2) * sin(delta_lambda / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    return R * c;
}

std::chrono::system_clock::time_point parseTimestamp(const std::string& datetime) {
    std::tm tm = {};
    std::istringstream ss(datetime);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    size_t pos = datetime.find('.');
    if (pos != std::string::npos) {
        std::string ms_str = datetime.substr(pos + 1, 3);
        try {
            int ms = std::stoi(ms_str);
            tp += std::chrono::milliseconds(ms);
        } catch (...) {}
    }
    return tp;
}

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
    std::string inputPath = "input/vehicle_objects_10min_nodimensions.csv";
    std::ifstream file(inputPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open input file." << std::endl;
        return 1;
    }

    size_t pos = inputPath.find_last_of("/\\");
    std::string filename = (pos != std::string::npos) ? inputPath.substr(pos + 1) : inputPath;
    std::string outputPath = "output/" + filename;

    std::string header;
    std::getline(file, header);
    std::vector<std::string> columns = split(header, ',');

    int idIdx = -1, latIdx = -1, lonIdx = -1, speedIdx = -1, headingIdx = -1, timestampIdx = -1, sensorIdx = -1;
    for (size_t i = 0; i < columns.size(); ++i) {
        if (columns[i] == "id") idIdx = i;
        else if (columns[i] == "map_matched_lat") latIdx = i;
        else if (columns[i] == "map_matched_lon") lonIdx = i;
        else if (columns[i] == "horizontal_speed") speedIdx = i;
        else if (columns[i] == "heading") headingIdx = i;
        else if (columns[i] == "timestamp") timestampIdx = i;
        else if (columns[i] == "sensor_id") sensorIdx = i;
    }

    if (idIdx == -1 || latIdx == -1 || lonIdx == -1 || speedIdx == -1 ||
        headingIdx == -1 || timestampIdx == -1 || sensorIdx == -1) {
        std::cerr << "Required columns missing." << std::endl;
        return 1;
    }

    std::map<std::pair<std::string, std::string>, std::vector<VehicleData>> vehicleTracks;
    std::vector<std::vector<std::string>> allRows;
    std::string line;
    while (std::getline(file, line)) {
        std::vector<std::string> row = split(line, ',');
        int maxIndex = std::max(std::max(std::max(idIdx, latIdx), std::max(lonIdx, speedIdx)),
                                std::max(std::max(headingIdx, timestampIdx), sensorIdx));
        if (row.size() <= maxIndex) continue;

        allRows.push_back(row);

        try {
            VehicleData v;
            v.id = row[idIdx];
            v.sensor_id = row[sensorIdx];
            v.lat = std::stod(row[latIdx]);
            v.lon = std::stod(row[lonIdx]);
            v.speed = std::stod(row[speedIdx]);
            v.heading = std::stod(row[headingIdx]);
            v.timestamp = parseTimestamp(row[timestampIdx]);
            vehicleTracks[{v.sensor_id, v.id}].push_back(v);
        } catch (...) {
            continue;
        }
    }

    file.close();

    std::map<std::pair<std::string, std::string>, int> groupAssignments;
    int groupId = 1;
    std::set<std::pair<std::string, std::string>> used;

    for (auto it1 = vehicleTracks.begin(); it1 != vehicleTracks.end(); ++it1) {
        if (used.count(it1->first)) continue;
        groupAssignments[it1->first] = groupId;
        used.insert(it1->first);

        for (auto it2 = std::next(it1); it2 != vehicleTracks.end(); ++it2) {
            if (used.count(it2->first)) continue;
            if (it1->first.first != it2->first.first) continue;

            double totalDist = 0;
            double totalSpeedDiff = 0;
            double totalHeadingDiff = 0;
            size_t comparisons = 0;

            auto& traj1 = it1->second;
            auto& traj2 = it2->second;

            size_t len = std::min(traj1.size(), traj2.size());
            for (size_t i = 0; i < len; ++i) {
                double dist = calculateDistance(traj1[i].lat, traj1[i].lon, traj2[i].lat, traj2[i].lon);
                double speedDiff = std::abs(traj1[i].speed - traj2[i].speed);
                double headingDiff = std::abs(traj1[i].heading - traj2[i].heading);
                if (headingDiff > 180) headingDiff = 360 - headingDiff;

                totalDist += dist;
                totalSpeedDiff += speedDiff;
                totalHeadingDiff += headingDiff;
                comparisons++;
            }

            if (comparisons > 0) {
                double avgDist = totalDist / comparisons;
                double avgSpeedDiff = totalSpeedDiff / comparisons;
                double avgHeadingDiff = totalHeadingDiff / comparisons;
                // if (avgDist < 3.0 && avgSpeedDiff < 0.5 && avgHeadingDiff < 10.0) {
                if (avgSpeedDiff < 5.0 && avgHeadingDiff < 10.0) {
                    groupAssignments[it2->first] = groupId;
                    used.insert(it2->first);
                }
            }
        }

        groupId++;
    }

    std::ofstream out(outputPath);
    out << header << ",group_id\n";
    for (const auto& row : allRows) {
        for (size_t i = 0; i < row.size(); ++i) {
            out << row[i];
            if (i < row.size() - 1) out << ",";
        }
        std::pair<std::string, std::string> key = {row[sensorIdx], row[idIdx]};
        if (groupAssignments.count(key)) {
            out << "," << groupAssignments[key];
        } else {
            out << ",";
        }
        out << "\n";
    }
    out.close();

    std::cout << "Grouped data saved to: " << outputPath << std::endl;
    return 0;
}
