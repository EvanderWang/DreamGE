#include "test.hpp"

VTest::VTest(int add)
    :m_add(add)
{

}

VTest::~VTest()
{

}

int VTest::DoTest(int from)
{
    return from + m_add;
}