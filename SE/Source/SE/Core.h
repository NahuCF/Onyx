#pragma once

#ifdef _DEBUG
	#ifdef __cplusplus 
		#define STATIC_ASSERT(expr)\
		static_assert(expr, "Tumadre" #expr)
	#endif
#endif