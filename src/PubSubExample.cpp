#include "PubSubExample.h"

#include <iostream>
#include <chrono>
#include <string>
#include <so_5/all.hpp>

//using namespace std::literals;

struct AcquiredMessage {
    std::chrono::steady_clock::time_point acquiredAt;
    int value;
};

class Producer final : public so_5::agent_t {
public:
    Producer(context_t ctx, so_5::mbox_t mailBox):
        so_5::agent_t(std::move(ctx)),
        _mailBox(std::move(mailBox)),
        _counter(0){
    }
    
public:
    // Вызывается при инициализации агента
    virtual void so_define_agent() override {
        so_subscribe_self().event(&Producer::on_timer);
    }
    
    // Вызывается при старте актора на исполнение
    virtual void so_evt_start() override {
        // Данный актор будет периодически получать сообщение AcquisitionTime
        // с начальной задержкой 100ms и периодом в 750ms
        _timer = so_5::send_periodic<AcquisitionTime>(*this, std::chrono::milliseconds(500), std::chrono::milliseconds(500));
    }
    
    // Вызывается на завершение работы данного актора
    virtual void so_evt_finish() override {
        // Отключаем таймер
        _timer.release();
    }
    
private:
    const so_5::mbox_t _mailBox;
    so_5::timer_id_t _timer;
    int _counter;

private:
    struct AcquisitionTime final: public so_5::signal_t {
    };
    
    void on_timer(so_5::mhood_t<AcquisitionTime>) {
        // Отсылаем следующее значение счетчика
        so_5::send<AcquiredMessage>(_mailBox, std::chrono::steady_clock::now(), ++_counter);
    }
};

class Consumer final : public so_5::agent_t {
public:
    Consumer(context_t ctx, so_5::mbox_t mailBox, std::string name):
        so_5::agent_t(std::move(ctx)),
        _mailBox(std::move(mailBox)),
        _name(std::move(name)){
    }
    
public:
    void so_define_agent() override {
        so_subscribe(_mailBox).event(&Consumer::on_value);
    }
    
private:
    const so_5::mbox_t _mailBox;
    const std::string _name;

private:
    void on_value(so_5::mhood_t<AcquiredMessage> cmd) {
        std::cout << _name << ": " << cmd->value << std::endl;
    }
};

int pubSubExample() {
    // Создаем среду исполнения для акторов
    so_5::launch([](so_5::environment_t & env) {
        // Создаем общий канал
        so_5::mbox_t sharedMailBox = env.create_mbox();
        // Создаем кооперативную многозадачностью (группу акторов)
        env.introduce_coop([sharedMailBox](so_5::coop_t& coop) {
            // Создаем акторов, которые работают с общим каналом
            coop.make_agent<Producer>(sharedMailBox);
            
            coop.make_agent<Consumer>(sharedMailBox, "First");
            coop.make_agent<Consumer>(sharedMailBox, "Second");
        });
        
        std::this_thread::sleep_for(std::chrono::seconds(4));
        env.stop();
    });
    
    return 0;
}
