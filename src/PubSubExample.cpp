#include "PubSubExample.h"

#include <iostream>
#include <chrono>
#include <string>
#include <so_5/all.hpp>

//using namespace std::literals;

struct acquired_value {
    std::chrono::steady_clock::time_point acquired_at_;
    int value_;
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
        so_5::send<acquired_value>(_mailBox, std::chrono::steady_clock::now(), ++_counter);
    }
};

class consumer final : public so_5::agent_t {
    const so_5::mbox_t board_;
    const std::string name_;
    
    void on_value(mhood_t<acquired_value> cmd) {
        std::cout << name_ << ": " << cmd->value_ << std::endl;
    }
    
public:
    consumer(context_t ctx, so_5::mbox_t board, std::string name)
    :  so_5::agent_t{std::move(ctx)}
    ,  board_{std::move(board)}
    ,  name_{std::move(name)}
    {}
    
    void so_define_agent() override {
        so_subscribe(board_).event(&consumer::on_value);
    }
};

int pubSubExample() {
    // Создаем среду исполнения для акторов
    so_5::launch([](so_5::environment_t & env) {
        // Создаем общий канал
        so_5::mbox_t board = env.create_mbox();
        // Создаем кооперативную многозадачностью (группу акторов)
        env.introduce_coop([board](so_5::coop_t& coop) {
            // Создаем акторов, которые работают с общим каналом
            coop.make_agent<Producer>(board);
            
            coop.make_agent<consumer>(board, "first");
            coop.make_agent<consumer>(board, "second");
        });
        
        std::this_thread::sleep_for(std::chrono::seconds(4));
        env.stop();
    });
    
    return 0;
}
