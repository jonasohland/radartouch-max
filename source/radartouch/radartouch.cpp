#include <c74_min.h>
#include <functional>
#include <boost/asio.hpp>
#include <oscpp/server.hpp>


namespace o {

    class radartouch : public c74::min::object<radartouch> {

      public:
        radartouch(const c74::min::atoms& args = {}) {
            
            if(args.size() <= 0)
                return;
            
            auto args_it = args.begin();
            
            try {
                
                port = c74::min::atom::get<long>(*args_it);
                
                if(++args_it != args.end())
                    max_blobs = c74::min::atom::get<long>(*args_it);
                
                cout << "Port: " << port << " Max Blobs: " << max_blobs << c74::min::endl;
                
            } catch (std::exception ex){
                cerr << ex.what() << c74::min::endl;
            }
            
            
        }
        
        short port;
        long max_blobs = -1;
        
    };

}
MIN_EXTERNAL(o::radartouch);
