#ifndef RingBuffer_h
#define RingBuffer_h

#include <Arduino.h>

template <typename T>
class RingBuffer
{
    private:
        T* buffer;
        size_t size;
        size_t head;
        size_t tail;
        size_t count;

    public:
        RingBuffer(size_t size) : size(size), head(0), tail(0), count(0)
        {
            buffer = new T[size];
        }

        ~RingBuffer()
        {
            delete[] buffer;
        }

        bool push(const T& data)
        {
            if (this->isFull()) {
                tail = (tail + 1) % size;
            } else {
                count++;
            }
            buffer[head] = data;
            head = (head + 1) % size;
            return true;
        }

        bool pop(T& data)
        {
            if (this->isEmpty()) {
                return false;
            }
            data = buffer[tail];
            tail = (tail + 1) % size;
            count--;
            return true;
        }

        void serializeToJson(JsonArray& jsonArray) const
        {
            size_t idx = tail;
            for (size_t i = 0; i < count; ++i) {
                JsonObject obj = jsonArray.add<JsonObject>();
                buffer[idx].serialize(obj);
                idx = (idx + 1) % size;
            }
        }
        
        bool isFull() const
        {
            return count == size;
        }

        bool isEmpty() const
        {
            return count == 0;
        }

        size_t getCount() const
        {
            return count;
        }
};

#endif