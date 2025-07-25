// Resolution dns asynchrone

// Puprose:
// Exemple simple de résolution dns asynchrone
// Illustre cas ecole de résolution à travers un boost::asio::io_context
//

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
 while(1)
    {
    try {
        boost::asio::io_context io_context;
        
        tcp::resolver resolver(io_context);
        std::string host = "www.google.com";
        std::string port = "80";
        std::cout << "🔹 Enter host" << std::endl;
        std::cin >> host;

        if(host == "000")
            break;
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
        

        // Lancement de la boucle d'événements
        io_context.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    }
    return 0;
}
