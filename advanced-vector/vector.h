#pragma once
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <new>
#include <utility>

/*
// Шаблонный класс RawMemory будет отвечать за хранение буфера, который вмещает заданное количество элементов,
// и предоставлять доступ к элементам по индексу
*/
template <typename T>
class RawMemory {
public:
    RawMemory() = default;
    
    // Операция копирования не имеет смысла для класса RawMemory,
    // так как у него нет информации о количестве находящихся в сырой памяти элементов
    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory&) = delete;
    
    RawMemory(RawMemory&& other) noexcept {
        Swap(other);
    }
    
    RawMemory& operator=(RawMemory&& other) noexcept {
        Swap(other);
        return *this;
    }

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    using iterator = T*;
    using const_iterator = const T*;
    
    // Конструктор по умолчанию; создаёт пустой вектор
    Vector() = default;
    
    // Явный конструктор с заданием количества элементов
    explicit Vector(size_t size)
        : data_(size)
        , size_(size)
        {
            std::uninitialized_value_construct_n(data_.GetAddress(), size);
        }

    // Конструктор копирования
    Vector(const Vector& other)
        : data_(other.size_)
            , size_(other.size_)
        {
            std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
        }

    // Оператор присваивания копированием
    Vector& operator=(const Vector& other) {
        if (other.Size() > data_.Capacity()) {
            Vector tmp(other);
            Swap(tmp);
            return *this;
        } else {
            for (size_t i = 0; i < Size() && i < other.Size(); ++i) {
                data_[i] = other[i];
            }
            if (Size() < other.Size()) {
                std::uninitialized_copy_n(other.data_.GetAddress() + Size(),
                                          other.Size() - Size(),
                                          data_.GetAddress() + Size());
            } else if (Size() > other.Size()) {
                std::destroy_n(data_.GetAddress() + other.Size(),
                               Size() - other.Size());
            }
            size_ = other.Size();
        }
        return *this;
    }

    // Конструктор перемещения
    Vector(Vector&& other) noexcept {
        Swap(other);
    }

    // Оператор присваивания перемещением
    Vector& operator=(Vector&& other) noexcept {
        Swap(other);
        return *this;
    }
    
    // Деструктор вектора
    ~Vector() {
        std::destroy_n(data_.GetAddress(), Size());
    }
    
    // Вставляет элемент в позицию pos без создания промежуточного объекта
    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        size_t result = pos - this->begin();
        if (this->size_ == result) {
            this->EmplaceBack(std::forward<Args>(args)...);
            
        } else if (this->size_ < this->Capacity()) {
            T tmp = T(std::forward<Args>(args)...);
            new (this->data_ + this->size_) T(std::move(this->data_[this->size_ - 1u]));
            std::move_backward(this->data_.GetAddress() + result, this->end() - 1, this->end());
            this->data_[result] = std::move(tmp);
            this->size_++;
            
        } else {
            RawMemory<T> new_data(this->size_ * 2);
            new(new_data + result) T(std::forward<Args>(args)...);
            
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(this->data_.GetAddress(), result, new_data.GetAddress());
            } else {
                std::uninitialized_copy_n(this->data_.GetAddress(), result, new_data.GetAddress());
            }
            
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(this->data_.GetAddress() + result,
                                          this->size_ - result, new_data.GetAddress() + result + 1);
            } else {
                std::uninitialized_copy_n(this->data_.GetAddress() + result,
                                          this->size_ - result, new_data.GetAddress() + result + 1);
            }
            
            std::destroy_n(this->data_.GetAddress(), this->size_);
            this->data_.Swap(new_data);
            this->size_++;
        }
        return (this->data_.GetAddress() + result);
    }
    
    // Удаляет элемент, на который указывает переданный итератор
    iterator Erase(const_iterator pos) /*noexcept(std::is_nothrow_move_assignable_v<T>)*/ {
        iterator pos_it = const_cast<iterator>(pos);
        std::move(pos_it + 1, this->end(), pos_it);
        std::destroy_n(this->data_.GetAddress() + (--this->size_), 1);
        return pos_it;
    }
    
    /*
    // Так как метод Emplace способен передать свои аргументы любому конструктору T,
    // включая конструкторы копирования и перемещения, – методы Insert реализованы через Emplace
    */
    // Вставляет элемент в заданную позицию вектора;
    // возвращает итератор, указывающий на вставленный элемент в новом блоке памяти
    iterator Insert(const_iterator pos, const T& value) {
        return this->Emplace(pos, value);
    }
    
    // Версия для временных объектов
    iterator Insert(const_iterator pos, T&& value) {
        return this->Emplace(pos, std::forward<T>(value));
    }
    
    // Изменяет количество элементов в векторе
    void Resize(size_t new_size) {
        if (size_ < new_size) {
            // сначала нужно убедиться, что вектору достаточно памяти для новых элементов,
            // затем новые элементы нужно проинициализировать
            Reserve(new_size); 
            std::uninitialized_value_construct_n(data_.GetAddress() + Size(), new_size - Size());
            
        } else if (size_ > new_size) {
            std::destroy_n(data_.GetAddress() + new_size, Size() - new_size);
        }
        size_ = new_size;
    }
    
    // Вставляет элемент в конец вектора без создания промежуточного объекта
    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (this->size_ == this->Capacity()) {
            size_t capacity_tmp = 0;
            
            this->size_ == 0
                ? capacity_tmp += 1
                : capacity_tmp += this->size_ * 2;

            RawMemory<T> new_data(capacity_tmp);
            new (new_data + this->size_) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(this->data_.GetAddress(), this->size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(this->data_.GetAddress(), this->size_, new_data.GetAddress());
            }

            std::destroy_n(this->data_.GetAddress(), this->size_);
            this->data_.Swap(new_data);
        }
        else {
            new (this->data_ + this->size_) T(std::forward<Args>(args)...);
        }
        this->size_++;
        return this->data_[this->size_ - 1];
    }
    
    // Корректно вставляет в вектор объекты, находящиеся вне вектора и внутри него
    void PushBack(const T& value) {
        EmplaceBack(value);
    }
    
    // Версия метода для временных объектов
    void PushBack(T&& value) {
        EmplaceBack(std::move(value));
    }
    
    // Разрушает последний элемент вектора и уменьшает размер вектора
    void PopBack() {
        std::destroy_at(data_.GetAddress() + size_ - 1);
        --size_;
    }
    
    // Резервирует память под заданное количество элементов вектора
    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);

        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        
        std::destroy_n(data_.GetAddress(), Size());
        data_.Swap(new_data);
    }
    
    // Обмен содержимым с другим объектом Vector
    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }
    
    iterator begin() noexcept { return this->data_.GetAddress(); }
    iterator end() noexcept { return (this->data_.GetAddress() + this->size_); }
    
    const_iterator begin() const noexcept { return this->data_.GetAddress(); }
    const_iterator cbegin() const noexcept { return this->data_.GetAddress(); }
    
    const_iterator end() const noexcept { return (this->data_.GetAddress() + this->size_); }
    const_iterator cend() const noexcept { return (this->data_.GetAddress() + this->size_); }
    
    // Оператор произвольного доступа без проверки границ
    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    // Оператор произвольного доступа без проверки границ
    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }
    
    // Размер вектора
    size_t Size() const noexcept {
        return size_;
    }

    // Вместимость вектора
    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

private:
    /*
    // класс хранит указатель на область памяти
    // в которой непрерывным блоком размещаются элементы динамического массива,
    // и размер вектора. Вместимость вектора хранится в поле класса RawMemory
    */
    RawMemory<T> data_;
    size_t size_ = 0;
};