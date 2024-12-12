#include <fstream>
#include <iostream>
#include <regex>
#include <cassert>
using namespace std;
#define PI 0
#define AND 1
#define NOT 2

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
  std::cout << "num_and: " << num_and << std::endl;
  std::cout << "num_pi: " << num_pi << std::endl;
  std::cout << "num_not: " << num_not << std::endl;

  for(int i = 0; i < pos.size(); i++){
    std::cout << "pos: " << pos[i] << std::endl;
  }

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



int main(int argc, char **argv){
    std::string filename = argv[1];
    // std::cout<< "read aiger file: "<<filename<<std::endl;
    std::vector<std::vector<int>> x_data;
    std::vector<std::vector<int>> edge_index;
    int res = read_aiger(filename, x_data, edge_index);
    if (res == 0){
        std::cout << "read aiger file " << filename << " successfully" << std::endl;
        std::cout << "x_data size: " << x_data.size() << std::endl;
        std::cout << "edge_index size: " << edge_index.size() << std::endl;
    }
    else{
        std::cout << "read aiger file " << filename << " failed" << std::endl;
    }

    for (int i = 0; i < edge_index.size(); i++){
        std::cout << edge_index[i][0] << "->" << edge_index[i][1] << std::endl;
    }
    return 0;
}