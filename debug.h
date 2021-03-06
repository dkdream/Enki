#if !defined(_ea_debug_h_)
#define _ea_debug_h_
/***************************
 **
 ** Purpose
 **   -- <desc>
 **/
#include <stdbool.h>

extern unsigned int ea_global_debug;
extern unsigned int ea_global_trace;
extern void debug_Message(const char *filename, unsigned int linenum, bool newline, const char *format, ...) __attribute__ ((format (printf, 4, 5)));
extern void error_Message(const char *filename, unsigned int linenum, const char *format, ...) __attribute__ ((noreturn, format (printf, 3, 4)));
extern void fatal(const char *reason, ...) __attribute__ ((noreturn, format (printf, 1, 2)));
extern void boom();

#undef VM_ERROR
#undef VM_DEBUG
#undef VM_ON_DEBUG
#undef VM_ERROR

#define BOOM() boom()
#define VM_ERROR(args...) error_Message(__FILE__,  __LINE__, args)
#define VM_TRACE(args...) debug_Message(__FILE__,  __LINE__, true, args)

#ifdef debug_THIS
#define VM_DEBUG(level, args...) ({ typeof (level) hold__ = (level); if (hold__ <= ea_global_debug) debug_Message(__FILE__,  __LINE__, true, args); })
#define VM_ON_DEBUG(level, arg) ({ typeof (level) hold__ = (level); if (hold__ <= ea_global_debug) arg; })
#else
#define VM_DEBUG(level, args...)    ({ 0; })
#define VM_ON_DEBUG(level, args...) ({ 0; })
#endif

#undef debug_THIS

/***************************
 ** end of file
 **************************/
#endif
