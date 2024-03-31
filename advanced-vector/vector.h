#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <iterator>
#include <algorithm>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;


    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept : buffer_(std::move(other.buffer_)) {
        // buffer_ = std::move(other.buffer_);
        capacity_ = std::move(other.Capacity());
        other.buffer_ = nullptr;
    }

    RawMemory& operator=(RawMemory&& rhs) noexcept {

        Deallocate(buffer_);
        buffer_ = rhs.buffer_;
        capacity_ = rhs.capacity_;
        rhs.buffer_ = nullptr;
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

    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)  //
    {
        
        std::uninitialized_value_construct_n(data_.GetAddress(), size_);
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)  //
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }

    Vector& operator=(Vector&& rhs) noexcept {
        if (this != &rhs) {
            
            // Swap(rhs);
            data_ = std::move(rhs.data_);
            size_ = std::move(rhs.size_);
            rhs.size_ = 0;
        }
        return *this;
    }

    Vector(Vector&& rhs) noexcept : data_(std::move(rhs.data_)) {
        size_ = std::move(rhs.size_);
        rhs.size_ = 0;
    }

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {

                auto copy = Vector(rhs);
                Swap(copy);
                /* Применить copy-and-swap */
            } else {
                /* Скопировать элементы из rhs, создав при необходимости новые
                   или удалив существующие */
                size_t copy_count = std::min(size_, rhs.size_);
                // std::copy_n(rhs.data_.GetAddress(), copy_count, data_.GetAddress());

                std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + copy_count, data_.GetAddress());
                if( rhs.Size() < size_ ) {
                    std::destroy_n(data_.GetAddress() + rhs.Size(), size_ - rhs.Size() );   
                } else {
                    std::uninitialized_copy_n(data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress());
                }
                size_ = rhs.Size();
            }// size_t copy_count = std::min(size_, rhs.size_);
                // std::copy_n(rhs.data_.GetAddress(), copy_count, data_.GetAddress());
        }
        return *this;
    }


    iterator begin() noexcept {
        return data_.GetAddress();
    }

    iterator end() noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator end() const noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator cbegin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator cend() const noexcept {
        return data_.GetAddress() + size_;
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }

        RawMemory new_data = RawMemory<T>(new_capacity);
        MoveOrCopy(data_.GetAddress(), size_, new_data.GetAddress());

        std::destroy_n(data_.GetAddress(), size_);
        
        new_data.Swap(data_);
    }

    void Resize(size_t new_size) {
        if( new_size < size_ ) {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
        } else {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
        }

        size_ = new_size;
    }

    void PushBack(const T& value) {
        EmplaceBack(value);
    }

    void PushBack(T&& value) {
        EmplaceBack(std::move(value));
    }

    void PopBack() {
        if( size_ != 0 ) {
            --size_;
            std::destroy_n(data_.GetAddress() + size_, 1);
            
        }
    }

template <typename... Args>
    T& EmplaceBack(Args&&... args) {

        if( size_ == data_.Capacity() ) {
            RawMemory new_data = RawMemory<T>(size_ == 0 ? 1 : size_ * 2);
            // std::uninitialized_move_n(data_.GetAddress() + size_, size_, new_data.GetAddress());
            new (new_data + size_) T(std::forward<Args>(args)...);

            MoveOrCopy(data_.GetAddress(), size_, new_data.GetAddress());

            std::destroy_n(data_.GetAddress(), size_);
        
            new_data.Swap(data_);

        } else {
            new (data_ + size_) T(std::forward<Args>(args)...);
        }

        ++size_;
        return (*this)[size_ - 1];
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        // const auto n = pos - 
        iterator result = nullptr;
        const int dist = std::distance(cbegin(), pos);
        if( size_ == data_.Capacity() ) {
            RawMemory new_data = RawMemory<T>(size_ == 0 ? 1 : size_ * 2);

            try {
                result = new (new_data + dist) T(std::forward<Args>(args)...);
            } catch(...) {
                std::destroy_n(new_data + dist, 1);
            }
            
            int count = std::distance(cbegin(), pos);
            MoveOrCopy(data_.GetAddress(), count, new_data.GetAddress());

            int count_after = std::distance(pos, cend());
            MoveOrCopy(begin() + count, count_after, new_data.GetAddress() + count + 1);
            
            std::destroy_n(data_.GetAddress(), size_);
        
            new_data.Swap(data_);

        } else {
            if( begin() != end() ) {
                T temp = T(std::forward<Args>(args)...);

                try { 
                    new ( data_+ size_) T(std::move((*this)[size_ - 1]));
                } catch(...) {
                    std::destroy_n(end(), 1);
                } 
                std::move_backward(data_ + dist, end() - 1, end());

                data_[dist] = std::move(temp);
                result = begin() + dist;
            } else {
                try { 
                    result = new ( data_.GetAddress()) T(std::forward<Args>(args)...);
                } catch(...) {
                    std::destroy_n(begin(), 1);
                }
            }
        }

        ++size_;
        return result;
    }
    iterator Erase(const_iterator pos) /*noexcept(std::is_nothrow_move_assignable_v<T>)*/ {
        
        int dist = std::distance(cbegin(), pos);
        std::ranges::move(begin() + dist + 1, cend(), begin() + dist);
        std::destroy_n(end() - 1, 1);
        --size_;
        return begin() + dist;
    }

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
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

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);   
        std::swap(size_, other.size_);
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

private:

template<typename Iter>
void MoveOrCopy(Iter out, size_t count, Iter in) {
    if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
        std::uninitialized_move_n(out, count, in);
    } else {
        std::uninitialized_copy_n(out, count, in);
    }   
}


private:
    RawMemory<T> data_;
    size_t size_ = 0;
};