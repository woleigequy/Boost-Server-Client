#ifndef CHAT_MESSAGE_HPP
#define CHAT_MESSAGE_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <random>
#include <string>

class chat_message {
public:
    enum { header_length = 4 };
    enum { max_body_length = 512 };

    chat_message() : body_length_(0) { }

    const char* data() const { return data_; }
    char* data() { return data_; }
    std::size_t length() const { return header_length + body_length_; }

    const char* body() const { return data_ + header_length; }
    char* body() { return data_ + header_length; }
    std::size_t body_length() const { return body_length_; }
    void body_length(std::size_t new_length) {
        body_length_ = new_length;
        if (body_length_ > max_body_length)
            body_length_ = max_body_length;
    }

    bool decode_header() {
        char header[header_length + 1] = "";
        strncpy_s(header, sizeof(header), data_, header_length);
        body_length_ = std::atoi(header);
        if (body_length_ > max_body_length) {
            body_length_ = 0;
            return false;
        }
        return true;
    }

    void encode_header() {
        char header[header_length + 1] = "";
        sprintf_s(header, sizeof(header), "%4d", static_cast<int>(body_length_));
        memcpy_s(data_, sizeof(data_), header, header_length);
    }

    void update_body_with_time_and_id(const std::string& sender_id, const std::string& recipient_id) {
        std::string time_and_ids = get_current_time() + " Sender:" + sender_id + " Recipient:" + recipient_id + " ";
        std::string updated_body = time_and_ids + std::string(body(), body_length_);
        body_length(updated_body.size());
        memcpy_s(body(), max_body_length, updated_body.data(), updated_body.size());
    }

private:
    std::size_t body_length_;
    char data_[header_length + max_body_length];

    std::string generate_random_id(size_t length = 5) {
        const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        std::random_device random_device;
        std::mt19937 generator(random_device());
        std::uniform_int_distribution<> distribution(0, chars.size() - 1);

        std::string random_string;
        for (size_t i = 0; i < length; ++i) {
            random_string += chars[distribution(generator)];
        }
        return random_string;
    }

    std::string get_current_time() {
        std::time_t now = std::time(nullptr);
        struct tm timeinfo = { 0 };
        localtime_s(&timeinfo, &now); // Use localtime_s instead of localtime
        char buf[100] = { 0 };
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        return std::string(buf);
    }
};

#endif // CHAT_MESSAGE_HPP
