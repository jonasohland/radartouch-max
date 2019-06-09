#include <c74_min.h>
#include <functional>
#include <radartouch_receiver.h>

namespace o {

    class radartouch_raw : public c74::min::object<radartouch_raw>,
                           public radartouch_receiver {

      public:
                               
        MIN_AUTHOR = "Jonas Ohland";
        MIN_DESCRIPTION = "Receive messages from radartouch";
        MIN_TAGS = "net, laser, radar, tracking, room";
        MIN_RELATED = "udpreceive, udpsend, nodes";

        using b_outlet = c74::min::outlet<c74::min::thread_check::any,
                                          c74::min::thread_action::fifo>;

        radartouch_raw() {}

        radartouch_raw(const c74::min::atoms& args) {

            if (args.size() <= 0) throw std::runtime_error("No port specified");

            auto args_it = args.begin();

            long port;

            try {
                port = c74::min::atom::get<long>(*args_it);
            } catch (c74::min::bad_atom_access& ex) {
                throw std::runtime_error("first arg must be int");
            }

            this->bind(port);

            this->do_read();

            this->set_init(true);
        }

        void print_error_str(std::string str) final {
            cerr << str << c74::min::endl;
        }

        void print_warn_str(std::string str) final {
            cwarn << str << c74::min::endl;
        }

        void handle_radartouch_msg(radartouch_message&& msg) final {

            auto blobs = msg.blobs();

            c74::min::atoms ids;

            ids.emplace_back("alive");

            for (auto& blob : blobs)
                ids.emplace_back(static_cast<int>(blob.bid));

            out_.send(ids);

            for (auto& blob : blobs)
                out_.send("set", static_cast<int>(blob.bid), blob.args[0],
                          blob.args[1], blob.args[2], blob.args[3],
                          blob.args[4]);

            out_.send("fseq", static_cast<int>(msg.fseq()));
        }

        b_outlet out_{this, "data out", "list"};
    };
}

void ext_main(void* r) {
    c74::max::object_post(
        nullptr, "radartouch_raw external %s // (c) 2019 Jonas Ohland ",
        O_THIS_TARGET_VERSION());
    c74::min::wrap_as_max_external<o::radartouch_raw>(
        "radartouch_raw", __FILE__, r);
}
