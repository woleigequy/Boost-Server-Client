#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include <map>

#include "chat_message.hpp"

using boost::asio::ip::tcp;
using chat_message_queue = std::deque<chat_message>;

class chat_participant {
public:
    virtual ~chat_participant() {}
    virtual void deliver(const chat_message& msg) = 0;
    virtual std::string get_id() const = 0;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

class chat_room {
    std::set<chat_participant_ptr> participants_;
    chat_message_queue recent_msgs_;
    enum { max_recent_msgs = 100 };

public:
    void join(chat_participant_ptr participant) {
        participants_.insert(participant);
        // Comment out the following line to prevent sending previous messages
        // for (auto& msg : recent_msgs_) {
        //     participant->deliver(msg);
        // }
    }

    void leave(chat_participant_ptr participant) {
        participants_.erase(participant);
    }

    void deliver(const chat_message& msg) {
        recent_msgs_.push_back(msg);
        while (recent_msgs_.size() > max_recent_msgs) {
            recent_msgs_.pop_front();
        }

        std::string body(msg.body(), msg.body_length());
        std::string recipient_id = body.substr(body.find("Recipient:") + 10, 5); // assuming recipient ID is of length 5
        for (auto participant : participants_) {
            if (participant->get_id() == recipient_id) {
                participant->deliver(msg);
                break;
            }
        }
    }

    void print_connected_clients() {
        std::cout << "Connected clients: ";
        for (auto participant : participants_) {
            std::cout << participant->get_id() << " ";
        }
        std::cout << std::endl;
    }
};

class chat_session : public chat_participant, public std::enable_shared_from_this<chat_session> {
    tcp::socket socket_;
    chat_room& room_;
    chat_message read_msg_;
    chat_message_queue write_msgs_;
    std::string client_id_;

public:
    chat_session(boost::asio::io_service& io_service, chat_room& room)
        : socket_(io_service), room_(room), client_id_(generate_random_id()) {}

    tcp::socket& socket() {
        return socket_;
    }

    void start() {
        room_.join(shared_from_this());
        room_.print_connected_clients();
        send_id_to_client();
        do_read_header();
    }

    void deliver(const chat_message& msg) {
        bool write_in_progress = !write_msgs_.empty();
        write_msgs_.push_back(msg);
        if (!write_in_progress) {
            do_write();
        }
    }

    std::string get_id() const override {
        return client_id_;
    }

    void send_id_to_client() {
        chat_message msg;
        std::string id_message = "Your ID: " + client_id_;
        msg.body_length(id_message.size());
        std::memcpy(msg.body(), id_message.data(), msg.body_length());
        msg.encode_header();
        deliver(msg);
    }

    void do_read_header() {
        auto self(shared_from_this());
        boost::asio::async_read(socket_,
            boost::asio::buffer(read_msg_.data(), chat_message::header_length),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec && read_msg_.decode_header()) {
                    do_read_body();
                }
                else {
                    room_.leave(shared_from_this());
                }
            });
    }

    void do_read_body() {
        auto self(shared_from_this());
        boost::asio::async_read(socket_,
            boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    std::cout << "Message received: " << std::string(read_msg_.body(), read_msg_.body_length()) << std::endl;
                    room_.deliver(read_msg_);
                    do_read_header();
                }
                else {
                    room_.leave(shared_from_this());
                }
            });
    }

    void do_write() {
        auto self(shared_from_this());
        boost::asio::async_write(socket_,
            boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    write_msgs_.pop_front();
                    if (!write_msgs_.empty()) {
                        do_write();
                    }
                }
                else {
                    room_.leave(shared_from_this());
                }
            });
    }

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
};

class chat_server {
    boost::asio::io_service& io_service_;
    tcp::acceptor acceptor_;
    chat_room room_;

public:
    chat_server(boost::asio::io_service& io_service, const tcp::endpoint& endpoint)
        : io_service_(io_service), acceptor_(io_service, endpoint) {
        do_accept();
    }

    void do_accept() {
        auto session = std::make_shared<chat_session>(io_service_, room_);
        acceptor_.async_accept(session->socket(),
            [this, session](boost::system::error_code ec) {
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

        boost::asio::io_service io_service;

        std::list<chat_server*> servers;
        for (int i = 1; i < argc; ++i) {
            tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
            servers.push_back(new chat_server(io_service, endpoint));
        }

        io_service.run();
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
