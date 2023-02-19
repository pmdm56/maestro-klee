#include "../pch.hpp"

template<typename T>
void print_arg(T&& arg) {
    std::cout << arg;
}

template<typename T, typename... Args>
void print_arg(T&& arg, Args&&... args) {
    std::cout << arg;
    print_arg(std::forward<Args>(args)...);
}

template<typename... Args>
void info(Args&&... args) {
    std::cout << "[INFO] ";
    print_arg(std::forward<Args>(args)...);
    std::cout << std::endl;
}

template<typename... Args>
void danger(Args&&... args) {
    std::cout << "[ERROR] ";
    print_arg(std::forward<Args>(args)...);
    std::cout << std::endl;
    std::exit(EXIT_FAILURE);
}

template<typename... Args>
void debug(Args&&... args) {
    std::cout << "[DEBUG] ";
    print_arg(std::forward<Args>(args)...);
    std::cout << std::endl;
}

template<typename... Args>
void success(Args&&... args) {
    std::cout << "[SUCCESS] ";
    print_arg(std::forward<Args>(args)...);
    std::cout << std::endl;
}

template<typename... Args>
void warn(Args&&... args) {
    std::cout << "[WARNING] ";
    print_arg(std::forward<Args>(args)...);
    std::cout << std::endl;
}
