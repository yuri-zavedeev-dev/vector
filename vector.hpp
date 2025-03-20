#pragma once

#include "raw_memory.hpp"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <utility>

template <typename T>
class Vector {
public:
    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size) 
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    ~Vector() {
        DestroyN(data_.GetAddress(), size_);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)  
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
    }

    Vector(Vector&& other) noexcept
        : data_{ std::move(other.data_) }
        , size_{ std::exchange(other.size_, 0) }
    {}

    Vector& operator=(Vector&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        this->Swap(other);
        return *this;
    }

    Vector& operator=(const Vector& other) {
        if (this != &other) {
            if (other.size_ > data_.Capacity()) {
                Vector other_copy(other);
                Swap(other_copy);
            }
            else {
                CopyOtherData(other);
            }
        }
        return *this;
    }

    void Swap(Vector& other) noexcept {
        std::swap(this->size_, other.size_);
        this->data_.Swap(other.data_);
    }

    void Resize(size_t new_size) {
        if (new_size < size_) {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
            size_ = new_size;
        }
        else {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
            size_ = new_size;
        }
    }

    void PushBack(const T& value) {
        EmplaceBack(value);
    }

    void PushBack(T&& value) {
        EmplaceBack(std::move(value));
    }

    void PopBack()  noexcept {
        Destroy(data_.GetAddress() + size_ - 1);
        --size_;
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (size_ == Capacity()) {
            RawMemory<T> new_data{ size_ == 0 ? 1 : size_ * 2 };
            new (new_data + size_) T(std::forward<Args>(args)...);
            UninitializedMoveOrCopyN(data_.GetAddress(), size_, new_data.GetAddress());
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        }
        else {
            new (data_ + size_) T(std::forward<Args>(args)...);
        }

        ++size_;
        return data_[size_ - 1];
    }

    using iterator = T*;
    using const_iterator = const T*;

    iterator begin() noexcept {
        return data_ + 0;
    }

    iterator end() noexcept {
        return data_ + size_;
    }

    const_iterator begin() const noexcept {
        return data_ + 0;
    }

    const_iterator end() const noexcept {
        return data_ + size_;
    }

    const_iterator cbegin() const noexcept {
        return data_ + 0;
    }

    const_iterator cend() const noexcept {
        return data_ + size_;
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        assert(pos >= begin());
        assert(pos <= end());
        iterator entry_pos{ const_cast<iterator>(pos) };

        if (!(size_ == Capacity()))
        {
            if (end() != pos){
                T buffer{ std::forward<Args>(args)... };
                UninitializedMoveOrCopyN(end() - 1, 1, data_ + size_);

                std::move_backward(entry_pos, (end() - 1), end());
                *entry_pos = std::move(buffer);
            }
            else {
                new (end()) T(std::forward<Args>(args)...);
            }
            

            size_++;
            return entry_pos;
        }
        else {
            RawMemory<T> buffer{ size_ == 0 ? 1 : size_ * 2 };
            size_t shift = std::distance(static_cast<const_iterator>(begin()), pos);
            auto new_item_ptr = new (buffer + shift) T(std::forward<Args>(args)...);

            try {
                UninitializedMoveOrCopy(begin(), entry_pos, buffer.GetAddress());
            }
            catch (...) {
                Destroy(new_item_ptr);
                throw;
            }

            try
            {
                UninitializedMoveOrCopy(entry_pos, end(), new_item_ptr + 1);
            }
            catch (...)
            {
                DestroyN(buffer + 0, std::distance(buffer + 0, new_item_ptr));
                throw;
            }

            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(buffer);
            size_++;

            return static_cast<iterator>(new_item_ptr);
        }
    }

    iterator Erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>) {
        std::move(const_cast<iterator>(pos + 1), end(), const_cast<iterator>(pos));
        Destroy(end() - 1);
        size_--;
        return const_cast<iterator>(pos);
    }

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }


    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }

        RawMemory<T> new_data(new_capacity);
        UninitializedMoveOrCopyN(data_.GetAddress(), size_, new_data.GetAddress());

        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

private:
    void UninitializedMoveOrCopyN(iterator first, size_t count, iterator d_first) {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(first, count, d_first);
        }
        else {
            std::uninitialized_copy_n(first, count, d_first);
        }
    }

    void UninitializedMoveOrCopy(iterator first, iterator second, iterator d_first) {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move(first, second, d_first);
        }
        else {
            std::uninitialized_copy(first, second, d_first);
        }
    }

    void CopyOtherData(const Vector& other)
    {
        std::copy_n(other.data_.GetAddress(), std::min(other.size_, this->size_), this->data_.GetAddress());

        if (other.size_ < this->size_) {
            std::destroy_n(data_.GetAddress() + other.size_, this->size_ - other.size_);
        }
        else {
            std::uninitialized_copy_n(
                data_.GetAddress() + this->size_
                , other.size_ - this->size_
                , this->data_.GetAddress() + this->size_
            );
        }

        this->size_ = other.size_;
    }

    static void DestroyN(T* buf, size_t n) noexcept {
        for (size_t i = 0; i != n; ++i) {
            Destroy(buf + i);
        }
    }

    static void CopyConstruct(T* buf, const T& elem) {
        new (buf) T(elem);
    }

    static void Destroy(T* buf) noexcept {
        buf->~T();
    }

    RawMemory<T> data_;
    size_t size_ = 0;
};
