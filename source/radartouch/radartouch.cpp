#include <c74_min.h>
#include <functional>
#include <radartouch_receiver.h>

namespace o {

    class radartouch : public c74::min::object<radartouch>, public radartouch_receiver {

      public:
        enum class parser_state { ALIVE, SET, FSEQ };

        using b_outlet = c74::min::outlet<c74::min::thread_check::any,
                                          c74::min::thread_action::fifo>;

        radartouch() {}

        radartouch(const c74::min::atoms& args) {

            if (args.size() <= 0) throw std::runtime_error("No port specified");

            auto args_it = args.begin();
            
            long port;

            try {

                port = c74::min::atom::get<long>(*args_it);

                if (++args_it != args.end())
                    max_blobs = c74::min::atom::get<long>(*args_it);

                if (max_blobs) {

                    for (int i = 0; i < max_blobs; ++i) {
                        std::stringstream outlet_name;

                        outlet_name << "Output #" << i << " (list)"
                                    << std::endl;

                        outlets_.emplace_back(std::make_unique<b_outlet>(
                            this, outlet_name.str(), "list"));

                    }

                } else {
                    outlets_.push_back(std::make_unique<b_outlet>(
                        this, "Output (list)", "list"));
                }

            } catch (c74::min::bad_atom_access ex) {
                throw std::runtime_error(ex.what());
            }
            
            this->bind(port);


            do_read();

            this->set_init(true);
        }

        ~radartouch(){}
        
        void print_error_str(std::string str) final {
            cerr << str << c74::min::endl;
        }
            
        void print_warn_str(std::string str) final {
            cwarn << str << c74::min::endl;
        }
            
        void handle_radartouch_msg(radartouch_message&& msg) final {
            
            cout << "Received complete msg" << c74::min::endl;
            
            for(auto& blob : msg.blobs())
                cout << "Blob #" << blob.bid << " Arg0: " << blob.args[0] << c74::min::endl;
        }

            
        long max_blobs = 0;

        c74::min::inlet<> inlet_{this, "commands input", "list"};

        std::mutex blobs_mtx_;

        std::vector<std::unique_ptr<b_outlet>> outlets_;
    };
}

MIN_EXTERNAL(o::radartouch);
