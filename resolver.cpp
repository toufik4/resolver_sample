// Resolution dns asynchrone

// Puprose:
// Exemple simple de résolution dns asynchrone
// Illustre cas ecole de résolution à travers un boost::asio::io
// Utilisation d'un seul io(io_context) pour toutes les résolutions
// Il faut un io.restart() pour accepter de nouvelles résolutions
// [NEW] work_guard pour éviter que io.run() ne sorte trop tôt
// [NEW] Encapsulation dans une seule classe Resolver

// Author : Toufik ABDELMOUMENE

// File: resolver.cpp


#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;

void on_resolve(const boost::system::error_code& ec,
                tcp::resolver::results_type results)
{
    if (ec) {
        std::cerr << "Erreur de résolution: " << ec.message() << "\n";
        return;
    }

    std::cout << "Solve OK:\n";
    for (const auto& entry : results) {
        std::cout << entry.endpoint().address().to_string() << "\n";
    }
}

int main() {
#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
#endif  
    boost::asio::io_context io;       
    tcp::resolver resolver(io);  

    // Pour empêcher io.run() de s’arrêter immédiatement.
     auto work_guard = boost::asio::make_work_guard(io);

 while(1)
    {
    try {

        std::string host = "www.google.com";
        std::string port = "80";
        std::cout << "🔹 Enter host" << std::endl;
        std::cin >> host;

        if(host == "000"){
            work_guard.reset(); // indique que io peut s’arrêter.
            break;
        }
        auto lamb = [host](const boost::system::error_code& ec,
               tcp::resolver::results_type results) {
            if (ec) {
                std::cerr << "Erreur pour " << host << ": " << ec.message() << "\n";
                return;
            }

            std::cout << "✅ Solving " << host << " OK:\n";
            for (const auto& entry : results) {
                std::cout << entry.endpoint().address().to_string() << "\n";
            }
        };
        // Démarre la résolution asynchrone
        resolver.async_resolve(host, port, lamb);
        

            // 🔧 Redémarrer le contexte pour accepter de nouvelles opérations
            io.restart();

            // 🔁 Relancer le traitement des événements asynchrones
            io.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    }
    return 0;
}
