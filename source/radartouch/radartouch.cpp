#include <c74_min.h>
#include <functional>

class radartouch : public c74::min::object<radartouch> {

  public:
    radartouch(const c74::min::atoms& args = {}) {}

    c74::min::inlet<> data_in{this, "data in"};

    c74::min::outlet<> data_out{this, "data out"};
    c74::min::outlet<> ids_out{this, "ids out"};
    c74::min::outlet<> shit_out{this, "shit out"};

    c74::min::atoms test(const c74::min::atoms& args, int inlet) {
        return args;
    }

    c74::min::message<c74::min::threadsafe::no> reset_msg{
        this, "reset", std::bind(&radartouch::test, this, std::placeholders::_1,
                                 std::placeholders::_2),
        "reset all data"};
};

MIN_EXTERNAL(radartouch);
