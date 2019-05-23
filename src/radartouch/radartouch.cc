#include <c74_min.h>
#include <functional>

class radartouch : c74::min::object<radartouch> {
  public:
    radartouch(c74::min::atoms& args) {}

    c74::min::inlet<> in{this, "data in"};
    c74::min::outlet<> out{this, "data out"};

    c74::min::atoms test(const c74::min::atoms& args, int inlet) {
        return args;
    }

    c74::min::message<c74::min::threadsafe::no> reset_msg{
        this, "reset all data",
        std::bind(&radartouch::test, this, std::placeholders::_1,
                  std::placeholders::_2)};
};
