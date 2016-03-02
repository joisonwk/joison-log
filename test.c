#include "jlog.h"

#define LOG_NDEBUG 0
#define LOG_NDDEBUG 0
#define LOG_NIDEBUG 0
#define LOG_TAG	"test"

int main(){
	LOGV("verbose log");
	//LOGD("debug log");
	//LOGE("error log");
	__android_log_print(ANDROID_LOG_ERROR,LOG_TAG,"__android_log_print called\n");
	return 0;
}
