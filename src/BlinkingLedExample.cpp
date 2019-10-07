#include "PubSubExample.h"

#include <iostream>
#include <chrono>
#include <string>
#include <so_5/all.hpp>

class BlinkingLed final : public so_5::agent_t {
public:
    struct TurnOnOff: public so_5::signal_t {
    };
    
    BlinkingLed(context_t ctx):
        so_5::agent_t{std::move(ctx)},
        off(this),                                      // Отключено
        blinking(this),                                 // Включено
        blinkingLedOn(initial_substate_of(blinking)),   // Включено -> Лампочка включена
        blinkingLedOff(substate_of(blinking)){          // Включено -> Лампочка выключена
            
        // Переключаем состояние на "Выключено"
        so_change_state(off); //this >>= off;
        
        // Отключеное состояние может переходить во включеное по сообщению TurnOnOff
        off.just_switch_to<TurnOnOff>(blinking);
        
        // Моргающее состояние может переходить в выключеное по сообщению TurnOnOff
        blinking.just_switch_to<TurnOnOff>(off);
        
        // Это состояние будет переключаться по времени
        blinkingLedOn
            .on_enter([]{ std::cout << "OFF -> ON" << std::endl; })
            //.on_exit([]{ std::cout << "from ON to OFF" << std::endl; })
            .time_limit(std::chrono::milliseconds(1000), blinkingLedOff);
        
        // Это состояние будет переключаться по времени
        blinkingLedOff
            .on_enter([]{ std::cout << "ON -> OFF" << std::endl; })
            //.on_exit([]{ std::cout << "to off" << std::endl; })
            .time_limit(std::chrono::milliseconds(500), blinkingLedOn);
    }
    
private:
    // Список состояний
    so_5::state_t off;
    so_5::state_t blinking;
    so_5::state_t blinkingLedOn;
    so_5::state_t blinkingLedOff;

};

int blinkingLedExample() {
    so_5::launch([](so_5::environment_t & env) {
        so_5::mbox_t m;
        env.introduce_coop([&](so_5::coop_t & coop) {
            auto led = coop.make_agent<BlinkingLed>();
            m = led->so_direct_mbox();
        });
        
        const auto pause = [](std::chrono::seconds duration) {
            std::this_thread::sleep_for(duration);
        };
        
        std::cout << "Turn blinking on for 10s" << std::endl;
        so_5::send<BlinkingLed::TurnOnOff>(m);
        pause(std::chrono::seconds(10));
        
        std::cout << "Turn blinking off for 5s" << std::endl;
        so_5::send<BlinkingLed::TurnOnOff>(m);
        pause(std::chrono::seconds(5));
        
        std::cout << "Turn blinking on for 5s" << std::endl;
        so_5::send<BlinkingLed::TurnOnOff>(m);
        pause(std::chrono::seconds(5));
        
        std::cout << "Stopping..." << std::endl;
        env.stop();
    } );
    
    return 0;
}
