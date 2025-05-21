#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <tuple>
#include <filesystem>

const double EARTH_RADIUS = 6371000.0;
const double MAX_DIST = 10.0; // in meters
const double MAX_TIME_DIFF = 3.0; // in seconds
const double MAX_HEADING_DIFF = 20.0; // in degrees

struct Vehicle {
    std::vector<std::string> original_row;
    double id;
    double lat, lon;
    double heading;
    double speed;
    long long timestamp;
};

inline double deg2rad(double deg) {
    return deg * M_PI / 180.0;
}

double haversine(double lat1, double lon1, double lat2, double lon2) {
    double dlat = deg2rad(lat2 - lat1);
    double dlon = deg2rad(lon2 - lon1);
    lat1 = deg2rad(lat1);
    lat2 = deg2rad(lat2);
    double a = std::pow(std::sin(dlat / 2), 2) +
               std::cos(lat1) * std::cos(lat2) * std::pow(std::sin(dlon / 2), 2);
    return 2 * EARTH_RADIUS * std::asin(std::sqrt(a));
}

double heading_diff(double h1, double h2) {
    double diff = std::fabs(h1 - h2);
    return std::fmin(diff, 360 - diff);
}

// Union-Find
struct UnionFind {
    std::unordered_map<double, double> parent;
    double find(double x) {
        if (parent.find(x) == parent.end()) parent[x] = x;
        if (parent[x] != x) parent[x] = find(parent[x]);
        return parent[x];
    }
    void unite(double x, double y) {
        double rx = find(x), ry = find(y);
        if (rx != ry) parent[ry] = rx;
    }
};

std::vector<Vehicle> load_csv(const std::string& path, std::vector<std::string>& header) {
    std::ifstream file(path);
    std::string line;
    std::getline(file, line);
    header = {line + ",cluster_id_predicted"};

    std::vector<Vehicle> data;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string item;
        Vehicle v;
        v.original_row.push_back(line);

        std::vector<std::string> tokens;
        while (std::getline(ss, item, ',')) {
            tokens.push_back(item);
        }

        v.heading = std::stod(tokens[1]);
        v.speed = std::stod(tokens[4]);
        v.id = std::stod(tokens[6]);
        v.lon = std::stod(tokens[14]);
        v.lat = std::stod(tokens[15]);

        std::string ts = tokens[10].substr(11, 8);  // "hh:mm:ss"
        int h = std::stoi(ts.substr(0, 2));
        int m = std::stoi(ts.substr(3, 2));
        int s = std::stoi(ts.substr(6, 2));
        v.timestamp = h * 3600 + m * 60 + s;

        data.push_back(v);
    }
    return data;
}

void write_csv(const std::string& out_path, const std::vector<std::string>& header, const std::vector<Vehicle>& vehicles, UnionFind& uf) {
    std::ofstream out(out_path);
    out << header[0] << "\n";
    out << std::fixed << std::setprecision(0);
    for (const auto& v : vehicles) {
        out << v.original_row[0] << "," << uf.find(v.id) << "\n";
    }
}

int main() {
    const std::string input_path = "csv/10min_1_2.csv";
    const std::string output_path = "csv/10min_1_2_cluster.csv";

    std::vector<std::string> header;
    auto vehicles = load_csv(input_path, header);
    UnionFind uf;

    const int N = vehicles.size();
    for (int i = 0; i < N; ++i) {
        const auto& vi = vehicles[i];
        for (int j = i + 1; j < N; ++j) {
            const auto& vj = vehicles[j];

            if (std::fabs(vi.timestamp - vj.timestamp) > MAX_TIME_DIFF) continue;
            if (haversine(vi.lat, vi.lon, vj.lat, vj.lon) > MAX_DIST) continue;
            if (heading_diff(vi.heading, vj.heading) > MAX_HEADING_DIFF) continue;

            uf.unite(vi.id, vj.id);
        }
    }

    write_csv(output_path, header, vehicles, uf);
    std::cout << "Done. Output written to " << output_path << "\n";
    return 0;
}
