//下面是服务器源文件
#include <cstdlib>                                                                                                            
#include <deque>                                                                                                              
#include <iostream>                                                                                                           
#include <list>                                                                                                               
#include <memory>                                                                                                             
#include <set>                                                                                                                
#include <utility>                                                                                                            
#include <boost/asio.hpp>                                                                                                     

#include "chat_message.hpp"                                                                                                   

using boost::asio::ip::tcp;

//deque先进先出（在建立vector容器时，一般来说伴随这建立空间->填充数据->重建更大空间->复制原空间数据->删除原空间->添加新数据，
//如此反复，保证vector始终是一块独立的连续内存空间；在建立deque容器时，一般便随着建立空间->建立数据->建立新空间->填充新数据，如
//此反复，没有原空间数据的复制和删除过程，是由多个连续的内存空间组成的。）
using chat_message_queue = std::deque<chat_message>;

//聊天成员                                                                                                                    
class chat_participant {
public:
    virtual ~chat_participant() {}
    virtual void deliver(const chat_message& msg) = 0;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//聊天室                                                                                                                      
class chat_room {
    //聊天室中所有成员                                                                                                        
    std::set<chat_participant_ptr> m_participants;
    //历史消息(最多100条)                                                                                                     
    enum { max_recent_msgs = 100 };
    chat_message_queue m_recent_msgs;

public:
    //成员进入聊天室                                                                                                          
    void join(chat_participant_ptr participant) {
        m_participants.insert(participant);
        for (auto& msg : m_recent_msgs) {
            participant->deliver(msg);
        }
    }

    //成员离开聊天室
    void leave(chat_participant_ptr participant) {
        m_participants.erase(participant);
    }

    //传递消息
    void deliver(const chat_message& msg) {
        m_recent_msgs.push_back(msg);//压入尾部
        while (m_recent_msgs.size() > max_recent_msgs) {//保证100条
            m_recent_msgs.pop_front();
        }
        //将消息分发给所有成员
        for (auto participant : m_participants) {
            participant->deliver(msg);
        }
    }

};

//与客户端的聊天会话,即聊天成员（当接受客户端发送过来的请求后，就会创建一个chat_session，用它读写消息）
class chat_session : public chat_participant, public std::enable_shared_from_this<chat_session> {

    tcp::socket m_socket;//与客户端建立连接的socket
    chat_room& m_room;  //所属聊天室
    chat_message m_read_msg;//当前消息
    chat_message_queue m_write_msgs;//写给客户端的消息队列
public:
    chat_session(boost::asio::io_service& io_service, chat_room& room) :
        m_socket(io_service), m_room(room) {

    }

    tcp::socket& socket() {
        return m_socket;
    }

    void start() {
        m_room.join(shared_from_this());//聊天室增加成员
        do_read_header();//读取成员传递过来的消息头
    }

    virtual void deliver(const chat_message& msg) {
        //第一次调用时为false
        bool write_in_progress = !m_write_msgs.empty();

        //存放写给客户端的消息队列
        m_write_msgs.push_back(msg);

        //防止同时多次调用
        if (!write_in_progress) {
            do_write();//将历史消息写给客户端
        }
    }

    //读取成员传递过来的消息头
    void do_read_header() {
        auto sharedFromThis = shared_from_this();
        //异步读取4个字节
        boost::asio::async_read(m_socket,
            boost::asio::buffer(m_read_msg.data(), chat_message::header_length),
            [this, sharedFromThis](boost::system::error_code ec, std::size_t lenght) {
                if (!ec && m_read_msg.decoder_header()) {
                    do_read_body();
                }
                else {
                    m_room.leave(sharedFromThis);
                }

            });
    }

    //读取成员传递过来的消息体
    void do_read_body() {
        auto sharedFromThis = shared_from_this();
        boost::asio::async_read(m_socket,
            boost::asio::buffer(m_read_msg.body(), m_read_msg.body_length()),
            [this, sharedFromThis](boost::system::error_code ec, std::size_t lenght) {
                if (!ec) {
                    m_room.deliver(m_read_msg);
                    //继续读取下一条消息的头
                    do_read_header();
                }
                else {
                    m_room.leave(sharedFromThis);
                }
            });
    }

    //将m_write_msgs中的第一条写给客户端
    void do_write() {
        auto sharedFromThis = shared_from_this();
        boost::asio::async_write(m_socket,
            boost::asio::buffer(m_write_msgs.front().data(), m_write_msgs.front().length()),
            [this, sharedFromThis](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    m_write_msgs.pop_front();
                    if (!m_write_msgs.empty()) {
                        do_write();
                    }
                }
                else {
                    m_room.leave(sharedFromThis);
                }
            });
    }
};

//聊天服务器，当接受客户端发送过来的请求后，就会创建一个chat_session，用它读写消息
class chat_server {

    boost::asio::io_service& m_io_service;
    tcp::acceptor m_acceptor;
    //tcp::socket m_socket;//为客户端提供的,便于建立socket连接(服务端socket<---->客户端socket)
    chat_room m_room;//每个chat_server都有一个room

public:
    chat_server(boost::asio::io_service& io_service,
        const tcp::endpoint& endpoint) :m_io_service(io_service),
        m_acceptor(io_service, endpoint) {
        do_accept();
    }

    //异步接受客户端的请求
    void do_accept() {
        auto session = std::make_shared<chat_session>(m_io_service, m_room);

        m_acceptor.async_accept(session->socket(), [this, session](boost::system::error_code ec) {
            if (!ec) {
                session->start();
            }

            do_accept();
            });
    }

};

int main(int argc, const char* argv[]) {

    try {
        if (argc < 2) {
            std::cerr << "Usage: chat_server <port> [<port>...]\n";
            return 1;
        }

        printf_s("argurement input correct\n");
        boost::asio::io_service io_service;

        std::list<chat_server*> servers;
        for (int i = 1; i < argc; ++i) {
            tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
            servers.push_back(new chat_server(io_service, endpoint));//注意容器里保存的指针，程序最后要负责释放所指向的堆
        }

        io_service.run();
        printf_s("test\n");
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
