#include "PingPongExample.h"

#include <iostream>
#include <so_5/all.hpp>

struct Message {
    int sendCounter;
};

// Актор отправляющий сообщения
class Pinger final : public so_5::agent_t {
public:
    Pinger(context_t ctx, const std::string& name):
        so_5::agent_t(std::move(ctx)),
        _name(name){
    }
    
    // Устанавливаем канал для передачи сообщений
    void setSendMailBox(const so_5::mbox_t mbox) {
        _sendMailBox = mbox;
    }
    
public:
    // Перегруженные методы для работы с системой агентов
    
    // Вызывается при инициализации актора
    virtual void so_define_agent() override {
        so_subscribe_self().event(&Pinger::onMessage);
    }
    
    // Вызывается при старте работы данного актора
    virtual void so_evt_start() override {
        // Отправляем сообщение в ящик со значением 10000
        so_5::send<Message>(_sendMailBox, Message{10000});
    }
    
    // Вызывается на завершение работы данного актора
    virtual void so_evt_finish() override {
    }
    
private:
    so_5::mbox_t _sendMailBox;
    std::string _name;
    
private:
    // Обработчик сообщения
    void onMessage(so_5::mhood_t<Message> cmd) {
        // Пока счетчик не закончился, отправляем ответное сообщение через канал
        if(cmd->sendCounter > 0){
            so_5::send<Message>(_sendMailBox, Message{cmd->sendCounter - 1});
        }else{
            // Если счетчик уменьшился, то отключаем данного актора из системы
            so_deregister_agent_coop_normally();
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////////

class Ponger final: public so_5::agent_t {
public:
    Ponger(context_t ctx, const std::string& name):
        so_5::agent_t(std::move(ctx)),
        _pingsReceived(0),
        _name(name){
    }
    
    // Устанавливаем канал для передачи сообщений
    void setSendMailBox(const so_5::mbox_t mbox) {
        _sendMailBox = mbox;
    }
    
public:
    // Перегруженные методы для работы с системой агентов
    
    // Вызывается при инициализации актора
    void so_define_agent() override {
        so_subscribe_self().event(&Ponger::onMessage);
    }
    
    // Вызывается при старте работы данного актора
    virtual void so_evt_start() override {
        // Отправляем сообщение в ящик со значением 1000
        //so_5::send<Message>(_sendMailBox, Message{1000});
    }
    
    // Вызывается на завершение работы данного актора
    virtual void so_evt_finish() override {
        std::cout << "Pings received (" + _name + "): " << _pingsReceived << std::endl;
    }
    
private:
    so_5::mbox_t _sendMailBox;
    int _pingsReceived;
    std::string _name;

private:
    // Обработчик сообщения
    void onMessage(so_5::mhood_t<Message> cmd) {
        // Увеличиваем полученное число
        ++_pingsReceived;
        // Отправляем полученное значение назад
        so_5::send<Message>(_sendMailBox, cmd->sendCounter);
        
        // Если счетчик уменьшился, то отключаем данного актора из системы
        if (cmd->sendCounter == 0) {
            so_deregister_agent_coop_normally();
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////

int pingPongExample() {
    // Создаем среду исполнения для акторов
    so_5::launch([](so_5::environment_t & env) {
        // Создаем диспетчер на текущей среде исполнения и привязываем его к ней
        so_5::disp_binder_unique_ptr_t dispatcher = so_5::disp::active_obj::create_private_disp(env)->binder();
        
        // Данный диспетчер заставляет выполняться акторов в одном и том же потоке
        //so_5::disp_binder_unique_ptr_t dispatcher = so_5::disp::one_thread::create_private_disp(env)->binder();
        
        // TODO: ???
        //so_5::disp_binder_unique_ptr_t dispatcher = so_5::disp::active_group::create_private_disp(env)->binder("test_group");
        
        // Акторы из группы будут выполняться на пуле потоков
        /*so_5::disp::thread_pool::bind_params_t params;
        //params.fifo(so_5::disp::thread_pool::fifo_t::individual); // Каждый актор будет выполняться в отдельном потоке из пула потоков
        params.fifo(so_5::disp::thread_pool::fifo_t::cooperation); // Каждый актор из группы будет работать на пуле потоков, но работа будет кооперативная, сначала один - потом второй
        so_5::disp_binder_unique_ptr_t dispatcher = so_5::disp::thread_pool::create_private_disp(env)->binder(params);*/
        
        
        // Создаем группу акторов, которые живут в конкретном диспетчере
        env.introduce_coop(std::move(dispatcher), [](so_5::coop_t & coop) {
            // Создаем акторов
            auto pingerActor = coop.make_agent<Pinger>("Coop");
            auto pongerActor = coop.make_agent<Ponger>("Coop");
            
            // Назначаем каждому из них канал другого, чтобы они могли друг с другом обмениваться сообщениями
            pongerActor->setSendMailBox(pingerActor->so_direct_mbox());
            pingerActor->setSendMailBox(pongerActor->so_direct_mbox());
        });
        
        /*{
            // Создаем акторов
            auto pingerActor = env.make_agent<Pinger>("Individual");
            auto pongerActor = env.make_agent<Ponger>("Individual");
            
            // Назначаем каждому из них канал другого, чтобы они могли друг с другом обмениваться сообщениями
            pongerActor->setSendMailBox(pingerActor->so_direct_mbox());
            pingerActor->setSendMailBox(pongerActor->so_direct_mbox());
            
            // Теперь каждого из акторов запускаем отдельно, а не в составе группы
            env.register_agent_as_coop("Coop test 1", std::move(pingerActor));
            env.register_agent_as_coop("Coop test 2", std::move(pongerActor));
        }*/
    });
    
    return 0;
}
