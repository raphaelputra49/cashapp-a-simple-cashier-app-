#include "sqlite3.h"
#include <iostream>
#include <string>
#include <filesystem>
#include "ftxui/component/app.hpp"             // for App
#include "ftxui/component/captured_mouse.hpp"  // for ftxui
#include "ftxui/component/component.hpp"  // for Radiobox, Renderer, Tab, Toggle, Vertical
#include "ftxui/component/component_base.hpp"  // for ComponentBase
#include "ftxui/dom/elements.hpp"
#include <ftxui/component/screen_interactive.hpp>
#include <memory>
#include <vector>

bool databaseready;

struct Product {
    int id;
    std::string name;
    double price;
};

struct CartItem {
    Product item;
    int quantity;
};

void makedb(sqlite3* db) {
    std::string sql=
    "CREATE TABLE IF NOT EXISTS list("
    "ID INTEGER PRIMARY KEY NOT NULL,"
    "NAME TEXT NOT NULL,"
    "PRICE REAL NOT NULL)";

    char* messageError = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &messageError);

    if (rc != SQLITE_OK) {
        std::cerr << "Error Create Table" << std::endl;
        sqlite3_free(messageError);
    }

    else {
        std::cout << ".db created Successfully" << std::endl;

        sqlite3_exec(db, "INSERT INTO list (ID, NAME, PRICE) "
                     "VALUES (001, 'KOPI GULA AREN  ', 16000)",
                      nullptr, nullptr ,nullptr);
        sqlite3_exec(db, "INSERT INTO list (ID, NAME, PRICE) "
                   "VALUES (002, 'KOPI ECSPRESO ASEM  ', 10777)",
                    nullptr, nullptr ,nullptr);
        databaseready = true;
    }

}


static int callback(void* data, int argc, char** argv, char** azColName)
{
    int i;
    fprintf(stderr, "%s: ", (const char*)data);

    for (i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }

    printf("\n");
    return 0;
}


std::vector<Product> loadproduct(sqlite3* db) {
    std::vector<Product> products;
    sqlite3_stmt* stmt = nullptr;

    const char sql[] =
    "SELECT ID, NAME, PRICE FROM list ORDER BY ID";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Database error: "
        << sqlite3_errmsg(db) << '\n';
        sqlite3_close(db);
        return products;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Product p;

        p.id = sqlite3_column_int(stmt, 0);

        auto* name = sqlite3_column_text(stmt, 1);
        std::string productName;

        if (name != nullptr) {
            p.name = reinterpret_cast<const char*>(name);
        } else {
            p.name = "";
        }

        p.price = sqlite3_column_double(stmt, 2);

        std::cout << "ID: " << p.id
          << " | Name: " << p.name
          << " | Price: " << p.price << '\n';

        products.push_back(p);
    }
    sqlite3_finalize(stmt);
    return products;
}

void loadTUI(const std::vector<Product>& products) {
        using namespace ftxui;
         auto screen = ScreenInteractive::TerminalOutput();

        std::vector<std::string> tab_values{
            "MAIN MENU",
                "HISTORY",
            "QUIT"
        };
        int tab_selected = 0;
        auto tab_toggle = Toggle(&tab_values, &tab_selected);

    std::vector<std::string> product_entries;

    for (const auto& p : products) {
        product_entries.push_back(
            p.name + " - Rp " + std::to_string(static_cast<int>(p.price))
        );
    }

    int product_selected = 0;

    std::vector<CartItem> cart_items;

    auto checkout_button = Button("CHECKOUT", [&] {
    cart_items.clear();
});

    MenuOption product_options = MenuOption::Vertical();

    product_options.on_enter = [&] {
        if (products.empty()) return;

        const Product& selected = products[product_selected];

        for (auto& item : cart_items) {
            if (item.item.id == selected.id) {
                item.quantity++;
                return;
            }
        }

        cart_items.push_back({selected, 1});
    };

    auto product_menu = Menu(
        &product_entries,
        &product_selected,
        product_options
    );

    std::vector<std::string> tab_2_entries{};
    int tab_2_selected = 0;

    auto tab_container = Container::Tab(
        {
            product_menu,
            Radiobox(&tab_2_entries, &tab_2_selected),
        },
        &tab_selected
    );
         auto carts = Container::Vertical({
                 checkout_button
         });

          auto layout = Container::Horizontal({
             tab_container,
             carts
          });

             auto container = Container::Vertical({
            tab_toggle,
            layout
        });

    auto renderer = Renderer(container, [&] {
        Elements cart_lines;

        if (cart_items.empty()) {
            cart_lines.push_back(text("Cart is empty"));
        }

        int total = 0;

        for (const auto& item : cart_items) {
            int subtotal = static_cast<int>(
                item.item.price * item.quantity
            );

            total += subtotal;

            cart_lines.push_back(text(
                item.item.name + " x" +
                std::to_string(item.quantity)
            ));

            cart_lines.push_back(text(
                "Rp " + std::to_string(subtotal)
            ));
        }

        cart_lines.push_back(separator());

        return vbox({
            tab_toggle->Render(),
            separator(),
            hbox({
                tab_container->Render()
                    | size(WIDTH, GREATER_THAN, 25)
                    | flex,
                vbox({
                text("CART") | bold,
                separator(),
                vbox(std::move(cart_lines)),
                separator(),
                hbox({
                text("TOTAL: Rp " + std::to_string(total)) | bold,
                checkout_button->Render()
     })
 }) | size(WIDTH, GREATER_THAN, 30) | border
            }) | flex
        }) | border;
    });
        screen.Loop(renderer);
    }



int main () {
    const std::string path = "shop.db";
    sqlite3* db = nullptr;

    bool existed = std::filesystem::exists(path);

    std::cout << "loading up...." << std::endl;
    std::cout << "this might take a while..." << std::endl;

        if (sqlite3_open("shop.db", &db) != SQLITE_OK) {

            std::cout << "Failed to open:" <<  sqlite3_errmsg(db) << std::endl;
            std::cout << "wants to overide with the default list? [Y/n]";
            char check;
            std::cin >> check;
            if (check == 'Y' || check == 'y') {
                makedb(db);
            }
        }
        else if (existed) {
            std::cout << "loading database..."<< std::endl;
            databaseready = true;
         }
        else {
            std::cout << "No database found, creating a new one.\n";
            makedb(db);
        }

    std::vector<Product> products = loadproduct(db);
    loadTUI(products);
    std::cout << products[0].name << '\n';
    std::cout << products[0].price << '\n';
        std::cout << "\nPress Enter to exit...";
        std::cin.ignore(10000, '\n');
        std::cin.get();
         sqlite3_close(db);

    }
