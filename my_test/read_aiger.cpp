#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <cassert>


std::pair<std::vector<std::vector<int>>, std::vector<std::vector<int>>> aig_to_xdata(const std::string& aig_filename, std::map<std::string, int> gate_to_index = {{"PI", 0}, {"AND", 1}, {"NOT", 2}}) {
    std::ifstream file(aig_filename);
    std::string line;
    std::vector<std::vector<int>> x_data;
    std::vector<std::vector<int>> edge_index;

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << aig_filename << std::endl;
        return {x_data, edge_index};  // Return empty if the file cannot be opened
    }

    std::getline(file, line);
    std::istringstream header_stream(line);
    std::string temp;
    std::vector<std::string> header;
    while (header_stream >> temp) header.push_back(temp);

    int n_variables = std::stoi(header[1]);
    int n_inputs = std::stoi(header[2]);
    int n_outputs = std::stoi(header[4]);
    int n_and = std::stoi(header[5]);
    int no_latch = std::stoi(header[3]);
    assert(no_latch == 0 && "The AIG has latches.");

    // Initialize x_data for inputs and AND gates
    for (int i = 0; i < n_inputs; ++i) {
        x_data.push_back({x_data.size(), gate_to_index["PI"]});
    }
    for (int i = 0; i < n_and; ++i) {
        x_data.push_back({x_data.size(), gate_to_index["AND"]});
    }

    std::vector<int> has_not(x_data.size() + 1, -1);
    int line_count = 0;

    // Process AND connections
    while (std::getline(file, line)) {
        line_count++;
        if (line_count <= n_inputs + n_outputs) continue;

        std::istringstream iss(line);
        std::vector<std::string> arr;
        while (iss >> temp) arr.push_back(temp);

        if (arr.size() != 3) continue;

        int and_index = std::stoi(arr[0]) / 2 - 1;
        int fanin_1_index = std::stoi(arr[1]) / 2 - 1;
        int fanin_2_index = std::stoi(arr[2]) / 2 - 1;
        int fanin_1_not = std::stoi(arr[1]) % 2;
        int fanin_2_not = std::stoi(arr[2]) % 2;

        if (fanin_1_not == 1) {
            if (has_not[fanin_1_index] == -1) {
                x_data.push_back({static_cast<int>(x_data.size()), gate_to_index["NOT"]});
                int not_index = x_data.size() - 1;
                edge_index.push_back({fanin_1_index, not_index});
                has_not[fanin_1_index] = not_index;
            }
            fanin_1_index = has_not[fanin_1_index];
        }

        if (fanin_2_not == 1) {
            if (has_not[fanin_2_index] == -1) {
                x_data.push_back({static_cast<int>(x_data.size()), gate_to_index["NOT"]});
                int not_index = x_data.size() - 1;
                edge_index.push_back({fanin_2_index, not_index});
                has_not[fanin_2_index] = not_index;
            }
            fanin_2_index = has_not[fanin_2_index];
        }

        edge_index.push_back({fanin_1_index, and_index});
        edge_index.push_back({fanin_2_index, and_index});
    }

    file.close();
    return {x_data, edge_index};
}

int main() {
    auto results = aig_to_xdata("/data/yunhaozhou/project/datapath_cec/data/32_32/and_wallace/top.aig");
    std::cout << "Nodes: " << results.first.size() << ", Edges: " << results.second.size() << std::endl;
    return 0;
}
