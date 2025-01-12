#ifndef DEBOG_H_
#define DEBOG_H_

void
debog_init(void);


#if defined(DEBOG_IMPLEMENTATION)

#define B_STACKTRACE_IMPL
#include "third-party-thing/b_stacktrace.h"

#include <signal.h>


void
debog_handler(int signal)
{
    printf("signal: %d\n", signal);
    puts(b_stacktrace_get_string());
    exit(1);
}


void
debog_init(void)
{
    signal(SIGSEGV, debog_handler);
    signal(SIGABRT, debog_handler);
}

#endif // defined(DEBOG_IMPLEMENTATION)


#endif // DEBOG_H_
