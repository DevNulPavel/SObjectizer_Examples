#include "HelloWorldAll.h"
#include <iostream>
#include <so_5/all.hpp>

// Структурка сообщения всем
struct MessageHelloToAll{
    // Имя отправителя
    const std::string m_sender;
    // Почтовый ящик, канал передачи данных?
    const so_5::mbox_t m_mbox;
};

struct MessageHelloToYou{
    // Имя отправителя
    const std::string m_sender;
};

// Класс агента
class TestAgent: public so_5::agent_t {
public:
    TestAgent(context_t ctx, std::string agent_name);
    
    // Вызывается при установке объекта в среду исполнения
    virtual void so_define_agent() override;
    
    // Вызывается на старт работы в среде исполнения
    virtual void so_evt_start() override;
    
    // Обработчики событий
    void evt_hello_to_all(const MessageHelloToAll& evt_data);
    void evt_hello_to_you(const MessageHelloToYou& evt_data);
    
private:
    // Имя текущего объекта
    const std::string _agentName;
    
    // Очередь сообщений
    const so_5::mbox_t _commonMailBox;
};

TestAgent::TestAgent(context_t ctx, std::string agent_name):
    so_5::agent_t(ctx),
    _agentName(std::move(agent_name)),
    _commonMailBox(so_environment().create_mbox("common_mbox")){
    // Выше создаем канал передачи, у всех агентов одинаковое имя - следовательно канал как бы общий
}

// Вызывается при установке объекта в среду исполнения
void TestAgent::so_define_agent() {
    // Подписываемся на события через "общий" канал
    so_subscribe(_commonMailBox).event(&TestAgent::evt_hello_to_all);
    
    // Подписываемся на события текущему актору?
    so_subscribe_self().event(&TestAgent::evt_hello_to_you);
}

// Вызывается на старт работы в среде исполнения
void TestAgent::so_evt_start() {
    std::cout << _agentName << ".so_evt_start" << std::endl;
    
    // Отправка приветствия всем агентам на канале, последний параметр - канал отправки текущему объекту
    so_5::send<MessageHelloToAll>(_commonMailBox, _agentName, so_direct_mbox());
}

// Вызывается на сообщение событие от всех
void TestAgent::evt_hello_to_all(const MessageHelloToAll & evt_data) {
    std::cout << "Message to " << _agentName << " (all) from: " << evt_data.m_sender << std::endl;
    
    // Если текущий объект не отправитель?
    if(_agentName != evt_data.m_sender){
        // Таким образом отправка события конкретному актору через канал этого актора
        so_5::send<MessageHelloToYou>(evt_data.m_mbox, _agentName);
    }
}

// Вызывается на сообщение конкретно текущему объекту
void TestAgent::evt_hello_to_you(const MessageHelloToYou& evt_data) {
    std::cout << "Message to " << _agentName << " (to YOU) from: " << evt_data.m_sender << std::endl;
}

///////////////////////////////////////////////////////////////////////////

// Функция инициализации среды исполнения SObjectizer
void init( so_5::environment_t & env ) {
    // Создание и регистрация кооперативного исполнения
    env.introduce_coop([](so_5::coop_t& coop) {
        // Созрание агентов
        coop.make_agent<TestAgent>("alpha");
        coop.make_agent<TestAgent>("beta");
        coop.make_agent<TestAgent>("gamma");
    } );
    
    // Даем какое-то время на исполнение
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    
    // Вырубаем среду исполнения
    env.stop();
}

int helloWorldAll(){
    try{
        so_5::launch( &init );
    }catch(const std::exception& ex){
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
