//��������Ϣ��ͷ�ļ�
#ifndef chat_message_hpp                                                                                                      
#define chat_message_hpp                                                                                                      

#include <cstdio>                                                                                                             
#include <cstdlib>                                                                                                            
#include <cstring>                                                                                                            

//�������ݵ���Ϣ�����ࣺ����header��body��������httpЭ�飩��header������body������ԣ����������ù̶���4���ֽڱ�����body���ֽڸ���
class chat_message {

public:
    enum { header_length = 4 };//�̶���4���ֽ�                                                                                  
    enum { max_body_length = 512 };//body����ֽڸ���                                                                           

    chat_message() :m_body_length(0), m_data("") {}
    ~chat_message() {}

    //��Ϣ�ֽ�����
    char* data() { return m_data; };

    //��Ϣ�ֽ���������Ч���ȣ���ͷ+����ܳ���
    std::size_t length() { return header_length + m_body_length; }

    //��ȡָ������ָ��
    char* body() { return m_data + header_length; }

    //��ȡ���峤��
    std::size_t body_length() { return m_body_length; }

    //���ð��峤��
    void body_length(std::size_t new_length) {
        m_body_length = new_length;
        if (m_body_length > max_body_length) {
            m_body_length = max_body_length;
        }
    }

    //����ͷ��Ϣ
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

    //����ͷ��Ϣ
    void encode_header() {
        char header[header_length + 1] = "";
        sprintf_s(header, "%4d", static_cast<int>(m_body_length));
        std::memcpy(m_data, header, header_length);
    }

private:
    std::size_t m_body_length;//body���ȣ����ֽڸ���
    char m_data[header_length + max_body_length];//���������Ϣ���̶���ǰ4���ֽڱ�����body���ֽڸ�����������body��Ϣ
};

#endif /* chat_message_hpp */
