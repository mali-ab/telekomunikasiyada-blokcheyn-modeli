#include "p2p.h"
#include "signature.h"

#include <algorithm>
#include <istream>

namespace {
constexpr std::size_t kMaxMessageBytes = 128 * 1024;
}

P2PNode::P2PNode(int port, Blockchain &bc)
    : port_(port), blockchain_(bc), acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)) {}

P2PNode::~P2PNode() {
    boost::system::error_code ignored;
    acceptor_.close(ignored);
    {
        std::lock_guard<std::mutex> lock(peersMutex_);
        for (const auto &socket : peers_) socket->shutdown(tcp::socket::shutdown_both, ignored);
        for (const auto &socket : peers_) socket->close(ignored);
        peers_.clear();
        connected_addresses_.clear();
    }
    io_context_.stop();
    if (ioThread_.joinable()) ioThread_.join();
}

void P2PNode::start_node() {
    if (ioThread_.joinable()) return;
    do_accept();
    ioThread_ = std::thread([this]() { io_context_.run(); });
    std::cout << "[P2P]: Düwün porty diňleýär " << port_ << "..." << std::endl;
}

void P2PNode::do_accept() {
    auto socket = std::make_shared<tcp::socket>(io_context_);
    acceptor_.async_accept(*socket, [this, socket](boost::system::error_code error) {
        if (!error) {
            boost::system::error_code endpointError;
            const auto endpoint = socket->remote_endpoint(endpointError);
            if (!endpointError) {
                const std::string peerInfo = endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
                bool added = false;
                {
                    std::lock_guard<std::mutex> lock(peersMutex_);
                    if (connected_addresses_.insert(peerInfo).second) {
                        peers_.push_back(socket);
                        added = true;
                    }
                }
                if (added) {
                    std::cout << "[P2P]: Täze düwün birikdirildi: " << peerInfo << std::endl;
                    do_read(socket);
                }
            }
        }
        if (acceptor_.is_open()) do_accept();
    });
}

void P2PNode::do_read(std::shared_ptr<tcp::socket> socket) {
    // TCP is a byte stream. async_read_until retains partial data until a
    // complete newline-delimited frame arrives, unlike async_read_some.
    auto buffer = std::make_shared<boost::asio::streambuf>(kMaxMessageBytes);
    do_read(std::move(socket), std::move(buffer));
}

void P2PNode::do_read(std::shared_ptr<tcp::socket> socket, std::shared_ptr<boost::asio::streambuf> buffer) {
    boost::asio::async_read_until(*socket, *buffer, '\n',
        [this, socket, buffer](boost::system::error_code error, std::size_t) {
            if (error) {
                remove_peer(socket);
                return;
            }
            std::istream input(buffer.get());
            std::string message;
            std::getline(input, message);
            if (!message.empty()) handle_incoming_block(message);
            // Keep the buffer: async_read_until is allowed to read bytes that
            // belong to the next frame while locating this delimiter.
            do_read(socket, buffer);
        });
}

void P2PNode::remove_peer(const std::shared_ptr<tcp::socket> &socket) {
    boost::system::error_code ignored;
    const auto endpoint = socket->remote_endpoint(ignored);
    std::lock_guard<std::mutex> lock(peersMutex_);
    peers_.erase(std::remove(peers_.begin(), peers_.end(), socket), peers_.end());
    if (!ignored) {
        connected_addresses_.erase(endpoint.address().to_string() + ":" + std::to_string(endpoint.port()));
    }
}

void P2PNode::handle_incoming_block(const std::string &message) {
    if (message.rfind("NEW_BLOCK|", 0) != 0) return;
    std::string eventHash;
    bool registeredEvent = false;
    try {
        std::stringstream stream(message.substr(10));
        std::string data, signatureString, publicExponent, modulus;
        if (!std::getline(stream, data, '|') || !std::getline(stream, signatureString, '|') ||
            !std::getline(stream, publicExponent, '|') || !std::getline(stream, modulus, '|')) {
            throw std::invalid_argument("Invalid message format");
        }

        std::vector<long long> signature;
        std::stringstream signatureStream(signatureString);
        std::string segment;
        while (std::getline(signatureStream, segment, ',')) {
            if (!segment.empty()) signature.push_back(std::stoll(segment));
        }
        const long long publicKey = std::stoll(publicExponent);
        const long long publicModulus = std::stoll(modulus);
        if (signature.empty() || DigitalSignature::decrypt(signature, publicKey, publicModulus) != data) {
            throw std::invalid_argument("Signature verification failed");
        }

        eventHash = sha256(data);
        if (!received_event_hashes_.insert(eventHash).second) {
            std::cout << "[P2P]: Gaýtalanan blok habary äsgerilmedi." << std::endl;
            return;
        }
        registeredEvent = true;
        blockchain_.AddBlock(Block(0, data));
        std::cout << "[P2P]: Gol tassyklanyldy we blok hasaba alyndy." << std::endl;
    } catch (const std::exception &e) {
        if (registeredEvent) received_event_hashes_.erase(eventHash);
        std::cerr << "[P2P ÝALŇYŞLYK]: Gelýän blokda ýalňyşlyk: " << e.what() << std::endl;
    }
}

void P2PNode::list_active_peers() {
    std::lock_guard<std::mutex> lock(peersMutex_);
    std::cout << "\n--- IŞJEŇ P2P TOR DÜWÜNLERI ---" << std::endl;
    if (connected_addresses_.empty()) {
        std::cout << ">>> Torda başga işjeň düwün ýok." << std::endl;
    } else {
        int count = 1;
        for (const auto &address : connected_addresses_) {
            std::cout << " [" << count++ << "] IP/Port -> " << address << " [BIRIKDIRILDI]" << std::endl;
        }
        std::cout << ">>> Jemi: " << connected_addresses_.size() << " düwün birikdirildi." << std::endl;
    }
    std::cout << "--------------------------------\n" << std::endl;
}

void P2PNode::connect_to_peer(const std::string &host, const std::string &port) {
    auto socket = std::make_shared<tcp::socket>(io_context_);
    tcp::resolver resolver(io_context_);
    boost::asio::connect(*socket, resolver.resolve(host, port));
    const std::string peerInfo = host + ":" + port;
    bool added = false;
    {
        std::lock_guard<std::mutex> lock(peersMutex_);
        if (connected_addresses_.insert(peerInfo).second) {
            peers_.push_back(socket);
            added = true;
        }
    }
    if (added) {
        std::cout << "[P2P]: Düwüne birikdirildi " << peerInfo << "." << std::endl;
        do_read(socket);
    }
}

void P2PNode::broadcast_message(const std::string &message) {
    if (message.size() > kMaxMessageBytes - 1) {
        throw std::invalid_argument("P2P message exceeds the maximum frame size");
    }
    std::vector<std::shared_ptr<tcp::socket>> peers;
    {
        std::lock_guard<std::mutex> lock(peersMutex_);
        peers = peers_;
    }

    for (const auto &socket : peers) {
        boost::system::error_code error;
        boost::asio::write(*socket, boost::asio::buffer(message + "\n"), error);
        if (error) remove_peer(socket);
    }
    std::cout << "[P2P]: Täze blok tora ýaýradyldy." << std::endl;
}
