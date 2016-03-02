#include <time.h>
#include <stdio.h>
#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/uio.h>
#include "jlog.h"
#define LOGGER_DIR	"/home/joison/joison-log"
#define LOGGER_LOG_MAIN "main.log"
#define LOGGER_LOG_CONNECTION	"conn.log"
#define LOGGER_LOG_PROCESS "process.log"
#define LOGGER_LOG_EVENTS "events.log"
#define LOG_BUF_SIZE	1024

#ifdef LOG_TO_STDOUT
/*debug version write log info to stdout*/
#define log_writev(fd, vec, nr) writev(STDOUT_FILENO, vec, nr)
#define log_open(filename, mode) open(filename, mode)
#define log_close(fd) close(STDOUT_FILENO)
#else
#define log_writev(fd, vec, nr) writev(fd, vec, nr)
#define log_open(filename, mode) open(filename, mode)
#define log_close(fd) close(fd)
#endif


typedef enum{
	LOG_ID_MAIN,
	LOG_ID_CONNECTION,
	LOG_ID_PROCESS,
	LOG_ID_EVENTS,
	LOG_ID_MAX
}log_id_t;

static int __write_to_log_init(log_id_t, struct iovec* vec, size_t nr);
static int (*write_to_log)(log_id_t,struct iovec* vec, size_t nr) = 
__write_to_log_init;

#ifdef HAVE_PTHREADS
static pthread_mutex_t log_init_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

static int log_fds[(int)LOG_ID_MAX] = {-1,-1,-1,-1};

#if 0
static enum{
	KLogUninitialized,KLogNotAvailable,KLogAvailable
} g_log_status = KLogUninitialized;
int __android_log_dev_available(void)
{
	if(g_log_status == KLogUninitialized){
		if(access(LOGO_FILE,W_OK)==0){
			g_log_status = KLogAvailable;
		}else{
			g_log_status = KLogNotAvailable;
		}
	}
	return (g_log_status == KLogAvailable);
}
#endif //marked the logfile vailable capbility testing

static int __write_to_log_null(log_id_t log_id, struct iovec* piovec, size_t nr)
{
#ifdef LOG_TO_STDOUT
	writev(STDOUT_FILENO, piovec, nr);
	return 0;
#else
	return -1;
#endif
}

static int __write_to_log_kernel(log_id_t log_id, struct iovec* vec, size_t nr)
{
	ssize_t ret;
	int log_fd;
	
	if((int)log_id<(int)LOG_ID_MAX){
		log_fd = log_fds[(int)log_id];
	}else{
		return EBADF;
	}
	
	do {
		ret = log_writev(log_fd, vec, nr);
	} while(ret < 0 && errno == EINTR);
	
	return ret;
}

static int __write_to_log_init(log_id_t log_id, struct iovec *vec, size_t nr)
{
#ifdef HAVE_PTHREADS
	pthread_mutex_lock(&log_init_lock);
#endif 
	if(write_to_log = __write_to_log_init){
#define OPEN_AND_ASSIGN_FC(name) log_fds[LOG_ID_##name] = \
	log_open(LOGGER_DIR"/"LOGGER_LOG_##name, O_WRONLY | O_CREAT)
		OPEN_AND_ASSIGN_FC(MAIN);
		OPEN_AND_ASSIGN_FC(CONNECTION);
		OPEN_AND_ASSIGN_FC(PROCESS);
		OPEN_AND_ASSIGN_FC(EVENTS);

		write_to_log = __write_to_log_kernel;

		if(log_fds[LOG_ID_MAIN] < 0 || log_fds[LOG_ID_CONNECTION] < 0|| 
		log_fds[LOG_ID_PROCESS] < 0 || log_fds[LOG_ID_EVENTS] < 0){
			log_close(log_fds[LOG_ID_MAIN]);
			log_close(log_fds[LOG_ID_CONNECTION]);
			log_close(log_fds[LOG_ID_PROCESS]);
			log_close(log_fds[LOG_ID_EVENTS]);

			write_to_log = __write_to_log_null;
		}
	}
#ifdef HAVE_PTHREADS
	pthread_mutex_unlock(&log_init_lock);
#endif
	return write_to_log(log_id, vec, nr);
}

int __android_log_write(int prio, const char *tag, const char *msg)
{
	struct iovec vec[3];
	log_id_t log_id = LOG_ID_MAIN;

	if (!tag)
		tag = "";

	/* XXX: This needs to go! */
	if (!strcmp(tag, "TCP") ||
			!strcmp(tag, "UDP") ||
			!strcmp(tag, "RS232") ||
			!strcmp(tag, "RS485") ||
			!strcmp(tag, "BT") ||
			!strcmp(tag, "USB") ){
		log_id = LOG_ID_CONNECTION;
	}

	if (!strcmp(tag, "ID_PROCESS")){
		log_id = LOG_ID_PROCESS;
	}

	vec[0].iov_base   = (unsigned char *) &prio;
	vec[0].iov_len    = 1;
	vec[1].iov_base   = (void *) tag;
	vec[1].iov_len    = strlen(tag) + 1;
	vec[2].iov_base   = (void *) msg;
	vec[2].iov_len    = strlen(msg) + 1;

	return write_to_log(log_id, vec, 3);
}

int __android_log_vprint(int prio, const char *tag, const char *fmt, va_list ap)
{
	char buf[LOG_BUF_SIZE];    

	vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);

	return __android_log_write(prio, tag, buf);
}

int __android_log_print(int prio, const char *tag, const char *fmt, ...)
{
	va_list ap;
	char buf[LOG_BUF_SIZE];    

	va_start(ap, fmt);
	vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
	va_end(ap);

	return __android_log_write(prio, tag, buf);
}

void __android_log_assert(const char *cond, const char *tag,
		const char *fmt, ...)
{
	va_list ap;
	char buf[LOG_BUF_SIZE];    

	va_start(ap, fmt);
	vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
	va_end(ap);

	__android_log_write(ANDROID_LOG_FATAL, tag, buf);

	__builtin_trap(); /* trap so we have a chance to debug the situation */
}

int __android_log_bwrite(int32_t tag, const void *payload, size_t len)
{
	struct iovec vec[2];

	vec[0].iov_base = &tag;
	vec[0].iov_len = sizeof(tag);
	vec[1].iov_base = (void*)payload;
	vec[1].iov_len = len;

	return write_to_log(LOG_ID_EVENTS, vec, 2);
}

/*
 * Like __android_log_bwrite, but takes the type as well.  Doesn't work
 * for the general case where we're generating lists of stuff, but very
 * handy if we just want to dump an integer into the log.
 */
int __android_log_btwrite(int32_t tag, char type, const void *payload,
		size_t len)
{
	struct iovec vec[3];

	vec[0].iov_base = &tag;
	vec[0].iov_len = sizeof(tag);
	vec[1].iov_base = &type;
	vec[1].iov_len = sizeof(type);
	vec[2].iov_base = (void*)payload;
	vec[2].iov_len = len;

	return write_to_log(LOG_ID_EVENTS, vec, 3);
}



