#include "blockchain.h"
#include "database.h"
#include "smartContract.h"
#include "subscriber_repository.h"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace pt = boost::property_tree;
using tcp = asio::ip::tcp;

namespace {
std::string jsonEscape(const std::string &value) {
    std::ostringstream out;
    for (const unsigned char c : value) {
        switch (c) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\b': out << "\\b"; break;
        case '\f': out << "\\f"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (c < 0x20) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
            else out << c;
        }
    }
    return out.str();
}

std::string quote(const std::string &value) { return "\"" + jsonEscape(value) + "\""; }

std::string statusFor(const Subscriber &subscriber) {
    if (subscriber.isServiceActive(ServiceType::INTERNET) || subscriber.isServiceActive(ServiceType::IPTV) ||
        subscriber.isServiceActive(ServiceType::PHONE)) return "Active";
    return subscriber.balance > 0.01 ? "Pending" : "Inactive";
}

std::string internetFor(const Subscriber &subscriber) {
    std::string result = subscriber.internet.speed + " Mbit/s • ";
    return result + (subscriber.isServiceActive(ServiceType::INTERNET)
        ? std::to_string(subscriber.remainingDays(subscriber.internetExpiry)) + " days left" : "Inactive");
}

std::string iptvFor(const Subscriber &subscriber) {
    std::string result = std::to_string(subscriber.iptv.tvCount) + " channels • ";
    return result + (subscriber.isServiceActive(ServiceType::IPTV)
        ? std::to_string(subscriber.remainingDays(subscriber.iptvExpiry)) + " days left" : "Inactive");
}

std::string phoneFor(const Subscriber &subscriber) {
    return subscriber.isServiceActive(ServiceType::PHONE)
        ? "Active • " + std::to_string(subscriber.remainingDays(subscriber.phoneExpiry)) + " days left" : "Inactive";
}

std::optional<std::string> field(const std::string &data, const std::string &key) {
    const auto position = data.find(key);
    if (position == std::string::npos) return std::nullopt;
    const auto start = position + key.size();
    const auto end = data.find(',', start);
    return data.substr(start, end == std::string::npos ? std::string::npos : end - start);
}

std::optional<double> payment(const std::string &data) {
    const auto start = data.find("Payment: ");
    if (start == std::string::npos) return std::nullopt;
    const auto end = data.find(" TMT", start);
    if (end == std::string::npos) return std::nullopt;
    try { return std::stod(data.substr(start + 9, end - start - 9)); } catch (...) { return std::nullopt; }
}

std::string formatTime(std::time_t value) {
    std::tm time{};
#ifdef _WIN32
    localtime_s(&time, &value);
#else
    localtime_r(&value, &time);
#endif
    std::ostringstream out;
    out << std::put_time(&time, "%Y-%m-%d %H:%M");
    return out.str();
}

std::string subscriberJson(const Subscriber &s) {
    std::ostringstream out;
    out << "{\"name\":" << quote(s.name)
        << ",\"internet\":" << quote(internetFor(s))
        << ",\"iptv\":" << quote(iptvFor(s))
        << ",\"phone\":" << quote(phoneFor(s))
        << ",\"balance\":" << s.balance
        << ",\"status\":" << quote(statusFor(s)) << '}';
    return out.str();
}

std::string subscribersJson() {
    const auto subscribers = SubscriberRepository::list();
    std::ostringstream out;
    out << '[';
    for (size_t i = 0; i < subscribers.size(); ++i) {
        const auto &s = subscribers[i];
        if (i) out << ',';
        out << subscriberJson(s);
    }
    return out.str() + ']';
}

std::string blocksJson(const Blockchain &chain) {
    auto blocks = chain.getAllBlocks();
    std::reverse(blocks.begin(), blocks.end());
    std::ostringstream out;
    out << '[';
    bool first = true;
    for (const auto &block : blocks) {
        if (block.sData == "Genesis Block") continue;
        const auto subscriber = field(block.sData, "Subscriber: ");
        const auto service = field(block.sData, "Service: ");
        const auto amount = payment(block.sData);
        if (!first) out << ',';
        first = false;
        out << "{\"index\":" << block.nIndex
            << ",\"time\":" << quote(formatTime(block.tTime))
            << ",\"subscriber\":" << quote(subscriber.value_or("—"))
            << ",\"service\":" << quote(service.value_or("Ledger event"))
            << ",\"amount\":" << amount.value_or(0)
            << ",\"hash\":" << quote(block.sPrevHash) << '}';
    }
    return out.str() + ']';
}

pt::ptree parseBody(const std::string &body) {
    std::istringstream stream(body);
    pt::ptree tree;
    pt::read_json(stream, tree);
    return tree;
}

ServiceType serviceType(const std::string &value) {
    if (value == "Internet") return ServiceType::INTERNET;
    if (value == "IPTV") return ServiceType::IPTV;
    if (value == "Phone") return ServiceType::PHONE;
    throw std::invalid_argument("Service must be Internet, IPTV, or Phone.");
}

std::string mimeType(const std::filesystem::path &path) {
    const auto extension = path.extension().string();
    if (extension == ".html") return "text/html; charset=utf-8";
    if (extension == ".css") return "text/css; charset=utf-8";
    if (extension == ".js") return "text/javascript; charset=utf-8";
    if (extension == ".svg") return "image/svg+xml";
    if (extension == ".png") return "image/png";
    return "application/octet-stream";
}

template<class Body, class Allocator>
http::response<http::string_body> route(const http::request<Body, http::basic_fields<Allocator>> &request,
                                        Blockchain &chain, const std::filesystem::path &webRoot) {
    const std::string target(request.target());
    auto response = [&](http::status status, std::string body, std::string contentType = "application/json; charset=utf-8") {
        http::response<http::string_body> result{status, request.version()};
        result.set(http::field::server, "telekom-web");
        result.set(http::field::content_type, contentType);
        result.set(http::field::cache_control, "no-store");
        result.keep_alive(request.keep_alive());
        result.body() = std::move(body);
        result.prepare_payload();
        return result;
    };

    try {
        if (request.method() == http::verb::get && target == "/api/subscribers") return response(http::status::ok, subscribersJson());
        if (request.method() == http::verb::get && target == "/api/blocks") return response(http::status::ok, blocksJson(chain));
        if (request.method() == http::verb::get && target == "/api/dashboard") {
            return response(http::status::ok, "{\"subscribers\":" + std::to_string(SubscriberRepository::count()) +
                ",\"blocks\":" + std::to_string(chain.getAllBlocks().size()) + ",\"database\":\"Online\"}");
        }
        if (request.method() == http::verb::post && target == "/api/subscribers") {
            const pt::ptree body = parseBody(request.body());
            const std::string name = body.get<std::string>("name");
            if (name.empty() || name.size() > 80) throw std::invalid_argument("Subscriber name must contain 1–80 characters.");
            Subscriber subscriber(name, false);
            if (!SmartContract::createSubscriberInDB(subscriber))
                return response(http::status::conflict, "{\"message\":\"A subscriber with that name already exists.\"}");
            return response(http::status::created, subscriberJson(subscriber));
        }
        if (request.method() == http::verb::post && target == "/api/payments") {
            const pt::ptree body = parseBody(request.body());
            const std::string name = body.get<std::string>("subscriber");
            const double amount = body.get<double>("amount");
            const ServiceType type = serviceType(body.get<std::string>("service"));
            Subscriber before(name, false);
            if (!SmartContract::getSubscriberFromDB(name, before))
                return response(http::status::not_found, "{\"message\":\"Subscriber not found.\"}");
            Subscriber after = before;
            std::string validation;
            const std::time_t processedAt = std::time(nullptr);
            if (!SmartContract::processService(after, amount, type, &validation, processedAt))
                return response(http::status::unprocessable_entity, "{\"message\":" + quote(validation) + "}");
            const std::string data = SmartContract::getPaymentData(before, after, type, amount, processedAt);
            chain.AddBlockWithMutation(Block(0, data), [&after](pqxx::work &transaction) {
                SmartContract::updateSubscriberInDB(after, transaction);
            });
            return response(http::status::created, "{\"ok\":true}");
        }
        if (request.method() == http::verb::post && target == "/api/blocks/audit") {
            const bool valid = chain.fullAudit();
            return response(valid ? http::status::ok : http::status::conflict,
                            valid ? "{\"valid\":true}" : "{\"valid\":false,\"message\":\"Chain integrity check failed.\"}");
        }
        if (target.rfind("/api/", 0) == 0) return response(http::status::not_found, "{\"message\":\"API endpoint not found.\"}");
        if (request.method() != http::verb::get && request.method() != http::verb::head)
            return response(http::status::method_not_allowed, "Method not allowed", "text/plain; charset=utf-8");

        std::string relative = target == "/" ? "index.html" : target.substr(1);
        if (relative.find("..") != std::string::npos) return response(http::status::bad_request, "Invalid path", "text/plain; charset=utf-8");
        std::filesystem::path path = webRoot / relative;
        if (!std::filesystem::is_regular_file(path)) path = webRoot / "index.html";
        std::ifstream file(path, std::ios::binary);
        if (!file) return response(http::status::not_found, "Not found", "text/plain; charset=utf-8");
        std::ostringstream body;
        body << file.rdbuf();
        return response(http::status::ok, body.str(), mimeType(path));
    } catch (const pt::json_parser_error &) {
        return response(http::status::bad_request, "{\"message\":\"Request body must be valid JSON.\"}");
    } catch (const std::invalid_argument &error) {
        return response(http::status::bad_request, "{\"message\":" + quote(error.what()) + "}");
    } catch (const std::exception &error) {
        std::cerr << "[WEB API] " << error.what() << '\n';
        return response(http::status::internal_server_error, "{\"message\":" + quote(error.what()) + "}");
    }
}
}

int main(int argc, char **argv) {
    try {
        const unsigned short port = static_cast<unsigned short>(argc > 1 ? std::stoi(argv[1]) : 8081);
        const std::filesystem::path webRoot = argc > 2 ? argv[2] : TELEKOM_WEB_ROOT;
        DatabaseManager::initTables();
        Blockchain chain;
        asio::io_context context{1};
        tcp::acceptor acceptor{context, {tcp::v4(), port}};
        std::cout << "TürkmenTelekom web console: http://localhost:" << port << '\n';
        std::cout << "Serving files from: " << webRoot << '\n';
        for (;;) {
            tcp::socket socket{context};
            acceptor.accept(socket);
            beast::flat_buffer buffer;
            http::request<http::string_body> request;
            http::read(socket, buffer, request);
            auto response = route(request, chain, webRoot);
            http::write(socket, response);
            beast::error_code error;
            socket.shutdown(tcp::socket::shutdown_send, error);
        }
    } catch (const std::exception &error) {
        std::cerr << "Could not start web server: " << error.what() << '\n';
        return 1;
    }
}
