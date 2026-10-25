#include "sqlite3.h"
#include <iostream>
#include <string>
#include <filesystem>

int main () {
        const std::string path = "shop.db";
        sqlite3* db = nullptr;
        std::cout << "loading up...." << std::endl;
        std::cout << "this might take a while..." << std::endl;
        std::cout << "\n";
        if (std::filesystem::exists(path)) {
            if (sqlite3_open("shop.db", &db) == SQLITE_OK) {

                std::cout << "loading existing database..." << std::endl;
            }
        }
        else {
            std::cout << ".db not found: "; std::cout << sqlite3_errmsg(db) << "\n";
         }
    std::cout << "\nPress Enter to exit...";
    std::cin.ignore(10000, '\n');   // discard leftover input from earlier cin >>
    std::cin.get();
}