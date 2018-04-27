#ifndef LWS_CIRCULAR_STREAMS_H_INCLUDED
#define LWS_CIRCULAR_STREAMS_H_INCLUDED

#include <utility>

namespace lws {

template<typename RingBufferType> class RingReader;
template<typename RingBufferType> class RingWriter;

template<typename RingBufferType>
class RingReader : public Reader {
private:
    RingBufferType *buffer;
    RingWriter<RingBufferType> *other;
    bool closed{ false };

    friend class RingWriter<RingBufferType>;

public:
    RingReader(RingBufferType *buffer, RingWriter<RingBufferType> *writer) : buffer(buffer), other(writer) {
    }

public:
    int32_t read(uint8_t *ptr, size_t size) override;

    int32_t read() override;

    void close() override;
};

template<typename RingBufferType>
class RingWriter : public Writer {
private:
    RingBufferType *buffer;
    RingReader<RingBufferType> *other;
    bool closed{ false };

    friend class RingReader<RingBufferType>;

public:
    RingWriter(RingBufferType *buffer, RingReader<RingBufferType> *reader) : buffer(buffer), other(reader) {
    }

public:
    using Writer::write;

    int32_t write(uint8_t *ptr, size_t size) override;

    int32_t write(uint8_t byte) override;

    void close() override;
};

template<typename RingBufferType>
int32_t RingReader<RingBufferType>::read(uint8_t *ptr, size_t size) {
    if (buffer->empty()) {
        if (other->closed) {
            return EOS;
        }
        return 0;
    }

    return Reader::read(ptr, size);
}

template<typename RingBufferType>
int32_t RingReader<RingBufferType>::read() {
    // Not totally happy with this. Don't have any way of
    // differentiating between empty and EoS though.
    if (buffer->empty()) {
        return EOS;
    }
    return buffer->shift();
}

template<typename RingBufferType>
void RingReader<RingBufferType>::close() {
    closed = true;
}

template<typename RingBufferType>
int32_t RingWriter<RingBufferType>::write(uint8_t *ptr, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if (buffer->full()) {
            return i;
        }
        buffer->push(ptr[i]);
    }
    return size;
}

template<typename RingBufferType>
int32_t RingWriter<RingBufferType>::write(uint8_t byte) {
    if (buffer->full()) {
        return EOS;
    }
    buffer->push(byte);
    return 1;
}

template<typename RingBufferType>
void RingWriter<RingBufferType>::close() {
    closed = true;
}

template<typename RingBufferType>
class CircularStreams {
    using OuterType = CircularStreams<RingBufferType>;

    RingBufferType buffer;
    RingReader<RingBufferType> reader{ &buffer, &writer };
    RingWriter<RingBufferType> writer{ &buffer, &reader };

public:
    CircularStreams() {
    }

    CircularStreams(RingBufferType &&buffer) : buffer(std::forward<RingBufferType>(buffer)) {
    }

public:
    Writer &getWriter() {
        return writer;
    }

    Reader &getReader() {
        return reader;
    }

    void clear() {
        buffer.clear();
        reader = RingReader<RingBufferType>{ &buffer, &writer };
        writer = RingWriter<RingBufferType>{ &buffer, &reader };
    }

};

}

#endif