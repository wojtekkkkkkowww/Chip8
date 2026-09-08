#include <iostream>
#include <string_view>

void skibidi(auto sigma1, auto sigma2) {
    sigma1();
    sigma2();
}

int main() {
    std::string_view sigma{"sigma\n"};
    skibidi([]() {}, [sigma]() { std::cout << sigma; });

    return 0;
}
