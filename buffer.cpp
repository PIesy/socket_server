#include "buffer.h"

Buffer::Buffer(size_t size)
{
    init(size);
}

Buffer::~Buffer()
{
    if (isOwnBuffer)
        delete[] data;
}

Buffer::Buffer(const Buffer& b)
{
    size = b.size;
    init(size);
    memcpy(data, b.data, size);
    writeOffset = b.writeOffset;
    readOffset = b.readOffset;
}

void Buffer::init(size_t size)
{
    isOwnBuffer = true;
    data = new byte[size]();
    this->size = size;
}

size_t Buffer::getSize()
{
    return size;
}

byte* Buffer::getData()
{
    return data;
}

const byte* Buffer::getData() const
{
    return data;
}

byte* Buffer::getWritePointer()
{
    return data + writeOffset;
}

const byte* Buffer::getReadPointer() const
{
    return data + readOffset;
}

void Buffer::Write(const byte* source, size_t size)
{
    if (size + writeOffset <= this->size)
        memcpy(data + writeOffset, source, size);
    writeOffset += size;
}

void Buffer::Clear(bool full)
{
    readOffset = 0;
    writeOffset = 0;
    if (full)
        memset(data, 0, size);
}

void Buffer::setWriteOffset(size_t writeOffset)
{
    this->writeOffset += writeOffset;
}

size_t Buffer::getWriteOffset() const
{
    return this->writeOffset;
}

void Buffer::setReadOffset(size_t readOffset)
{
    this->readOffset += readOffset;
}

size_t Buffer::getReadOffset() const
{
    return readOffset;
}

Buffer mergeData(void* dataA, void* dataB, size_t sizeA, size_t sizeB)
{
    Buffer result(sizeA + sizeB);
    result.Write((byte*)dataA, sizeA);
    result.Write((byte*)dataB, sizeB);
    return result;
}
