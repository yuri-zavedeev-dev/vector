# vector

## Описание

Этот учебный проект - аналог `std::vector`, повторяющий часть его функционала и свойств. Проект выполнен в рамках обучения на [курсе повышения квалификации от Яндекс Практикума](https://practicum.yandex.ru/cpp/?from=catalog). Он реализует хранение элементов в динамической памяти, методы `PushBack`, `EmplaceBack`, `Erase` и `Insert`. Методы `EmplaceBack` и `Emplace` реализованы с применением технологий "вариативный шаблон" и "универсальная ссылка". Также для него реализованы конструкторы перемещения и копирования, операторы присваивания перемещения и копии. Для работы с неинициализированной памятью используются алгоритмы `std::uninitialized_copy_n`, `std::uninitialized_move_n` и им подобные. Код вектора находится в файле `vector.hpp`. В файле `raw_memory.hpp` реализован небольшой класс, управляющий выделением и освобождением памяти в куче при помощи операторов `void* operator new(std::size_t size)` и `void operator delete(void* ptr) noexcept`. Файл `main.cpp` содержит тесты, предоставленные Яндекс Практикумом.

## Изученные технологии

* Динамическое выделение и освобождение памяти с помощью `operator new` и `operator delete`
* Работа с неинициализированной памятью с помощью семейства алгоритмов `std::uninitialized_copy_n`, `std::uninitialized_move_n`
* Применение вариативного шаблона в сочетании с универсальной ссылкой
