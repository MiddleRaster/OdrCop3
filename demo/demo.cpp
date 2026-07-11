
#include <iostream>

struct SomeStruct
{
    int aDataMember = 0;
    SomeStruct() {}
   ~SomeStruct() {}
    int SomeFunction(int a, bool b) { return b ? 0 : a + aDataMember; }
    int GetDataMember() const { return aDataMember; }
};

int main()
{
    std::cout << "Hello World!\n";
    return SomeStruct().aDataMember;
}
