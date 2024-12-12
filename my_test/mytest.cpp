#include <iostream>
#include <bitset>
#include <tuple>

std::tuple<int, double> get_values() {
    int x = 10;
    double y = 20.5;
    return std::make_tuple(x, y);  // 返回一个包含两个值的元组
}

int main() {
    auto [a, b] = get_values();  // 解构绑定，提取返回的元组值
    std::cout << "a: " << a << ", b: " << b << std::endl;
    return 0;
}
