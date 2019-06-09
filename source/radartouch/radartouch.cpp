#include <c74_min.h>
#include <functional>
#include <radartouch_receiver.h>

namespace o {

    class radartouch : public c74::min::object<radartouch>, public radartouch_receiver {

    public:

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
                    
                    outlet_states.resize(max_blobs);
                    
                    std::fill_n(outlet_states.begin(), max_blobs, std::make_tuple(-1, false, false));

                } else {

/*                    outlets_.push_back(std::make_unique<b_outlet>(
                        this, "Output (list)", "list")); */
                    
                    throw std::runtime_error("No max blobs specified");
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
            
            std::vector<std::pair<long, radartouch_message::blob>> output;
            
            std::vector<radartouch_message::blob> new_blobs;
            
            std::vector<long> alive_indc;
            
            std::vector<long> revived_indc;

            for (auto& blob : msg.blobs()) {

                // find the position where this blob was sent out last
                auto last_outlet_it = std::find_if(
                    outlet_states.begin(), outlet_states.end(), [&](auto o_ls_id) {
                        return std::get<0>(o_ls_id) == blob.bid;
                    });

                // add to output list if id was found
                if (last_outlet_it != outlet_states.end()){
                    
                    output.push_back(std::make_pair(last_outlet_it - outlet_states.begin(), blob));
                    
                    if(!std::get<2>(*last_outlet_it))
                        revived_indc.push_back(last_outlet_it - outlet_states.begin());
                    
                    std::get<0>(*last_outlet_it) = blob.bid;
                    std::get<1>(*last_outlet_it) = true; // will output
                    std::get<2>(*last_outlet_it) = true; // active this time
                }
                // add to new blobs list if not
                else
                    new_blobs.push_back(blob);
                
            }
            
            std::vector<long> died_indc;
    
            // find blobs that were active the last time but are missing this time
            for(int i = 0; i < outlet_states.size(); ++i){
                if( (!std::get<1>(outlet_states[i])) && std::get<2>(outlet_states[i]) ){
                    died_indc.push_back(i);
                    std::get<2>(outlet_states[i]) = false;
                }
            }
            
            // assign new blobs to inactive outlets
            for(auto& new_b : new_blobs){

                // find free outlet
                auto it =
                    std::find_if(outlet_states.begin(), outlet_states.end(),
                                 [&](auto ind) { return !std::get<1>(ind); });

                // add to output and alive list
                if(it != outlet_states.end()){
                    
                    std::get<1>(*it) = true;
                    std::get<2>(*it) = true;
                    
                    std::get<0>(*it) = new_b.bid;
                    
                    long index = it - outlet_states.begin();
                    
                    alive_indc.push_back(index);
                    
                    output.push_back(std::make_pair(index, new_b));

                }
            }
            
            // output "died" messages
            for(auto ind : died_indc){
                outlets_[ind]->send("died");
            }
            
            // output "alive messages"
            for(long ind : alive_indc){
                outlets_[ind]->send("alive");
            }
            
            // output "revive messages"
            for(long ind : revived_indc){
                outlets_[ind]->send("revive");
            }
            
            // sort output hi - lo, so we output max-style from right to left
            std::sort(output.begin(), output.end(), [&](auto lhs, auto rhs){
                return std::get<0>(lhs) > std::get<0>(rhs);
            });

            // output the values
            for (auto& out : output) {
                outlets_[out.first]->send(
                    static_cast<int>(out.second.bid), out.second.args[0],
                    out.second.args[1], out.second.args[2], out.second.args[3],
                    out.second.args[4]);
            }

            // reset outlet active flags
            for(auto& ols : outlet_states)
                std::get<1>(ols) = false;
            
            
        }
            
        // TODO: reset msg

            
        long max_blobs = 0;

        c74::min::inlet<> inlet_{this, "commands input", "list"};

        std::mutex blobs_mtx_;

        std::vector<std::unique_ptr<b_outlet>> outlets_;
        std::vector<std::tuple<long, bool, bool>> outlet_states;
    };
}

MIN_EXTERNAL(o::radartouch);
