#include <iostream>
#include "./helper/ConsoleApp.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc > 2) {
        cerr << "Использование: " << argv[0] << " [путь к schema.json]\n";
        return 1;
    }

    ConsoleApp app;
    return app.run(argc == 2 ? argv[1] : "schema.json");
}
