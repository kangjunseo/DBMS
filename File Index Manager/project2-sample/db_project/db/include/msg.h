#ifndef __DB_MSG_H_
#define __DB_MSG_H_

#include <iostream>

#ifdef DBG_PRINT
template<typename ...Args>
void MSG(Args && ...args)
{
	(std::cerr << ... << args);
}
#else
template<typename ...Args>
void MSG(Args && ...args) {};
#endif /* DBG_PRINT */

#endif /* DB_MSG_H */
