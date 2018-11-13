#pragma once

class VTest
{
public:
    VTest(int add);
    ~VTest();

    int DoTest(int from);

private:
    int m_add;
};