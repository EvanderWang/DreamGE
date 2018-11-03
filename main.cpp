#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <set>

// test c++11
void hello(){
    std::cout << "hello thread!\n";
}

// test c++14
auto getFunc(){
    auto f = [=](double guess){
        return guess*guess;
    };
    return f;
}

int main(int argc, char ** argv){
    // test c++ std library.
    std::cout << "Hello World" << std::endl;
    std::string sa = "hello";
    std::cout << sa << std::endl;

    // test compile c++11
    std::shared_ptr<int> p(new int[1]);
    std::thread t1(hello);
    std::shared_ptr<char> p1(new char[255]);
    t1.join();

    // test compile c++14
    auto f = getFunc();
    std::cout << f(2.0) << std::endl;
    
    // test c++17
    std::set<std::string> myset;
    if(auto[iter, success] = myset.insert("hello"); success)
        std::cout << *iter << std::endl;
        return 0;
}