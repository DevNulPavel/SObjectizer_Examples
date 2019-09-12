#include "HelloWorldSimple.h"

#include <iostream>
#include <so_5/all.hpp>

// Объявляем обработчика для работы
class HelloAgent : public so_5::agent_t {
public:
    HelloAgent(context_t ctx)
        :so_5::agent_t(ctx){
    }
    
    // Вызывается при старте работы в SObjectizer.
    virtual void so_evt_start() override{
        std::cout << "Hello, world! This is SObjectizer v.5 (" << SO_5_VERSION << ")" << std::endl;
        // Вырубаем SObjectizer
        so_environment().stop();
    }
    
    // Вызывается на завершение работы в SObjectizer.
    virtual void so_evt_finish() override {
        std::cout << "Bye! This was SObjectizer v.5." << std::endl;
    }
};

int helloWorldSimple(){
    try{
        // Запуск SObjectizer.
        so_5::launch(
                     // Лямбда дляr SO Environment инициализации.
                     []( so_5::environment_t & env ) {
                         // Создаем и регистрируем только одного агента для работы
                         env.register_agent_as_coop("coop", env.make_agent<HelloAgent>());
                     });
    } catch(const std::exception & ex){
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    
    return 0;
}
