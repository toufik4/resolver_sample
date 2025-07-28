// Resolution dns asynchrone

// Puprose:
// Exemple simple de résolution dns asynchrone
// Illustre cas ecole de résolution à travers un boost::asio::io
// Utilisation d'un seul io(io_context) pour toutes les résolutions
// Il faut un io.restart() pour accepter de nouvelles résolutions
// work_guard pour éviter que io.run() ne sorte trop tôt
// Encapsulation dans une seule classe Resolver
// runner_ [le thread qui execute  io.run()] au niveau de Resolver
// work_guard au niveau de Resolver
// Ajout d'un timeout de résolution DNS
// [NEW] Ajout class Request pour encapsuler les requêtes DNS

// Author : Toufik ABDELMOUMENE

// File: resolver.cpp

#include <boost/asio.hpp>
#include <iostream>
#include <deque>
#include <mutex>
#include <memory>
#include <thread>
#include <algorithm>
#include <windows.h>

using boost::asio::ip::tcp;

class Request {
public:
    std::string host_;
    std::string service_;
    std::vector<std::string> ips_;

    Request(std::string host, std::string service = "http")
        : host_(std::move(host)), service_(std::move(service)) {}

    void add_ip(const std::string& ip) {
        ips_.push_back(ip);
    };

    friend std::ostream& operator<<(std::ostream& os, const Request& req) {
    os << "🌍 Host: " << req.host_ << "\n";
    //os << "🔌 Service: " << req.service_ << "\n";
    if (req.ips_.empty()) {
        os << "❗ Aucun résultat DNS.\n";
    } else {
        os << "✅ IPs:\n";
        for (const auto& ip : req.ips_) {
            os << "  -> " << ip << "\n";
        }
    }
    return os;
}

};

class Resolver : public std::enable_shared_from_this<Resolver>
{
public:
    Resolver() : resolver_(io_),
                 work_guard_(boost::asio::make_work_guard(io_)),
                 runner_([&]()
                         { this->run(); })
    {
    }

    ~Resolver()
    {
        if (runner_.joinable())
        {
            runner_.join();
        }
    }
    void run()
    {
        std::cout << "Runner ID: " << std::this_thread::get_id() << std::endl;
        io_.run();
        std::cout << "🧵 io_context terminé.\n";
    }

    void stop()
    {
        std::lock_guard<std::mutex> lock(stop_mutex_);
        if (!stopped_)
        {
            stopped_ = true;
            work_guard_.reset(); // Permet à io_context de sortir de run()
            io_.stop();
        }
    }

    void print()
    {

        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "🧵 Listing ...\n";
        for (auto host : hosts_)
        {
            std::cout << host.first << std::endl;
        }
    }
    void add_host(std::string host, std::string service = "http")
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "🔧 add_host: " << host << std::endl;
        hosts_.emplace_back(std::move(host), std::move(service));

        if (!hosts_.empty())
        {
            auto self = shared_from_this();
            boost::asio::post(resolver_.get_executor(),
                              [this, self]()
                              { resolve_next(); });
        }
    }

    void remove_host(const std::string &host)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "🗑 remove_host: " << host << std::endl;
        hosts_.erase(std::remove_if(hosts_.begin(), hosts_.end(),
                                    [&](auto const &hs)
                                    { return hs.first == host; }),
                     hosts_.end());
    }

private:
    void resolve_next()
    {
        std::string host, service;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (hosts_.empty())
                return;

            host = std::move(hosts_.front().first);
            service = std::move(hosts_.front().second);
            hosts_.pop_front();
        }

        auto self = shared_from_this();
        std::cout << "🌐 Résolution: " << host << std::endl;

        // Démarrer le timer avant la résolution
        timeout_timer_.expires_after(timeout_duration_);
        auto lam_wait = [this, self, host](const boost::system::error_code &ec)
        {
            if (!ec)
            {
                std::cout << "⏳ Timeout atteint pour " << host << ", on annule la résolution.\n";
                resolver_.cancel(); // Annule toutes les résolutions en cours
            }
        };
        timeout_timer_.async_wait(lam_wait);

        auto lam_solve = [this, self, host](const boost::system::error_code &ec,
                                            tcp::resolver::results_type results)
        {
            // Annuler le timer si la résolution est terminée
            timeout_timer_.cancel();

            if (ec == boost::asio::error::operation_aborted)
            {
                std::cerr << "⏳ Timeout atteint ou résolution annulée pour " << host << "\n";
            }
            else if (ec == boost::asio::error::host_not_found)
            {
                std::cerr << "❌ Hôte introuvable : " << host << " (erreur DNS immédiate)\n";
            }
            else if (ec)
            {
                std::cerr << "❌ Autre échec DNS pour " << host << ": " << ec.message() << "\n";
            }
            else
            {
                std::cout << "✅ Résultat DNS pour " << host << " :\n";
                for (auto const &r : results)
                    std::cout << "  -> " << r.endpoint() << "\n";
            }

            // Continuer avec le prochain host sans timer
            resolve_next();
        };
        resolver_.async_resolve(host, service, lam_solve);
    }


    boost::asio::io_context io_; // should be unique
    tcp::resolver resolver_;
    std::deque<std::pair<std::string, std::string>> hosts_;
    std::mutex mutex_;
    std::thread runner_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
    bool stopped_ = false;
    std::mutex stop_mutex_;

    // Timeout de résolution
    std::chrono::seconds timeout_duration_{2};     // Timeout configurable
    boost::asio::steady_timer timeout_timer_{io_}; // Timer lié à io_context
};

int main()
{
#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::cout << "main ID: " << std::this_thread::get_id() << std::endl;
    std::cout << "🚀 Lancement...\n";

    auto resolver = std::make_shared<Resolver>();

    while (true)
    {
        std::string host;
        std::cout << "🔹 Entrez un nom d'hôte (ou 'stop' pour quitter) : ";
        std::cin >> host;

        if (host == "stop")
        {
            resolver->stop();
            break;
        }

        if (host == "print")
        {
            resolver->print();
            continue;
        }

        resolver->add_host(host);
    }

    std::cout << "✅ Fin normale.\n";
    return 0;
}
