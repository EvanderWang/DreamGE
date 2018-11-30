//#include <iostream>
//#include <string>
//#include <memory>
//#include <thread>
//#include <set>
//#include <windows.h>

//#include "test.hpp"
//// test c++11
//void hello(){
//    std::cout << "hello thread!\n";
//}
//
//// test c++14
//auto getFunc(){
//    auto f = [=](double guess){
//        return guess*guess;
//    };
//    return f;
//}
//
//int main(/*int argc, char ** argv*/){
//    // test c++ std library.
//    std::cout << "Hello World" << std::endl;
//    std::string sa = "hello";
//    std::cout << sa << std::endl;
//
//    // test compile c++11
//    std::shared_ptr<int> p(new int[1]);
//    std::thread t1(hello);
//    std::shared_ptr<char> p1(new char[255]);
//    t1.join();
//
//    // test compile c++14
//    auto f = getFunc();
//    std::cout << f(3.0) << std::endl;
//    
//    // test c++17
//    std::set<std::string> myset;
//    if(auto[iter, success] = myset.insert("hello"); success)
//        std::cout << *iter << std::endl;
//    
//    void * pointer = (void *)&f;
//
//    VTest testObj(11);
//    int x = testObj.DoTest(22);
//
//    //MessageBoxA(nullptr, "Test2", "Test", MB_OK);
//    system("pause");
//
//    return 0;
//}


#include "Third/RxCpp/Rx/v2/src/rxcpp/rx.hpp"
// builder example

struct VBDivide
{
    enum VEReturn 
    {
        VR_DIV_ZERO = 1,
    };

    int Build( int & out , const int & in1, const int & in2)
    {
        if( in2 == 0 )
        {
            return VR_DIV_ZERO;
        }
        else
        {
            out = in1 / in2;
            return 0;
        }
    }
};

//interface VIxxx = VI< input<int>, output<int> >;
//
//class VManager
//{
//    VManager( const rxcpp::observable<int> & , const rxcpp::observable<VIxxx> & ){}
//
//    // base
//    void OnLink( rxcpp::observable<int> & pipein1 , rxcpp::observer<int> & pipeout1 )
//    {
//        pipein1.customOP<VBuilder>().subscribe(pipeout1);
//    }
//
//    void OnLink( rxcpp::observable<int> & pipein1, rxcpp::observable<VIxxx> & pipein2 , rxcpp::observer<int> & pipeout1 )
//    {
//        pipein1.customOP(pipein2).subscribe(pipeout1);
//    }
//
//    void OnLink(rxcpp::observable<VIxxx> & pipein2, rxcpp::observer<VIyyy> & pipeout2)
//    {
//        // use sth like vexport()
//    }
//};
//
//vexport( VManager , VIxxx );
//
//rxcpp::observable<VIxxx> inport = vinport( VManager );

class MyTestOp //: public rxcpp::operators::operator_base<int>
{
public:
    MyTestOp(){}
    ~MyTestOp(){}

    rxcpp::subscriber<int> operator() (rxcpp::subscriber<int> s) const {
        return rxcpp::make_subscriber<int>([s](const int & next) {
            s.on_next(std::move(next + 1));
        },  [&s](const std::exception_ptr & e) {
            s.on_error(e);
        },  [&s]() {
            s.on_completed();
        });
    }
};

int main()
{
    auto keys = rxcpp::observable<>::create<int>(
        [](rxcpp::subscriber<int> dest){
            for (;;) {
                int key = std::cin.get();
                dest.on_next(key);
            }
        }).publish();

    keys.lift<int>(MyTestOp()).subscribe([](int key){ 
        std::cout << key << std::endl;
    });

    keys.subscribe([](int key){ 
        std::cout << key << std::endl;
    });

    //keys.lift<int>([](rxcpp::subscriber<int> s) { 
    //    return rxcpp::make_subscriber<int>([s](const int & next) {
    //        s.on_next(std::move(next + 1));
    //    },  [&s](const std::exception_ptr & e) {
    //        s.on_error(e);
    //    },  [&s]() {
    //        s.on_completed();
    //    });
    //}).subscribe([](int key){ 
    //    std::cout << key << std::endl;
    //});

    // run the loop in create
    keys.connect();

    return 0;
}