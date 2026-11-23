#pragma once
#include <cstddef>
#include <stdexcept>
#include <typeindex>

using namespace std;

template <typename T>
class Vector {
private:
    T* data;
    size_t size;
    size_t capacity;

public:
    Vector() : data(nullptr), size(0), capacity(0) {};
    ~Vector() {
        delete[] data;
    };
    void pushBack(const T& value);
    size_t Size() const {
        return size;
    };
    T& operator[](size_t index);

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