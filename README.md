# Продвинутый вектор

## Проект в рамках обучения на курсе Яндекс Практикум

Advanced Vector – это доведённая до совершенства реализация собственного аналога последовательного контейнера стандартной библиотеки STL `std::vector`. Работа с «сырой» памятью возложена на класс `RawMemory`.

## Реализованный функционал

### Создание объекта

* Конструктор по умолчанию. Создаёт пустой вектор с нулевой вместимостью. Не выделяет динамическую память и не выбрасывает исключений.
* Параметризованный конструктор, создающий вектор заданного размера. Элементы вектора инициализированы значением по умолчанию для шаблонного типа. Вектор имеет одинаковые размер и вместимость. Если размер нулевой, динамическая память для его элементов не выделяется.
* Конструктор копирования.
* Конструктор перемещения.

### Доступ к элементам

Методы `begin()` и `end()` и их константные версии `cbegin()` и `cend()` возвращают итераторы на первый элемент вектора и на позицию за последним элементом вектора соответственно.
Для доступа к произвольному элементу определен `operator[]`. Имеет две версии — константную и неконстантную. Не выбрасывает исключений. Для корректной работы оператора индекс элемента вектора не должен выходить за пределы массива.

### Методы

* `Emplace` - добавление элемента в указанную позицию без создания промежуточного объекта;
* `PushBack` - добавление элемента в конец вектора путём копирования либо перемещения;
* `Insert` - вставка элемента в указанную позицию путём копирования либо перемещения;
* `PopBack` - удаление последнего элемента вектора;
* `Erase` - удаление элемента в указанной позиции;
* `Swap` - обмен содержимого двух векторов;
* `Reserve` - резервирует память под заданное количество элементов вектора;
* `Resize` - изменяет размер вектора. При уменьшении размера лишние элементы удалятся. При увеличении размера автоматически резервируется место в памяти.

## Состав проекта и его запуск

* `vector.h` – файл шаблонного класса `Vector`;
* `main.cpp` – юнит-тесты корректности работы класса.

Для использования в своём проекте `AdvancedVector` поместите файл `vector.h` из репозитория в папку вашего проекта. Затем подключите заголовочный файл через директиву: `#include "vector.h"`.