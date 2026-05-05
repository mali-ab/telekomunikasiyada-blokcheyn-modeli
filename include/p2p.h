#ifndef P2P_H
#define P2P_H

#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <sstream>
#include <set>
#include "blockchain.h"

using boost::asio::ip::tcp;

class P2PNode {
public:
    P2PNode(int port, Blockchain &bc);
    void start_node();
    void connect_to_peer(const std::string &host, const std::string &port);
    void broadcast_message(const std::string &message);
    void list_active_peers();

private:
    void do_accept();
    void do_read(std::shared_ptr<tcp::socket> socket);
    void handle_incoming_block(const std::string &message);

    int port_;
    Blockchain &blockchain_;
    boost::asio::io_context io_context_;
    tcp::acceptor acceptor_;
    std::vector<std::shared_ptr<tcp::socket>> peers_;
    std::set<std::string> connected_addresses_;
};

#endif