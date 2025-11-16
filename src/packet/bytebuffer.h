#ifndef BYTEBUFFER_H
#define BYTEBUFFER_H

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <arpa/inet.h> // htonl, ntohl, htons, ntohs

namespace hangman {

class ByteBuffer {
public:
    std::vector<uint8_t> buf;
    size_t rpos = 0;

    ByteBuffer() = default;
    ByteBuffer(size_t reserve) { buf.reserve(reserve); }

    // write primitives in network byte order
    void write_u8(uint8_t v) { buf.push_back(v); }
    void write_u16(uint16_t v) { uint16_t x = htons(v); append_bytes(&x, sizeof(x)); }
    void write_u32(uint32_t v) { uint32_t x = htonl(v); append_bytes(&x, sizeof(x)); }
    void write_bytes(const uint8_t* data, size_t len) { append_bytes(data, len); }
    void write_string(const std::string& s) {
        // write length (u16) then bytes
        if (s.size() > 65535) throw std::runtime_error("string too long");
        write_u16((uint16_t)s.size());
        append_bytes((const uint8_t*)s.data(), s.size());
    }

    // read primitives (throws if not enough data)
    uint8_t read_u8() {
        require(1);
        return buf[rpos++];
    }
    uint16_t read_u16() {
        require(2);
        uint16_t x;
        memcpy(&x, &buf[rpos], 2);
        rpos += 2;
        return ntohs(x);
    }
    uint32_t read_u32() {
        require(4);
        uint32_t x;
        memcpy(&x, &buf[rpos], 4);
        rpos += 4;
        return ntohl(x);
    }
    std::string read_string() {
        uint16_t len = read_u16();
        require(len);
        std::string s((char*)&buf[rpos], len);
        rpos += len;
        return s;
    }
    std::vector<uint8_t> read_raw(size_t len) {
        require(len);
        std::vector<uint8_t> v(&buf[rpos], &buf[rpos]+len);
        rpos += len;
        return v;
    }

    size_t size() const { return buf.size(); }
    const uint8_t* data() const { return buf.data(); }

private:
    void append_bytes(const void* p, size_t n) {
        const uint8_t* b = (const uint8_t*)p;
        buf.insert(buf.end(), b, b+n);
    }
    void require(size_t n) {
        if (rpos + n > buf.size()) throw std::runtime_error("ByteBuffer: not enough data");
    }
};

} // namespace hangman

#endif // BYTEBUFFER_H
