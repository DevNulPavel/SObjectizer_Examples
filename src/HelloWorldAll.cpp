#include "HelloWorldAll.h"
#include <iostream>
#include <so_5/all.hpp>

// Структурка сообщения всем
struct MessageHelloToAll{
    // Имя отправителя
    const std::string m_sender;
    // Почтовый ящик, канал передачи сообщения назад?
    const so_5::mbox_t m_mbox;
};

struct MessageHelloToYou{
    // Имя отправителя
    const std::string m_sender;
};

// Класс актора
class TestAgent: public so_5::agent_t {
public:
    TestAgent(context_t ctx, so_5::mbox_t commonMailBox, std::string agent_name):
        so_5::agent_t(ctx),
        _agentName(std::move(agent_name)),
        _commonMailBox(std::move(commonMailBox)){
    }
    
public:
    // Вызывается при установке объекта в среду исполнения
    virtual void so_define_agent() override{
        // Подписываемся на события через "общий" канал
        so_subscribe(_commonMailBox).event(&TestAgent::messageHelloToAll);
        
        // Подписываемся на события текущему актору?
        so_subscribe_self().event(&TestAgent::messageHelloToYou);
    }
    
    // Вызывается на старт работы в среде исполнения
    virtual void so_evt_start() override{
        std::cout << _agentName << ".so_evt_start" << std::endl;
        
        // Отправка приветствия всем агентам на канале, последний параметр - канал отправки текущему объекту
        so_5::send<MessageHelloToAll>(_commonMailBox, _agentName, so_direct_mbox());
    }
    
private:
    // Имя текущего объекта
    const std::string _agentName;
    // Очередь сообщений
    const so_5::mbox_t _commonMailBox;

private:
    // Вызывается на сообщение событие от всех
    void messageHelloToAll(const MessageHelloToAll& evt_data){
        std::cout << "Message to " << _agentName << " (all) from: " << evt_data.m_sender << std::endl;
        
        // Если текущий объект не отправитель?
        if(_agentName != evt_data.m_sender){
            // Таким образом отправка события конкретному актору через канал этого актора
            so_5::send<MessageHelloToYou>(evt_data.m_mbox, _agentName);
        }
    }
    
    // Вызывается на сообщение конкретно текущему объекту
    void messageHelloToYou(const MessageHelloToYou& evt_data) {
        std::cout << "Message to " << _agentName << " (to YOU) from: " << evt_data.m_sender << std::endl;
    }
};

///////////////////////////////////////////////////////////////////////////

int helloWorldAll(){
    try{
        // Запускаем работу с акторами
        so_5::launch([](so_5::environment_t& env){
            // Создание и регистрация кооперативного исполнения
            env.introduce_coop([](so_5::coop_t& coop) {
                // Выше создаем общий канал передачи
                so_5::mbox_t commonMailBox = coop.environment().create_mbox("common_mbox");
                // Созрание акторов
                coop.make_agent<TestAgent>(commonMailBox, "alpha");
                coop.make_agent<TestAgent>(commonMailBox, "beta");
                coop.make_agent<TestAgent>(commonMailBox, "gamma");
            });
            
            // Даем какое-то время на исполнение
            std::this_thread::sleep_for(std::chrono::milliseconds(10000));
            
            // Вырубаем среду исполнения
            env.stop();
        });
    }catch(const std::exception& ex){
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
