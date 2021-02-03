#include "SE.H"
#include <iostream>

namespace se {

	__declspec(dllimport) void Print();

}

int main()
{
	se::Print();
	std::cin.get();
}