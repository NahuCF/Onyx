#include "SE.H"

namespace SE {

	__declspec(dllimport) void Print();

}

int main()
{
	SE::Print();
}