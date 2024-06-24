//�����ǿͻ���Դ�ļ�
#include <cstdlib>                                                                                                            
#include <deque>                                                                                                              
#include <iostream>                                                                                                           
#include <thread>                                                                                                             
#include <boost/asio.hpp>                                                                                                     

#include "chat_message.hpp"                                                                                                   

using boost::asio::ip::tcp;

using chat_message_queue = std::deque<chat_message>;

class chat_client {
    boost::asio::io_service& m_io_service;
    tcp::socket m_socket;
    chat_message m_read_msg;
    chat_message_queue m_write_msgs;
public:
    chat_client(boost::asio::io_service& io_service,
        tcp::resolver::iterator endpoint_iterator) :
        m_io_service(io_service), m_socket(io_service) {
        do_connect(endpoint_iterator);
    }

    void write(const chat_message& msg) {
        //��lambda��m_io_service��run�����߳�ִ�У�������OC�е�performSelector: onThread: withObject: waitUntilDone: modes:��
        m_io_service.post([this, msg]() {
            bool write_in_progress = !m_write_msgs.empty();
            m_write_msgs.push_back(msg);

            if (!write_in_progress) {
                do_write();
            }
            });
    }

    void close() {
        m_io_service.post([this]() {
            m_socket.close();
            });
    }

    void do_connect(tcp::resolver::iterator endpoint_iterator) {
        boost::asio::async_connect(m_socket, endpoint_iterator,
            [this](boost::system::error_code ec, tcp::resolver::iterator it) {
                if (!ec) {
                    do_read_header();
                }
            });
    }

    void do_read_header() {
        boost::asio::async_read(m_socket,
            boost::asio::buffer(m_read_msg.data(), chat_message::header_length),
            [this](boost::system::error_code ec, std::size_t lenght) {
                if (!ec && m_read_msg.decoder_header()) {
                    do_read_body();
                }
                else {
                    m_socket.close();
                }
            });
    }

    void do_read_body() {
        boost::asio::async_read(m_socket,
            boost::asio::buffer(m_read_msg.body(), m_read_msg.body_length()),
            [this](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    std::cout.write(m_read_msg.body(), m_read_msg.body_length());
                    std::cout << std::endl;

                    //������ȡ��һ����Ϣ��ͷ
                    do_read_header();

                }
                else {
                    m_socket.close();
                }
            });
    }

    void do_write() {
        boost::asio::async_write(m_socket,
            boost::asio::buffer(m_write_msgs.front().data(), m_write_msgs.front().length()),
            [this](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    m_write_msgs.pop_front();
                    if (!m_write_msgs.empty()) {
                        do_write();
                    }
                }
                else {
                    m_socket.close();
                }
            });
    }

};

int main(int argc, const char* argv[]) {

    try {
        if (argc != 3) {
            std::cerr << "Usage: chat_client <host> <port>\n";
            return 1;
        }

        boost::asio::io_service io_service;
        tcp::resolver resolver(io_service);
        auto endpoint_iterator = resolver.resolve({ argv[1], argv[2] });
        chat_client c(io_service, endpoint_iterator);

        std::thread t([&io_service]() {
            io_service.run();//��������io_service�󶨵��¼�Դ�Ļص��������߳���ִ�У�����ָ����boost::asio::async_xxx�е�lambda��������
            });

        char line[chat_message::max_body_length + 1] = "";
        while (std::cin.getline(line, chat_message::max_body_length + 1)) {
            chat_message msg;
            msg.body_length(std::strlen(line));
            std::memcpy(msg.body(), line, msg.body_length());
            msg.encode_header();
            c.write(msg);
        }

        c.close();//�������close���������߳���run�����˳�����Ϊboost::asio::async_read�¼�Դһֱע����io_service��.
        t.join();

    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
