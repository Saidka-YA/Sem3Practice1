#pragma once

#include <cstddef>
#include <stdexcept>

using namespace std;

template <typename T>
class Vector {
private:
    T* data;
    size_t size;
    size_t capacity;

public:
    Vector(const Vector& other);
    Vector() : data(nullptr), size(0), capacity(0) {};
    Vector& operator=(const Vector& other);
    ~Vector() {
        delete[] data;
    };
    void pushBack(const T& value);
    size_t Size() const {
        return size;
    };
    T& operator[](size_t index);
    const T& operator[](size_t index) const;
    void Clear();
};

template <typename T>
void Vector<T>::pushBack(const T& value) {
    if (size == capacity) {
        size_t newCapacity;
        if (capacity == 0) newCapacity = 2;
        else newCapacity = capacity * 2;
        T* newData = new T[newCapacity];
        for (size_t i = 0; i < size; i++) {
            newData[i] = data[i];
        }
        delete[] data;
        data = newData;
        capacity = newCapacity;
    } 
    
    data[size] = value;
    size++;
}

template <typename T> 
T& Vector<T>::operator[](size_t index) {
    if (index >= size) {
        throw out_of_range("Индекс за пределами массива");
    }
    return data[index];
}

template <typename T>
Vector<T>::Vector(const Vector& other) {
    size = other.size;
    capacity = other.capacity;
    if (capacity == 0) data = nullptr;
    else {
        data = new T[capacity];
        for (size_t i = 0; i < size; i++) {
            data[i] = other.data[i];
        }
    }
}

template <typename T>
Vector<T>& Vector<T>::operator=(const Vector& other) {
   if (this == &other) return *this;
   T* newData = new T[other.capacity];
   for (size_t i = 0; i < other.size; i++) {
    newData[i] = other.data[i];
   }

   delete[] data;
   data = newData;
   size = other.size;
   capacity = other.capacity;

   return *this;

}

template <typename T>
const T& Vector<T>::operator[](size_t index) const {
    if (index >= size) {
        throw out_of_range("Индекс за пределами массива");
    }
    return data[index]; 
}

template <typename T>
void Vector<T>::Clear() {
    size = 0;
}