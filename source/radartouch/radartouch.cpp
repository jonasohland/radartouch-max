#include <c74_min.h>
#include <functional>
#include <radartouch_receiver.h>

namespace o {

    class radartouch : public c74::min::object<radartouch>,
                       public radartouch_receiver {

      public:
        MIN_AUTHOR = "Jonas Ohland";
        MIN_DESCRIPTION = "Receive messages from radartouch";
        MIN_TAGS = "net, laser, radar, tracking, room";
        MIN_RELATED = "udpreceive, udpsend, nodes";

        using b_outlet = c74::min::outlet<c74::min::thread_check::any,
                                          c74::min::thread_action::fifo>;

        radartouch() {}

        radartouch(const c74::min::atoms& args) {

            if (args.size() <= 0) throw std::runtime_error("No port specified");

            auto args_it = args.begin();

            long port;

            try {

                port = c74::min::atom::get<long>(*args_it);

                if (++args_it != args.end()) {
                    if (args_it->a_type == c74::max::e_max_atomtypes::A_LONG) {
                        max_blobs = c74::min::atom::get<long>(*args_it);
                    } else
                        --args_it;
                } else
                    --args_it;

                if (max_blobs) {

                    for (int i = 0; i < max_blobs; ++i) {
                        std::stringstream outlet_name;

                        outlet_name << "Output #" << i << " (list)"
                                    << std::endl;

                        outlets_.emplace_back(std::make_unique<b_outlet>(
                            this, outlet_name.str(), "list"));
                    }

                    outlet_states.resize(max_blobs);

                    std::fill_n(outlet_states.begin(), max_blobs,
                                std::make_tuple(-1, false, false));

                } else {

                    outlets_.push_back(std::make_unique<b_outlet>(
                        this, "Output (list)", "list"));
                }

                for (++args_it; args_it != args.end(); ++args_it) {
                    if (c74::min::atom::get<std::string>(*args_it) ==
                        "nodesout")
                        nodes_out_ = std::make_unique<b_outlet>(
                            this, "nodes outlet", "list");
                    else if (c74::min::atom::get<std::string>(*args_it) ==
                             "by_index")
                        by_index = true;
                    else if (c74::min::atom::get<std::string>(*args_it) ==
                             "by_type")
                        by_index = false;
                }

            } catch (c74::min::bad_atom_access ex) {
                throw std::runtime_error(ex.what());
            }

            this->bind(port);

            do_read();

            this->set_init(true);
        }

        ~radartouch() {}

        void print_error_str(std::string str) final {
            cerr << str << c74::min::endl;
        }

        void print_warn_str(std::string str) final {
            cwarn << str << c74::min::endl;
        }

        void output_status_msg(long index, const char* msg) {
            if (max_blobs) {
                outlets_[index]->send(msg);
            } else {
                if (by_index)
                    outlets_[0]->send(static_cast<int>(index), msg);
                else
                    outlets_[0]->send(msg, static_cast<int>(index));
            }
        }

        void handle_radartouch_msg(radartouch_message&& msg) final {

            std::vector<std::pair<long, radartouch_message::blob>> output;

            std::vector<radartouch_message::blob> new_blobs;

            std::vector<int> alive_indc;
            std::vector<int> revived_indc;
            std::vector<int> died_indc;

            for (auto& blob : msg.blobs()) {

                // find the position where this blob was sent out last
                auto last_outlet_it =
                    std::find_if(outlet_states.begin(), outlet_states.end(),
                                 [&](auto o_ls_id) {
                                     return std::get<0>(o_ls_id) == blob.bid;
                                 });

                // add to output list if id was found
                if (last_outlet_it != outlet_states.end()) {

                    output.push_back(std::make_pair(
                        last_outlet_it - outlet_states.begin(), blob));

                    if (!std::get<2>(*last_outlet_it))
                        revived_indc.push_back(last_outlet_it -
                                               outlet_states.begin());

                    std::get<0>(*last_outlet_it) = blob.bid;
                    std::get<1>(*last_outlet_it) = true; // will output
                    std::get<2>(*last_outlet_it) = true; // active this time
                }
                // add to new blobs list if not
                else
                    new_blobs.push_back(blob);
            }

            // find blobs that were active the last time but are missing this
            // time
            for (int i = 0; i < outlet_states.size(); ++i) {
                if ((!std::get<1>(outlet_states[i])) &&
                    std::get<2>(outlet_states[i])) {
                    died_indc.push_back(i);
                    std::get<2>(outlet_states[i]) = false;
                }
            }

            // assign new blobs to inactive outlets
            for (auto& new_b : new_blobs) {

                // find free outlet
                auto it =
                    std::find_if(outlet_states.begin(), outlet_states.end(),
                                 [&](auto ind) { return !std::get<1>(ind); });

                // add to output and alive list
                if (it != outlet_states.end()) {

                    std::get<1>(*it) = true;
                    std::get<2>(*it) = true;

                    std::get<0>(*it) = new_b.bid;

                    long index = it - outlet_states.begin();

                    alive_indc.push_back(index);

                    output.push_back(std::make_pair(index, new_b));

                    // add new state container to list if no more free outlets
                    // remaining
                } else if (max_blobs == 0) {

                    outlet_states.push_back(
                        std::make_tuple(new_b.bid, true, true));

                    long index = outlet_states.size() - 1;

                    alive_indc.push_back(index);

                    output.push_back(std::make_pair(index, new_b));
                }
            }

            // sort message lists
            std::sort(died_indc.begin(), died_indc.end(), std::greater<long>());
            std::sort(
                alive_indc.begin(), alive_indc.end(), std::greater<long>());
            std::sort(
                revived_indc.begin(), revived_indc.end(), std::greater<long>());

            // output "died" messages
            for (auto ind : died_indc)
                output_status_msg(ind, "died");

            // output "alive messages"
            for (long ind : alive_indc)
                output_status_msg(ind, "alive");

            // output "revive messages"
            for (long ind : revived_indc)
                output_status_msg(ind, "revive");

            // sort output hi - lo, so we output max-style from right to left
            std::sort(output.begin(), output.end(), [&](auto lhs, auto rhs) {
                return std::get<0>(lhs) > std::get<0>(rhs);
            });

            // output the values
            if (max_blobs) {
                // to corresponding outlets
                for (auto& out : output) {
                    outlets_[out.first]->send(
                        static_cast<int>(out.second.bid), out.second.args[0],
                        out.second.args[1], out.second.args[2],
                        out.second.args[3], out.second.args[4]);
                }
            } else {
                // or to outlet 0
                for (auto& out : output) {
                    if (by_index)
                        outlets_[0]->send(
                            static_cast<int>(out.first), "blob",
                            static_cast<int>(out.second.bid),
                            out.second.args[0], out.second.args[1],
                            out.second.args[2], out.second.args[3],
                            out.second.args[4]);
                    else
                        outlets_[0]->send(
                            "blob", static_cast<int>(out.first),
                            static_cast<int>(out.second.bid),
                            out.second.args[0], out.second.args[1],
                            out.second.args[2], out.second.args[3],
                            out.second.args[4]);
                }
            }

            // output setnodes msg
            if (nodes_out_) {
                for (auto& out : output)
                    nodes_out_->send("setnode", static_cast<int>(out.first) + 1,
                                     out.second.args[0], out.second.args[1]);
            }

            // reset outlet active flags
            for (auto& ols : outlet_states)
                std::get<1>(ols) = false;
        }

        // TODO: reset msg

        long max_blobs = 0;

        bool by_index = false;

        c74::min::inlet<> inlet_{this, "commands input", "list"};

        std::unique_ptr<b_outlet> nodes_out_;

        std::mutex blobs_mtx_;

        std::vector<std::unique_ptr<b_outlet>> outlets_;
        std::vector<std::tuple<long, bool, bool>> outlet_states;
    };
}

void ext_main(void* r) {
    c74::max::object_post(nullptr,
                          "radartouch external %s // (c) 2019 Jonas Ohland ",
                          O_THIS_TARGET_VERSION());
    c74::min::wrap_as_max_external<o::radartouch>("radartouch", __FILE__, r);
}
