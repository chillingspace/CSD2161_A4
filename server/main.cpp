#include "server.h"


#define VERBOSE_LOGGING
#define JS_DEBUG

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif


int main()
{
	Server& s = Server::getInstance();
	return s.init();
}
