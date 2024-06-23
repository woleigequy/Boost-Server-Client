//下面是消息的头文件
#ifndef chat_message_hpp                                                                                                      
#define chat_message_hpp                                                                                                      

#include <cstdio>                                                                                                             
#include <cstdlib>                                                                                                            
#include <cstring>                                                                                                            

//承载内容的消息包的类：包含header和body（类似于http协议），header包含了body相关属性，比如这里用固定的4个字节保存了body中字节个数
class chat_message {

public:
    enum { header_length = 4 };//固定的4个字节                                                                                  
    enum { max_body_length = 512 };//body最大字节个数                                                                           

    chat_message() :m_body_length(0), m_data("") {}
    ~chat_message() {}

    //信息字节数组
    char* data() { return m_data; };

    //信息字节数组中有效长度，即头+体的总长度
    std::size_t length() { return header_length + m_body_length; }

    //获取指向包体的指针
    char* body() { return m_data + header_length; }

    //获取包体长度
    std::size_t body_length() { return m_body_length; }

    //设置包体长度
    void body_length(std::size_t new_length) {
        m_body_length = new_length;
        if (m_body_length > max_body_length) {
            m_body_length = max_body_length;
        }
    }

    //解码头信息
    bool decoder_header() {
        char header[header_length + 1] = "";
        strncat_s(header, m_data, header_length);
        m_body_length = std::atoi(header);
        if (m_body_length > max_body_length) {
            m_body_length = 0;
            return false;
        }
        return true;
    }

    //编码头信息
    void encode_header() {
        char header[header_length + 1] = "";
        sprintf_s(header, "%4d", static_cast<int>(m_body_length));
        std::memcpy(m_data, header, header_length);
    }

private:
    std::size_t m_body_length;//body长度，即字节个数
    char m_data[header_length + max_body_length];//用来存放消息：固定的前4个字节保存了body中字节个数，接着是body信息
};

#endif /* chat_message_hpp */
