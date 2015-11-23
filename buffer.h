#ifndef BUFFER_H
#define BUFFER_H

#include "data.h"

class Buffer
{
    byte* data = nullptr;
    size_t size = 0;
    size_t readOffset = 0;
    size_t writeOffset = 0;
    bool isOwnBuffer = false;
    void init(size_t size);
public:
    Buffer() {}
    Buffer(size_t size);
    Buffer(const Buffer& b);
    ~Buffer();
    size_t getSize();
    byte* getData();
    const byte* getData() const;
    byte* getWritePointer();
    const byte* getReadPointer() const;
    void setWriteOffset(size_t writeOffset);
    size_t getWriteOffset() const;
    void setReadOffset(size_t readOffset);
    size_t getReadOffset() const;
    void Write(const byte* source, size_t size);
    void Clear(bool full = false);
    
    template<class T>
    void Write(const T* data, size_t size)
    {
        Write((const byte*)data, size);
    }

    template<class T>
    void Write(const T& data)
    {
        Write((const byte*)&data, sizeof(T));
    }

    template<class T>
    Buffer(const T* src, size_t size = sizeof(T))
    {
        init(size);
        memcpy(data, src, size);
    }

    template<class T>
    Buffer(T* src, size_t size = sizeof(T))
    {
        this->size = size;
        data = (byte*)src;
    }

    template<typename T>
    operator const T&() const
    {
        return reinterpret_cast<T&>(*data);
    }

    template<typename T>
    operator T&()
    {
        return reinterpret_cast<T&>(*data);
    }
};

Buffer mergeData(void* dataA, void* dataB, size_t sizeA, size_t sizeB);

#endif
