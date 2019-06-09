#pragma once

#include <vector>
#include <string>

namespace o {

    class radartouch_message {
        
    public:

        struct blob {

            blob(long bid)
                : bid(bid) {}

            long bid;
            std::array<float, 5> args;
            bool init = false;
            
            bool operator==(const blob& other) const {
                return other.bid == bid && other.args == args && other.init == init;
            }
            
            bool operator!=(const blob& other) const {
                return !(*this == other);
            }
            
            void set_args(std::array<float, 5>&& bargs){
                args = bargs;
                init = true;
            }
            
            bool initialized() { return init; }
        };

        
        radartouch_message() = default;

        radartouch_message(const radartouch_message& other) = default;

        radartouch_message(radartouch_message&& other)
            : blobs_(std::move(other.blobs_)), init_(other.init_) {
            other.init_ = false;
        }
        
        bool operator==(const radartouch_message& other) const {
            return other.blobs_ == blobs_ && other.init_ == init_;
        }
        
        bool operator!=(const radartouch_message& other) const {
            return !(*this == other);
        }

        void set_alive_ids(std::vector<long> ids) {

            for (long _id : ids)
                blobs_.emplace_back(_id);
            
            init_ = true;
            
        }

        void add_blob_args(long bid, std::array<float, 5>&& args) {

            if (!blobs_.size()) return;

            auto it = std::find_if(blobs_.begin(), blobs_.end(),
                                   [&](auto blb) { return blb.bid == bid; });

            if (it != blobs_.end())
                (*it).set_args(std::forward<std::array<float, 5>>(args));
        }
        
        void close(long fseq){
            fseq_ = fseq;
        }
        
        long fseq(){
            return fseq_;
        }

        void reset() {
            init_ = false;
            blobs_.clear();
        }

        std::vector<blob>& blobs() { return blobs_; }

        bool valid() {
            
            if(!init_) return false;
            
            auto last_ninit = std::find_if(blobs_.begin(), blobs_.end(),
                         [](auto blob) { return !blob.initialized(); });
            
            return last_ninit == blobs_.end();
            
        }

      private:
        std::vector<blob> blobs_;
        bool init_ = false;
        long fseq_;
    };
}
