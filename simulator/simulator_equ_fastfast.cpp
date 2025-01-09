#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <cmath>
#include <queue>
#include <time.h>
#include <fstream>
#include <regex>
#include <cassert>
#include <queue>
#define rep(p, q) for (int p=0; p<q; p++)
#define PI 0
#define AND 1
#define NOT 2
#define STATE_WIDTH 16
#define CONNECT_SAMPLE_RATIO 0.1
using namespace std;


namespace detail
{
inline std::istream& getline( std::istream& is, std::string& t )
{
  t.clear();

  /* The characters in the stream are read one-by-one using a std::streambuf.
   * That is faster than reading them one-by-one using the std::istream.
   * Code that uses streambuf this way must be guarded by a sentry object.
   * The sentry object performs various tasks,
   * such as thread synchronization and updating the stream state.
   */

  std::istream::sentry se( is, true );
  std::streambuf* sb = is.rdbuf();

  for ( ;; )
  {
    int const c = sb->sbumpc();
    switch ( c )
    {
    /* deal with file endings */
    case '\n':
      return is;
    case '\r':
      if ( sb->sgetc() == '\n' )
      {
        sb->sbumpc();
      }
      return is;

    /* handle the case when the last line has no line ending */
    case std::streambuf::traits_type::eof():
      if ( t.empty() )
      {
        is.setstate( std::ios::eofbit );
      }
      return is;

    default:
      t += (char)c;
    }
  }
}
}

namespace aig_regex
{
static std::regex header( R"(^aig (\d+) (\d+) (\d+) (\d+) (\d+)( \d+)?( \d+)?( \d+)?( \d+)?$)" );
static std::regex ascii_header( R"(^aag (\d+) (\d+) (\d+) (\d+) (\d+)( \d+)?( \d+)?( \d+)?( \d+)?$)" );
static std::regex input( R"(^i(\d+) (.*)$)" );
static std::regex latch( R"(^l(\d+) (.*)$)" );
static std::regex output( R"(^o(\d+) (.*)$)" );
static std::regex bad_state( R"(^b(\d+) (.*)$)" );
static std::regex constraint( R"(^c(\d+) (.*)$)" );
static std::regex justice( R"(^j(\d+) (.*)$)" );
static std::regex fairness( R"(^f(\d+) (.*)$)" );
} // namespace aig_regex

int read_aiger_in( std::istream& in, std::vector<std::vector<int>> &x_data,  std::vector<std::vector<int>> &edge_index)
{
  std::vector<int> pos;

  std::smatch m;
  std::string header_line;
  detail::getline( in, header_line );

  uint32_t _m, _i, _l, _o, _a, _b, _c, _j, _f;

  /* parse header */
  if ( std::regex_search( header_line, m, aig_regex::header ) )
  {
    std::vector<uint32_t> header;
    for ( const auto& i : m )
    {
      if ( i == "" )
        continue;
      header.push_back( std::atol( std::string( i ).c_str() ) );
    }

    assert( header.size() >= 6u );
    assert( header.size() <= 10u );

    _m = header[1u];
    _i = header[2u];
    _l = header[3u];
    _o = header[4u];
    _a = header[5u];
    _b = header.size() > 6 ? header[6u] : 0u;
    _c = header.size() > 7 ? header[7u] : 0u;
    _j = header.size() > 8 ? header[8u] : 0u;
    _f = header.size() > 9 ? header[9u] : 0u;
  }
  else
  {
    return 1;
  }

  if(_l != 0){
    printf("Warning: AIGER has latches\n");
    return 1;
  }

  std::string line;

  /* inputs */
  for ( auto i = 0u; i < _i; ++i )
  {
    x_data.push_back({x_data.size(), PI});

  }

  /* gates */
  int num_gates = 0;
  for ( auto i = 0u; i < _a; ++i )
  {
    x_data.push_back({x_data.size(), AND});
  }


  for ( auto i = 0u; i < _o; ++i )
  {
    detail::getline( in, line );
    pos.push_back(std::atol( line.c_str() ));
  }


  std::vector<int> has_not(x_data.size() + 1, -1);
  int line_count = 0;

  const auto decode = [&]() {
    auto i = 0;
    auto res = 0;
    while ( true )
    {
      auto c = in.get();

      res |= ( ( c & 0x7f ) << ( 7 * i ) );
      if ( ( c & 0x80 ) == 0 )
        break;

      ++i;
    }
    return res;
  };

  /* and gates */
  for ( auto i = _i + 1; i < _i  + _a + 1; ++i )
  {
    const auto d1 = decode();
    const auto d2 = decode();
    // ( g - d1 ), ( g - d1 - d2 )
    const auto g = i << 1;
    const auto uLit1 = g - d1;
    const auto uLit2 = g - d1 - d2;

    int and_index = int(int(g) / 2) - 1;
    int fanin_1_index = int(int(uLit1) / 2) - 1;
    int fanin_2_index = int(int(uLit2) / 2) - 1;
    int fanin_1_not = int(uLit1) % 2;
    int fanin_2_not = int(uLit2) % 2;

    if (fanin_1_not == 1){
        if(has_not[fanin_1_index] == -1){
            x_data.push_back({x_data.size(), NOT});
            int not_index = x_data.size() - 1;
            edge_index.push_back({fanin_1_index, not_index});
            has_not[fanin_1_index] = not_index;
        }
        fanin_1_index = has_not[fanin_1_index];
    }

    if (fanin_2_not == 1){
        if(has_not[fanin_2_index] == -1){
            x_data.push_back({x_data.size(), NOT});
            int not_index = x_data.size() - 1;
            edge_index.push_back({fanin_2_index, not_index});
            has_not[fanin_2_index] = not_index;
        }
        fanin_2_index = has_not[fanin_2_index];
    }

    edge_index.push_back( {fanin_1_index, and_index} );
    edge_index.push_back( {fanin_2_index, and_index} );
  }

  // PO NOT check 
  for(int i = 0; i < pos.size(); i++){
    int po_index = int(int(pos[i]) / 2) - 1;
    if (po_index < 0){continue;}
    int po_not = int(pos[i]) % 2;
    if (po_not == 1){
        if (has_not[po_index] == -1){
            x_data.push_back({x_data.size(), NOT});
            int not_index = x_data.size() - 1;
            edge_index.push_back({po_index, not_index});
            has_not[po_index] = not_index;
        }
    }
  }

  int num_and = 0;
  int num_pi = 0;
  int num_not = 0;
  for (int i = 0; i < x_data.size(); i++){
    if (x_data[i][1] == AND){
      num_and++;
    } else if (x_data[i][1] == PI){
      num_pi++;
    } else if (x_data[i][1] == NOT){
      num_not++;
    }
  }
//   std::cout << "num_and: " << num_and << std::endl;
//   std::cout << "num_pi: " << num_pi << std::endl;
//   std::cout << "num_not: " << num_not << std::endl;

//   for(int i = 0; i < pos.size(); i++){
//     std::cout << "pos: " << pos[i] << std::endl;
//   }

  return 0;
}

int read_aiger( const std::string& filename, std::vector<std::vector<int>> &x_data, std::vector<std::vector<int>> &edge_index)
{
  std::ifstream in( filename , std::ifstream::binary );
  if ( !in.is_open() )
  {
    return 1;
  }
  else
  {
    auto const ret = read_aiger_in( in, x_data, edge_index);
    in.close();
    return ret;
  }
}

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


vector<vector<uint64_t> > get_tt(string filename, vector<vector<uint64_t> > input_pattern, int no_patterns)
{
    std::vector<std::vector<int>> x_data;
    std::vector<std::vector<int>> edge_index;

    int res = read_aiger(filename, x_data, edge_index);

    // if (res == 0){
    //     std::cout << "read aiger file " << filename << " successfully" << std::endl;
    //     std::cout << "x_data size: " << x_data.size() << std::endl;
    //     std::cout << "edge_index size: " << edge_index.size() << std::endl;
    // }
    // else{
    //     std::cout << "read aiger file " << filename << " failed" << std::endl;
    // }

    int n = x_data.size();
    // Graph
    vector<int> gate_list(n);
    vector<vector<int> > fanin_list(n);
    vector<vector<int> > fanout_list(n);
    vector<int> pi_list;
    
    for(int i = 0; i < x_data.size(); i++){
        int type = x_data[i][1];
        gate_list[i] = type;
        if (type == PI) {
            pi_list.push_back(i);
        }
    }

    for(int i = 0; i < edge_index.size(); i++){
        fanin_list[edge_index[i][1]].push_back(edge_index[i][0]);
    }
    for(int i = 0; i < edge_index.size(); i++){
        fanout_list[edge_index[i][0]].push_back(edge_index[i][1]);
    }

    int zero_fanin_nodes = 0;
    for(int i = 0; i < n; i++){
        if(fanin_list[i].size() == 0){
            zero_fanin_nodes++;
        }
    }
    cout << "Number of nodes with zero fanin: " << zero_fanin_nodes << endl;

    vector<vector<int> > level_list;
    int max_level = 0;
    // bfs
    queue<int> q;
    vector<int> visited(n, 0);
    for(int i = 0; i < pi_list.size(); i++){
        q.push(pi_list[i]);
    }

    while (!q.empty()) {
        vector<int> cur_level_list = {};
        int q_size = q.size();
        for(int i = 0; i < q_size; i++){
            int node = q.front();
            q.pop();
            cur_level_list.push_back(node);
            for (int fanout : fanout_list[node]) {
              if (gate_list[fanout] == AND) {
                if (visited[fanout] == 1) {
                    q.push(fanout);
                    visited[fanout] += 1;
                }
                else if (visited[fanout] == 0) {
                    visited[fanout] += 1;
                }
              }
              else if (gate_list[fanout] == NOT) {
                if (visited[fanout] == 0) {
                    q.push(fanout);
                    visited[fanout] += 1;
                }
              }
            }
        }
        level_list.push_back(cur_level_list);
    }
    max_level = level_list.size();
    //

    int no_pi = pi_list.size();
    cout << "Number of PI: " << no_pi << endl;

    cout<<"Start Simulation"<<endl;
    // Simulation
    vector<vector<uint64_t> > full_states(n); 

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

int tt_is_same(vector<uint64_t> &t1, vector<uint64_t> &t2){
    rep(p, t1.size()) {
        uint64_t diff = t1[p]^t2[p];
        if(diff != 0){
            return 0;
        }
    }
    return 1;
}

float get_equ_rate(vector<vector<uint64_t> > &tt1, vector<vector<uint64_t> > &tt2, vector<int> &pi_list){
    int start = pi_list.size();
    int cnt = 0;
    for(int i = start; i < tt1.size(); i++){
        for(int j = start; j < tt2.size(); j++){
            // if(get_diff_bits(tt1[i], tt2[j]) == 0){
            if(tt_is_same(tt1[i], tt2[j])){
                cnt++;
            }
        }
    }

    return float(cnt) / float(max(tt1.size() - start, tt2.size() - start));
}

int get_no_pi(string filename){
  std::smatch m;
  std::string header_line;

  std::ifstream in( filename , std::ifstream::binary );
  if ( !in.is_open() )
  {
    return -1;
  }

  detail::getline( in, header_line );

  uint32_t _m, _i, _l, _o, _a, _b, _c, _j, _f;

  /* parse header */
  if ( std::regex_search( header_line, m, aig_regex::header ) )
  {
    std::vector<uint32_t> header;
    for ( const auto& i : m )
    {
      if ( i == "" )
        continue;
      header.push_back( std::atol( std::string( i ).c_str() ) );
    }

    assert( header.size() >= 6u );
    assert( header.size() <= 10u );

    _m = header[1u];
    _i = header[2u];
    _l = header[3u];
    _o = header[4u];
    _a = header[5u];
    _b = header.size() > 6 ? header[6u] : 0u;
    _c = header.size() > 7 ? header[7u] : 0u;
    _j = header.size() > 8 ? header[8u] : 0u;
    _f = header.size() > 9 ? header[9u] : 0u;
  }
  else
  {
    return -1;
  }
  in.close();
  return _i;
}

int main(int argc, char **argv)
{
    if (!(argc == 3 || argc == 4)) {
        cout << "Failed" << endl;
        return 1;
    }
    string filename1 = argv[1];
    string filename2 = argv[2];

    int no_patterns = 15000;
    if(argc == 4) {
        no_patterns = atoi(argv[3]);
    }

    int no_patterns_save = no_patterns;

    std::vector<std::vector<int>> x_data;
    std::vector<std::vector<int>> edge_index;

    int res = read_aiger(filename1, x_data, edge_index);

    if (res == 0){
        std::cout << "read aiger file " << filename1 << " successfully" << std::endl;
        std::cout << "x_data size: " << x_data.size() << std::endl;
        std::cout << "edge_index size: " << edge_index.size() << std::endl;
    }
    else{
        std::cout << "read aiger file " << filename1 << " failed" << std::endl;
    }

    int n = x_data.size();
    // Graph
    vector<int> pi_list;
    
    for(int i = 0; i < x_data.size(); i++){
        int type = x_data[i][1];
        if (type == PI) {
            pi_list.push_back(i);
        }
    }

    int no_pi = get_no_pi(filename1);

    vector<vector<uint64_t> > input_pattern(no_pi); 

    while (no_patterns > 0) {
        no_patterns -= STATE_WIDTH; 
        vector<uint64_t> states(no_pi);
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
    
    vector<vector<uint64_t> > tt1 = get_tt(filename1, input_pattern, no_patterns_save);
    vector<vector<uint64_t> > tt2 = get_tt(filename2, input_pattern, no_patterns_save);

    float equ_rate = get_equ_rate(tt1, tt2, pi_list);
    printf("Equivalence Rate: %f\n", equ_rate);

    return 0;
}