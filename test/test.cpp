#include "log.h"

int main() {
    Log::instance()->init(0);
    for(int i=0;i<100;++i) {
        //std::cout<<i<<std::endl;
        LOG_DEBUG("LOG TEST");
        //std::cout<<-i<<std::endl;
    }
}
