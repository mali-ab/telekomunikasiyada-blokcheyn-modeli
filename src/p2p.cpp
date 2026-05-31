#include "p2p.h"
#include "signature.h"

P2PNode::P2PNode(int port, Blockchain &bc)
    : port_(port), blockchain_(bc), acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)) {}

void P2PNode::start_node() {
    std::thread([this]() {
        do_accept();
        io_context_.run();
    }).detach();
    std::cout << "[P2P]: Düwün porty diňleýär " << port_ << "..." << std::endl;
}

void P2PNode::do_accept() {
    auto socket = std::make_shared<tcp::socket>(io_context_);
    acceptor_.async_accept(*socket, [this, socket](boost::system::error_code ec) {
        if (!ec) {
            std::string peer_info = socket->remote_endpoint().address().to_string() +
                                    ":" + std::to_string(socket->remote_endpoint().port());
            if (connected_addresses_.find(peer_info) == connected_addresses_.end()) {
                std::cout << "[P2P]: Täze düwün birikdirildi: " << peer_info << std::endl;
                peers_.push_back(socket);
                connected_addresses_.insert(peer_info);
                do_read(socket);
            }
        }
        do_accept();
    });
}

void P2PNode::do_read(std::shared_ptr<tcp::socket> socket) {
    auto buffer = std::make_shared<std::vector<char>>(1024);
    socket->async_read_some(boost::asio::buffer(*buffer),
        [this, socket, buffer](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
            std::string msg(buffer->data(), length);
            handle_incoming_block(msg);
            do_read(socket);
        }
    });
}

void P2PNode::handle_incoming_block(const std::string &message) {
    if (message.find("NEW_BLOCK|") != 0) return;
    try {
        std::string raw = message.substr(10);
        std::stringstream ss(raw);
        std::string data, sigStr, eStr, nStr;
        if (!std::getline(ss, data, '|') || !std::getline(ss, sigStr, '|') ||
            !std::getline(ss, eStr, '|') || !std::getline(ss, nStr, '|')) {
            throw std::invalid_argument("Invalid message format");
        }
        std::vector<long long> receivedSig;
        std::stringstream sigSS(sigStr);
        std::string segment;
        while (std::getline(sigSS, segment, ',')) {
            if (!segment.empty()) receivedSig.push_back(std::stoll(segment));
        }
        long long pubE = std::stoll(eStr);
        long long modN = std::stoll(nStr);
        std::string decrypted = DigitalSignature::decrypt(receivedSig, pubE, modN);
        if (decrypted == data) {
            std::cout << "[P2P]: Gol tassyklanyldy." << std::endl;
            blockchain_.AddBlock(Block(blockchain_.getLatestBlock().getIndex() + 1, data));
        }
    } catch (const std::exception& e) {
        std::cerr << "[P2P ÝALŇYŞLYK]: Gelýän blokda ýalňyşlyk: " << e.what() << std::endl;
    }
}

void P2PNode::list_active_peers() {
    std::cout << "\n--- IŞJEŇ P2P TOR DÜWÜNLERI ---" << std::endl;
    if (connected_addresses_.empty()) {
        std::cout << ">>> Torda başga işjeň düwün ýok." << std::endl;
    } else {
        int count = 1;
        for (const auto& addr : connected_addresses_) {
            std::cout << " [" << count++ << "] IP/Port -> " << addr << " [BIRIKDIRILDI]" << std::endl;
        }
        std::cout << ">>> Jemi: " << connected_addresses_.size() << " düwün birikdirildi." << std::endl;
    }
    std::cout << "--------------------------------\n" << std::endl;
}

void P2PNode::connect_to_peer(const std::string &host, const std::string &port) {
    auto socket = std::make_shared<tcp::socket>(io_context_);
    tcp::resolver resolver(io_context_);
    boost::asio::connect(*socket, resolver.resolve(host, port));
    std::string peer_info = host + ":" + port;
    if (connected_addresses_.find(peer_info) == connected_addresses_.end()) {
        peers_.push_back(socket);
        connected_addresses_.insert(peer_info);
        std::cout << "[P2P]: Düwüne birikdirildi " << peer_info << "." << std::endl;
        do_read(socket);
    }
}

void P2PNode::broadcast_message(const std::string &message) {
    for (auto& socket : peers_) {
        boost::asio::write(*socket, boost::asio::buffer(message + "\n"));
    }
    std::cout << "[P2P]: Täze blok tora ýaýradyldy." << std::endl;
}