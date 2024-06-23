#include <boost/asio.hpp>
#include <iostream>
#include <array>

using boost::asio::ip::tcp;

class client {
public:
    client(boost::asio::io_context& io_context,
        const std::string& host, const std::string& port)
        : io_context_(io_context),
        socket_(io_context) {
        connect(host, port);
    }

    void send(const std::string& message) {
        boost::asio::write(socket_, boost::asio::buffer(message));
        std::array<char, 128> reply;
        size_t reply_length = boost::asio::read(socket_, boost::asio::buffer(reply, message.size()));
        std::cout << "Reply is: ";
        std::cout.write(reply.data(), reply_length);
        std::cout << "\n";
    }

private:
    boost::asio::io_context& io_context_;
    tcp::socket socket_;

    void connect(const std::string& host, const std::string& port) {
        tcp::resolver resolver(io_context_);
        boost::asio::connect(socket_, resolver.resolve(host, port));
    }
};

int main() {
    try {
        boost::asio::io_context io_context;
        client c(io_context, "127.0.0.1", "7890");
        c.send("Hello, World!");
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
