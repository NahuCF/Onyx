#pragma once

#ifdef _DEBUG
	#ifdef __cplusplus 
		#define check(number) { if(number) { std::cout << "Yes" << std::endl; } }
	#endif
#endif