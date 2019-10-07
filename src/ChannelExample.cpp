#include "PubSubExample.h"

#include <iostream>
#include <chrono>
#include <string>
#include <so_5/all.hpp>

#include <so_5/all.hpp>

struct Ping {
    int counter;
};

struct Pong {
    int counter;
};

void PingerThread(so_5::mchain_t self_ch, so_5::mchain_t ping_ch) {
    so_5::send<Ping>(ping_ch, 1000);
    
    // Read all message until channel will be closed.
    so_5::receive( so_5::from(self_ch),
                  [&](so_5::mhood_t<Pong> cmd) {
                      if(cmd->counter > 0)
                          so_5::send<Ping>(ping_ch, cmd->counter - 1);
                      else {
                          // Channels have to be closed to break `receive` calls.
                          so_5::close_drop_content(self_ch);
                          so_5::close_drop_content(ping_ch);
                      }
                  });
}

void PongerThread(so_5::mchain_t self_ch, so_5::mchain_t pong_ch) {
    int pings_received{};
    
    // Read all message until channel will be closed.
    so_5::receive( so_5::from(self_ch),
                  [&](so_5::mhood_t<Ping> cmd) {
                      ++pings_received;
                      so_5::send<Pong>(pong_ch, cmd->counter);
                  });
    
    std::cout << "pings received: " << pings_received << std::endl;
}

int channelExample() {
    so_5::wrapped_env_t sobj;
    
    auto pingerChannel = so_5::create_mchain(sobj);
    auto pongerChannel = so_5::create_mchain(sobj);
    
    std::thread pinger(PingerThread, pingerChannel, pongerChannel);
    std::thread ponger(PongerThread, pongerChannel, pingerChannel);
    
    ponger.join();
    pinger.join();
    
    return 0;
}
