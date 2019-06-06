#include <c74_min.h>
#include <functional>
#include <boost/asio.hpp>
#include <oscpp/server.hpp>

namespace o {

    class radartouch : public c74::min::object<radartouch> {

      public:
        enum class parser_state { ALIVE, SET, FSEQ };

        using b_outlet = c74::min::outlet<c74::min::thread_check::any,
                                          c74::min::thread_action::fifo>;

        radartouch() {}

        radartouch(const c74::min::atoms& args) {

            if (args.size() <= 0) throw std::runtime_error("No port specified");

            auto args_it = args.begin();

            try {

                port = c74::min::atom::get<long>(*args_it);

                if (++args_it != args.end())
                    max_blobs = c74::min::atom::get<long>(*args_it);

                cout << "Port: " << port << " Max Blobs: " << max_blobs
                     << c74::min::endl;

                if (max_blobs) {

                    for (int i = 0; i < max_blobs; ++i) {
                        std::stringstream outlet_name;

                        outlet_name << "Output #" << i << " (list)"
                                    << std::endl;

                        outlets_.emplace_back(std::make_unique<b_outlet>(
                            this, outlet_name.str(), "list"));

                        blobs.push_back(
                            std::make_tuple(false, -1, std::array<float, 5>()));
                    }

                } else {
                    outlets_.push_back(std::make_unique<b_outlet>(
                        this, "Output (list)", "list"));
                }

            } catch (c74::min::bad_atom_access ex) {
                throw std::runtime_error(ex.what());
            }

            try {

                socket_ = std::make_unique<boost::asio::ip::udp::socket>(
                    ctx_, boost::asio::ip::udp::endpoint(
                              boost::asio::ip::udp::v4(), port));

            } catch (std::exception e) {

                std::stringstream errortext;

                errortext << "Could not bind to Port: " << port
                          << ". Maybe another Application bound to this port?"
                          << std::endl;

                throw std::runtime_error(errortext.str());
            }

            cout << "Bound to Port: " << port << c74::min::endl;

            buffer_.reserve(4096);

            do_read();

            network_thread_ = std::make_unique<std::thread>(
                std::bind(&radartouch::network_run, this));
        }

        ~radartouch() {

            destroy_ = true;

            if (work_.owns_work()) work_.reset();

            if (socket_) socket_->cancel();

            if (network_thread_) {
                if (network_thread_->joinable()) network_thread_->join();
            }
        }

        void do_read() {
            if (socket_) {
                socket_->async_receive(
                    boost::asio::buffer(buffer_.data(), buffer_.capacity()),
                    std::bind(&radartouch::on_data_received, this,
                              std::placeholders::_1, std::placeholders::_2));
            }
        }

        void on_data_received(boost::system::error_code ec,
                              size_t bytes_received) {

            if (ec) {
                cerr << ec.message() << c74::min::endl;
                return;
            }

            if (bytes_received <= 0) return do_read();

            OSCPP::Server::Packet pack{buffer_.data(), bytes_received};

            cout << "received data: " << bytes_received << c74::min::endl;

            if (!pack.isBundle()) {
                cerr << "Received invalid data, expected OSC Bundle"
                     << c74::min::endl;
                return do_read();
            }

            std::lock_guard<std::mutex> blobs_lock(blobs_mtx_);

            handleBundle(OSCPP::Server::Bundle(pack));

            if (!destroy_) do_read();
        }

        void handleBundle(const OSCPP::Server::Bundle& bundle) {

            OSCPP::Server::PacketStream packets{bundle.packets()};

            while (!packets.atEnd()) {

                auto next = packets.next();

                if (next.isMessage())
                    handleMessage(next);
                else
                    handleBundle(next);
            }
        }

        void handleMessage(const OSCPP::Server::Message& msg) {

            if (strcmp(msg.address(), "/tuio/2Dcur") != 0) return;

            auto args = msg.args();

            try {

                std::string ty = args.string();

                if (ty == "alive") {

                    pstate = parser_state::ALIVE;

                    std::vector<long> alive_args;

                    while (!args.atEnd())
                        alive_args.push_back(args.int32());

                    message_alive(alive_args);

                } else if (ty == "set") {

                    if (pstate == parser_state::FSEQ) return message_begin();

                    if (pstate == parser_state::ALIVE) {
                        pstate = parser_state::SET;
                        message_begin();
                    }

                    std::vector<double> set_args;

                    int id = args.int32();

                    while (!args.atEnd())
                        set_args.push_back(args.float32());

                    message_set(id, set_args);

                } else if (ty == "fseq") {

                    if (pstate == parser_state::ALIVE ||
                        pstate == parser_state::FSEQ)
                        return message_begin();

                    message_fseq(args.int32());

                    pstate = parser_state::FSEQ;

                    message_set_complete();
                }

            } catch (std::exception e) {
                cerr << e.what() << c74::min::endl;
            }
        }

        void message_fseq(int current_fseq) {

            if (!fseq)
                fseq = current_fseq;

            else if (current_fseq > fseq) {

                if (!(current_fseq == (fseq + 1)))
                    cerr << "Lost " << current_fseq - fseq << " Packets"
                         << c74::min::endl;

                fseq = current_fseq;

            } else {

                if ((fseq - current_fseq) > 20)
                    fseq = current_fseq;
                else
                    message_begin();
            }
        }

        void message_alive(std::vector<long>& args) {

            if (max_blobs) {

                for (auto& arg : args) {

                    auto it = std::find_if(
                        blobs.begin(), blobs.end(),
                        [&](auto& blob) { return std::get<1>(blob) == arg; });

                    if (it == blobs.end()) {
                        (*std::find_if(blobs.begin(), blobs.end(),
                                       [&](auto& blob) {
                                           return std::get<0>(blob) == false;
                                       })) =
                            std::make_tuple(true, arg, std::array<float, 5>());
                    } else
                        (*it) =
                            std::make_tuple(true, arg, std::array<float, 5>());
                }
            }
        }

        void message_set(int id, std::vector<double>& args) {

            if (args.size() != 5) return;

            if (max_blobs) {

                for (auto& blob : blobs) {

                    if (std::get<1>(blob) == id) {

                        std::get<0>(blob) = true;
                        std::copy(args.begin(), args.end(),
                                  std::get<2>(blob).begin());
                    }
                }
            } else {

                blobs.push_back(
                    std::make_tuple(true, id, std::array<float, 5>()));

                std::copy(args.begin(), args.end(),
                          std::get<2>(*(blobs.end() - 1)).begin());
            }
        }

        void message_set_complete() {
            
            if (max_blobs) {

                for (int i = 0; i < max_blobs; ++i) {

                        if (std::get<0>(blobs[i])) {

                            outlets_[i]->send(
                                std::get<1>(blobs[i]), std::get<2>(blobs[i])[0],
                                std::get<2>(blobs[i])[1], std::get<2>(blobs[i])[2],
                                std::get<2>(blobs[i])[3], std::get<2>(blobs[i])[4]);
                        }
                   
                }
            } else {

                for (auto& blob : blobs) {
                    outlets_[0]->send(
                        std::get<1>(blob), std::get<2>(blob)[0],
                        std::get<2>(blob)[1], std::get<2>(blob)[2],
                        std::get<2>(blob)[3], std::get<2>(blob)[4]);
                }
            }

            message_begin();
        }

        void message_begin() {
            
            if(max_blobs){
                for (auto& blob : blobs)
                    std::get<0>(blob) = false;
            } else {
                blobs.clear();
            }
            
        }

        void network_run() { ctx_.run(); }

        long fseq = 0;

        long port;
        long max_blobs = 0;

        bool destroy_ = false;

        parser_state pstate = parser_state::FSEQ;
        long parser_set_index;
        bool reading_msg;

        using blob = std::tuple<bool, int, std::array<float, 5>>;

        std::vector<blob> blobs;

        c74::min::inlet<> inlet_{this, "commands input", "list"};

        boost::asio::io_context ctx_;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
            work_{ctx_.get_executor()};

        boost::asio::ip::udp::endpoint sender;
        std::unique_ptr<boost::asio::ip::udp::socket> socket_;

        std::mutex blobs_mtx_;

        std::unique_ptr<std::thread> network_thread_;

        std::vector<std::unique_ptr<b_outlet>> outlets_;
        std::vector<char> buffer_;
    };
}

MIN_EXTERNAL(o::radartouch);
