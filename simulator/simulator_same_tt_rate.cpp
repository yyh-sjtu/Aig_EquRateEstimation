#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <cmath>
#include <queue>
#include <time.h>
#define rep(p, q) for (int p=0; p<q; p++)
#define PI 0
#define AND 1
#define NOT 2
#define STATE_WIDTH 16
#define CONNECT_SAMPLE_RATIO 0.1
using namespace std;

int countOnesInBinary(uint64_t num, int width) {
    int count = 0;
    rep (_, width) {
        if (num & 1) {
            count++;
        }
        num >>= 1;
    }
    return count;
}

vector<vector<uint64_t> > get_tt(string file_name, vector<vector<uint64_t> > input_pattern)
{
    string in_filename = file_name;

    cout << "Read File: " << in_filename << endl;
    freopen(in_filename.c_str(), "r", stdin);
    int n, m;  // number of gates
    int no_patterns; 
    scanf("%d %d %d", &n, &m, &no_patterns);
    cout << "Number of gates: " << n << endl;

    // Graph
    vector<int> gate_list(n);
    vector<vector<int> > fanin_list(n);
    vector<vector<int> > fanout_list(n);
    vector<int> gate_levels(n);
    vector<int> pi_list;
    int max_level = 0;

    for (int k=0; k<n; k++) {
        int type, level;
        scanf("%d %d", &type, &level);
        gate_list[k] = type;
        gate_levels[k] = level;
        if (level > max_level) {
            max_level = level;
        }
        if (type == PI) {
            pi_list.push_back(k);
        }
    }
    vector<vector<int> > level_list(max_level+1);
    for (int k=0; k<n; k++) {
        level_list[gate_levels[k]].push_back(k);
    }
    for (int k=0; k<m; k++) {
        int fanin, fanout;
        scanf("%d %d", &fanin, &fanout);
        fanin_list[fanout].push_back(fanin);
        fanout_list[fanin].push_back(fanout);
    }

    int no_pi = pi_list.size();
    cout << "Number of PI: " << no_pi << endl;

    cout<<"Start Simulation"<<endl;
    // Simulation
    vector<vector<uint64_t> > full_states(n); 
    int tot_clk = 0;
    int clk_cnt = 0; 

    int count_input_pattern = 0;
    while (no_patterns > 0) {
        no_patterns -= STATE_WIDTH; 
        vector<uint64_t> states(n);
        // generate pi patterns 
        rep(k, no_pi) {
            int pi = pi_list[k];
            // states[pi] = rand() % int(std::pow(2, STATE_WIDTH));
            states[pi] = input_pattern[pi][count_input_pattern]; 
            // cout << "PI: " << pi << " " << states[pi] << endl;
        }
        count_input_pattern++;
        // Combination
        for (int l = 1; l < max_level+1; l++) {
            for (int gate: level_list[l]) {
                if (gate_list[gate] == AND) {
                    uint64_t res = (states[fanin_list[gate][0]] & states[fanin_list[gate][1]]); 
                    states[gate] = res;
                    // cout << gate << ": " << (res & 1) << " " << (states[fanin_list[gate][0]] & 1) << " " << (states[fanin_list[gate][1]] & 1) << endl;
                }
                else if (gate_list[gate] == NOT) {
                    uint64_t res = ~states[fanin_list[gate][0]]; 
                    states[gate] = res;
                }
            }
        }
        // Record
        rep (k, n) {
            full_states[k].push_back(states[k]);
        }
    }
    fclose(stdin);
    return full_states;
}

int get_diff_bits(vector<uint64_t> &t1, vector<uint64_t> &t2){
    int cnt = 0;
    int tot_cnt = 0;
    int all_bits = 0;
    rep(p, t1.size()) {

        cnt = countOnesInBinary(t1[p]^t2[p], STATE_WIDTH);
        tot_cnt += cnt;
        all_bits += STATE_WIDTH;
    }
    return tot_cnt;
}

float get_equ_rate(vector<vector<uint64_t> > &tt1, vector<vector<uint64_t> > &tt2, vector<int> &pi_list){
    int start = pi_list.size();
    int cnt = 0;
    for(int i = start; i < tt1.size(); i++){
        for(int j = start; j < tt2.size(); j++){
            if(get_diff_bits(tt1[i], tt2[j]) == 0){
                cnt++;
            }
        }
    }

    return float(cnt) / float(max(tt1.size() - start, tt2.size() - start));
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        cout << "Failed" << endl;
        return 1;
    }
    string in_filename1 = argv[1];
    string in_filename2 = argv[2];
    // string out_filename = argv[3];

    cout << "Read File: " << in_filename1 << endl;
    freopen(in_filename1.c_str(), "r", stdin);
    int n, m;  // number of gates
    int no_patterns;
    scanf("%d %d %d", &n, &m, &no_patterns);
    cout << "Number of gates: " << n << endl;

    // Graph
    vector<int> gate_list(n);
    vector<vector<int> > fanin_list(n);
    vector<vector<int> > fanout_list(n);
    vector<int> gate_levels(n);
    vector<int> pi_list;
    int max_level = 0;

    for (int k=0; k<n; k++) {
        int type, level;
        scanf("%d %d", &type, &level);
        gate_list[k] = type;
        gate_levels[k] = level;
        if (level > max_level) {
            max_level = level;
        }
        if (type == PI) {
            pi_list.push_back(k);
        }
    }
    fclose(stdin);

    int no_pi = pi_list.size();
    cout << "Number of PI: " << no_pi << endl;

    // rep(k, no_pi) {
    //     int pi = pi_list[k];
    //     input_pattern[pi] = rand() % int(std::pow(2, STATE_WIDTH));
    // // cout << "PI: " << pi << " " << states[pi] << endl;
    // }
    vector<vector<uint64_t> > input_pattern(no_pi); 

    while (no_patterns > 0) {
        no_patterns -= STATE_WIDTH; 
        vector<uint64_t> states(n);
        // generate pi patterns 
        rep(k, no_pi) {
            int pi = pi_list[k];
            states[pi] = rand() % int(std::pow(2, STATE_WIDTH)); 
            // cout << "PI: " << pi << " " << states[pi] << endl;
        }
        
        // Record
        rep (k, no_pi) {
            input_pattern[k].push_back(states[k]);
        }

    }

    vector<vector<uint64_t> > tt1 = get_tt(in_filename1, input_pattern);
    vector<vector<uint64_t> > tt2 = get_tt(in_filename2, input_pattern);

    // for(int i = 0; i < tt1.size(); i++){
    //     for(int j = 0; j < tt1[i].size(); j++){
    //         cout << tt1[i][j] << " ";
    //     }
    //     cout << endl;
    // }

    float equ_rate = get_equ_rate(tt1, tt2, pi_list);
    printf("Equivalence Rate: %f\n", equ_rate);

    return 0;
}