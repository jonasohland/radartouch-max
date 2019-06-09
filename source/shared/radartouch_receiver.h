#pragma once

#include <o.h>
#include <oscpp/server.hpp>
#include "radartouch_message.h"

namespace o {

    using udp_device = o::io::net::datagram_device<boost::asio::ip::udp,
                                                   std::string, o::ccy::strict>;

    using app = o::io::io_app_base<o::ccy::strict>;

    class radartouch_receiver : public app, public udp_device {

      public:
        radartouch_receiver() : udp_device(this->context()) {
            this->app_launch();
        }

        ~radartouch_receiver() {

            this->dgram_sock_close();

            this->app_allow_exit();

            this->app_join();
        }

        void set_init(bool is_init) { is_initialized = is_init; }

        void bind(long port) { this->dgram_sock_bind(port); }
        
        virtual void handle_radartouch_msg(radartouch_message&& msg){
            print_error_str("blob!");
        }
        
        virtual void print_warn_str(std::string str){}

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

            if (msg.args().atEnd()) return;
            
            auto margs = msg.args();
            
            try {

                auto ty = margs.string();

                if (strcmp(ty, "alive") == 0) {
                    
                    c_msg_.reset();
                    
                    std::vector<long> args;
                    
                    while(!margs.atEnd())
                        args.push_back(margs.int32());
                    
                    c_msg_.set_alive_ids(args);
                    
                    
                } else if (strcmp(ty, "set") == 0) {

                    std::array<float, 5> args;
                    
                    int bid = margs.int32();

                    auto args_it = args.begin();

                    do {
                        *args_it = margs.float32();
                    } while (++args_it != args.end() && !margs.atEnd());
                    
                    c_msg_.add_blob_args(bid, std::move(args));

                } else if (strcmp(ty, "fseq") == 0) {
                    
                    if (!c_msg_.valid()) return;
                    
                    long current_fseq = margs.int32();

                    if (!fseq)
                        fseq = current_fseq;
                    
                    else if (current_fseq > fseq) {
                        
                        if (!(current_fseq == (fseq + 1))){
                            
                            std::stringstream err;
                            
                            err << "lost " << current_fseq - fseq << " packets";
                            
                            print_warn_str(err.str());
                        }
                        
                        fseq = current_fseq;
                        
                    } else {
                        
                        if ((fseq - current_fseq) > 20)
                            fseq = current_fseq;
                        else
                            c_msg_.reset();
                    }
                    
                    c_msg_.close(fseq);
                    
                    handle_radartouch_msg(std::move(c_msg_));
                    
                    c_msg_.reset();
                }
                
            } catch (OSCPP::UnderrunError err) {
                print_error_str("UnderrunError");
            } catch(OSCPP::ParseError err){
                print_error_str("ParseError");
            }
            
            
        }

        void on_dgram_received(std::string&& data) final {

            auto pck = OSCPP::Server::Packet(data.data(), data.size());

            if (pck.isBundle())
                handleBundle(OSCPP::Server::Bundle(pck));
            else if (pck.isMessage())
                handleMessage(OSCPP::Server::Message(pck));
            else
                print_error_str("Could not parse data");
        }

        void on_dgram_sent() final {}

        void on_dgram_error(udp_device::error_case eca,
                            boost::system::error_code ec) final {

            switch (eca) {
            case udp_device::error_case::bind:
                break;
            case udp_device::error_case::connect:
                break;
            case udp_device::error_case::read:
                break;
            }

            print_error_str(ec.message());

            if (!is_initialized) throw std::runtime_error(ec.message());
        }

        virtual void print_error_str(std::string str) {}

        bool is_initialized = false;

        radartouch_message c_msg_;
        long fseq = 0;
    };
};
