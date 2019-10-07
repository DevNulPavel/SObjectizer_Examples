#include "HelloWorldSimple.h"

#include <iostream>
#include <so_5/all.hpp>

// Объявляем актора для работы
class HelloAgent: public so_5::agent_t {
public:
    HelloAgent(context_t ctx)
        :so_5::agent_t(ctx){
    }
    
    // Вызывается при старте работы в SObjectizer
    virtual void so_evt_start() override{
        std::cout << "Hello, world! This is SObjectizer v.5 (" << SO_5_VERSION << ")" << std::endl;
        // Вырубаем SObjectizer
        so_environment().stop();
    }
    
    // Вызывается на завершение работы в SObjectizer
    virtual void so_evt_finish() override {
        std::cout << "Bye! This was SObjectizer v.5." << std::endl;
    }
};

int helloWorldSimple(){
    try{
        // Лямбда для SO Environment инициализации
        so_5::launch([]( so_5::environment_t & env ) {
             // Создаем и регистрируем только одного актора для работы
            std::unique_ptr<HelloAgent> agent = env.make_agent<HelloAgent>();
            // Регистрируем нашего актора
            env.register_agent_as_coop("coop", std::move(agent));
        });
    } catch(const std::exception & ex){
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    
    return 0;
}
