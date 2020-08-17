#include <chrono>
#include <functional>
#include <vector>

#include "network/protocol.hpp"
#include "serialize.hpp"

using namespace std;
using namespace thallium;
using namespace std::chrono;

void function_time(const string &name, const function<void()> &f,
                   const int dup = 1) {
    auto t_start = steady_clock::now();

    for (int i = 0; i < dup; i++) {
        f();
    }

    auto t_end = steady_clock::now();

    auto dur = duration<double>(t_end - t_start).count();
    cout << "Test: " << name << "Execution time: " << dur << endl;
}

class DataHolder {
  public:
    vector<vector<char>> data;
    template <class Archive>
    void serializable(Archive &ar) {
        ar &data;
    }
};

void test_serialization_save() {
    DataHolder d1;
    int s1 = 1024, s2 = 64;
    for (int i = 0; i < s1; i++) {
        d1.data.push_back(vector<char>('z', s2));
    }

    auto dynamic_memory = function<void()>([&](){
        Serializer::StringSaveArchive ar;
        ar << d1;
        string ret = ar.build();
        return ret.size();
    });

    auto static_memory = function<void()>([&](){
        auto buf = message::build<message::ZeroCopyBuffer>(d1);
        return buf.size();
    });
    
    function_time("dynamic_memory", dynamic_memory, 500);
    function_time("static_memory", static_memory, 500);
}

int main()
{
    test_serialization_save(); 
    return 0;
}
