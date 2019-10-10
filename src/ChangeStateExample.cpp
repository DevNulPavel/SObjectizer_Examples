#include "ChangeStateExample.h"

#include <iostream>
#include <chrono>
#include <string>
#include <so_5/all.hpp>


// Periodic message.
class TestMessage: public so_5::signal_t {};

// Специальный листенер, который получает события из монитора
class StateMonitor: public so_5::agent_state_listener_t{
public:
    StateMonitor(const std::string& typeHint):
        _typeHint(typeHint){
    }
    
    // Вызывается на изменение состояния у актора
    virtual void changed(so_5::agent_t&, const so_5::state_t& state) override{
        std::cout << _typeHint << " agent changed state to " << state.query_name() << std::endl;
    }
    
private:
    const std::string _typeHint;
};

// Простой актор у которого мы изменяем состояние
class StateSwitcherActor: public so_5::agent_t{
public:
    StateSwitcherActor(context_t ctx):
        so_5::agent_t(ctx){
    }
    
    virtual ~StateSwitcherActor(){
    }
    
public:
    // Вызывается при объявлении актора, но до старта
    virtual void so_define_agent() override{
        // Подписываем дефолтное состояние на события
        so_subscribe_self().event<TestMessage>(&StateSwitcherActor::messageOnDefaultState);
        
        // Подписываемся на события в указанных состояниях, они будут вызываться только если это состояние активно
        so_subscribe_self().in(st_1).event<TestMessage>(&StateSwitcherActor::messageOnState1);
        so_subscribe_self().in(st_2).event<TestMessage>(&StateSwitcherActor::messageOnState2);
        so_subscribe_self().in(st_3).event<TestMessage>(&StateSwitcherActor::messageOnState3);
        so_subscribe_self().in(st_shutdown).event<TestMessage>(&StateSwitcherActor::messageOnStateShutdown);
    }
    
    // Вызывается при старте работы данного актора
    virtual void so_evt_start() override{
        show_event_invocation("so_evt_start()");
        
        // Инициируем периодические сообщения с начальной задержкой в 1 секунду + периодом в 1 секунду
        _timer = so_5::send_periodic<TestMessage>(*this, std::chrono::seconds(1), std::chrono::seconds(1));
    }
    
    // Вызывается на завершение работы данного актора
    virtual void so_evt_finish() override {
        if (_timer.is_active()) {
            _timer.release();
        }
    }
    
private:
    // Состояния агента, инициализируются даными значениями при вызове конструктора
    const state_t st_1{ this, "state_1" };
    const state_t st_2{ this, "state_2" };
    const state_t st_3{ this, "state_3" };
    const state_t st_shutdown{ this, "shutdown" };
    
    // Объект таймера, если не сохранить его, то таймер уничтожится автоматически после создания
    so_5::timer_id_t _timer;
    
private:
    // Обработчик сообщений в дефолтном состоянии
    void messageOnDefaultState(){
        show_event_invocation("evt_handler_default");
        
        // Переключаемся на другое состояние
        so_change_state( st_1 );
    }
    
    // Обработчик сообщения в состоянии 1
    void messageOnState1(){
        show_event_invocation( "evt_handler_1" );
        
        // Переключаемся на другое состояние
        so_change_state( st_2 );
    }
    
    // Message handler for the state_2.
    void messageOnState2(){
        show_event_invocation( "evt_handler_2" );
        
        // Переключаемся на другое состояние
        so_change_state( st_3 );
    }
    
    // Message handler for the state_3.
    void messageOnState3(){
        show_event_invocation( "evt_handler_3" );
        
        // Переключаемся на другое состояние
        so_change_state( st_shutdown );
    }
    
    // Message handler for the shutdown_state.
    void messageOnStateShutdown(){
        show_event_invocation( "evt_handler_3" );
        
        // Переключаемся на другое состояние
        so_change_state( so_default_state() );
        
        // Finishing SObjectizer's work.
        std::cout << "Stop sobjectizer..." << std::endl;
        so_environment().stop();
    }
    
    // Вспомогательный метод, который выводит имя обработчика
    void show_event_invocation(const char * event_name){
        //time_t t = time(0);
        //std::cout << asctime(localtime(&t)) << event_name << ", state: " << so_current_state().query_name() << std::endl;
    }
};



// A state listener.
StateMonitor g_state_monitor("nondestroyable_listener");

// The SObjectizer Environment initialization.
void init(so_5::environment_t & env){
    auto ag = env.make_agent<StateSwitcherActor>();
    
    // Adding the state listener. Its lifetime is not controlled by the agent.
    ag->so_add_nondestroyable_listener( g_state_monitor );
    
    // Создаем лиснер, его время жизни контролируется актором
    so_5::agent_state_listener_unique_ptr_t listenerPtr = so_5::agent_state_listener_unique_ptr_t(new StateMonitor("destroyable_listener"));
    ag->so_add_destroyable_listener(std::move(listenerPtr));
    
    // Creating and registering a cooperation.
    env.register_agent_as_coop( "coop", std::move(ag) );
}

int changeStateExample() {
    try{
        so_5::launch(&init);
    }catch( const std::exception & ex ){
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
