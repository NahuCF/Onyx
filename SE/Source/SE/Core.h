#pragma once

#ifdef SE_PLATFORM_WINDOWS
	#ifdef SE_BUILD_DLL
		#define SE_API __dclspect(dllexport)
	#else
		#define SE_API __dclspect(dllimport)
	#endif
#endif