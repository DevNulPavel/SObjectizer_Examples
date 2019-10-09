#include "PingPongExample.h"

#include <iostream>
#include <so_5/all.hpp>

struct Message {
    int sendCounter;
};
struct MessageSync {
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
        // Так как данный обработчик не меняет никакие данные, то его можно вызывать одновременно из разных потоков, значит он so_5::thread_safe
        so_subscribe_self().event(&Pinger::onMessage, so_5::thread_safe);
        //so_subscribe_self().event(&Pinger::onMessage, so_5::not_thread_safe);
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
            
            // Синхронный вызов у другого актора сообщения
            int64_t syncValue = so_5::request_value<int64_t, MessageSync>(_sendMailBox, so_5::infinite_wait, 0);
            std::cout << "Sync value received (" + _name + "): " << syncValue << std::endl;
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
        so_subscribe_self().event(&Ponger::onMessageSync);
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
    
    // Обработчик сообщения
    int64_t onMessageSync(so_5::mhood_t<MessageSync> cmd) {
        // Увеличиваем полученное число
        ++_pingsReceived;
        return 10;
    }
};

////////////////////////////////////////////////////////////////////////////////////

int pingPongExample() {
    // Создаем среду исполнения для акторов
    so_5::launch([](so_5::environment_t & env) {
        // one_thread - диспетчер, который выполняет каждого агента на одном и том же потоке
        // active_obj - диспетчер, который прикрепляет агентов на конкретный поток, агент становится активным объектом, он работает на своем потоке и не шарит поток с другими агентами
        // active_group - диспетчер, который прикрепляет группу агентов к конкретному потоку, только эти агенты шарят между собой этот поток
        // thread_pool - диспетчер, который использует пул рабочих потоков и пробрасывает между ними сообщения. This dispatcher doesn't distinguish between not_thread_safe and thread_safe event handlers and assumes that all event handlers are not_thread_safe. It means that agent work only on one working thread and no more than one event at a time;
        // adv_thread_pool - диспетчер, который использует пул потоков, and distinguish between not_thread_safe and thread_safe event handlers. It is called adv_thread_pool dispatcher. This dispatcher allows to run several of thread_safe event handlers of one agent on different threads in parallel.
        
        //        dispatcher prio_one_thread::strictly_ordered runs all events on the context of one working thread. This dispatcher allows for events of high priority agents to block events of low priority agents. It means that events queue is always strictly ordered: events for agents with high priority are placed before events for agents with lower priority;
        //        dispatcher prio_one_thread::quoted_round_robin also runs all events on the context of one working thread. Dispatcher prio_one_thread::quoted_round_robin works on round-robin principle. It allows to specify maximum count of events to be processed consequently for the specified priority. After processing that count of events dispatcher switches to processing events of lower priority even if there are yet more events of higher priority to be processed;
        //        dispatcher prio_dedicated_threads::one_per_prio creates dedicated thread for every priority (so the eight working threads will be created). It means that events for agents with priority p7 will be handled on different thread than events for agents with, for example, priority p6.
        
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
            // Специально после, когда сформированы объекты
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
    },
    [](so_5::environment_params_t& params) {
        // Разрешаем трассировку механизма доставки сообщений.
        // Направляем трассировочные сообщения на стандартый поток вывода.
        params.message_delivery_tracer(so_5::msg_tracing::std_cout_tracer());
    });
    
    return 0;
}
