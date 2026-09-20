#include <iostream>
#include <string_view>

void runCallbacks(auto firstCallback, auto secondCallback) {
    firstCallback();
    secondCallback();
}

int main() {
    std::string_view message{"sigma\n"};
    runCallbacks([]() {}, [message]() { std::cout << message; });

    return 0;
}
