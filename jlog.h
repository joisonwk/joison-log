#ifndef __JLOG_H__
#define __JLOG_H__
/*
 * Android log priority values, in ascending priority order.
 */
typedef enum android_LogPriority {
	ANDROID_LOG_UNKNOWN = 0,
	ANDROID_LOG_DEFAULT,    /* only for SetMinPriority() */
	ANDROID_LOG_VERBOSE,
	ANDROID_LOG_DEBUG,
	ANDROID_LOG_INFO,
	ANDROID_LOG_WARN,
	ANDROID_LOG_ERROR,
	ANDROID_LOG_FATAL,
	ANDROID_LOG_SILENT,     /* only for SetMinPriority(); must be last */
} android_LogPriority;
#define LOGV(...) ((void)LOG(LOG_VERBOSE, LOG_TAG, ##__VA_ARGS__))
#define LOGD(...) ((void)LOG(LOG_DEBUG, LOG_TAG, ##__VA_ARGS__))
#define LOGI(...) ((void)LOG(LOG_INFO, LOG_TAG, ##__VA_ARGS__))
#define LOGW(...) ((void)LOG(LOG_WARN, LOG_TAG, ##__VA_ARGS__))
#define LOGE(...) ((void)LOG(LOG_ERROR, LOG_TAG, ##__VA_ARGS__))

#define LOG(prio, tag, fmt,...) (__android_log_print(prio, tag, fmt, ##__VA_ARGS__))
int __android_log_print(int prio, const char *tag, const char *fmt, ...);
void __android_log_assert(const char *cond, const char *tag,
		const char *fmt, ...);
int __android_log_vprint(int prio, const char *tag, const char *fmt, va_list ap);
int __android_log_bwrite(int32_t tag, const void *payload, size_t len);
int __android_log_btwrite(int32_t tag, char type, const void *payload,
	size_t len);
#endif
