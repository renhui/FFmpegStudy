#include <stdio.h>
#include "pch.h"
#include <iostream>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include <libavformat/avformat.h>  
};

// 自定义日志输出
void my_logoutput(void* ptr, int level, const char* fmt, va_list vl) {
	printf("FFmepeg Log = %s", fmt);
}

int main(int argc, char *argv[]) {
	/** FFmpeg Hello World **/
	av_register_all();

	printf("%s\n", avcodec_configuration());

	/** Use FFmpeg Log System **/
	av_log_set_level(AV_LOG_INFO);

	/** 设置日志输出等级，及自定义的日志输出方法 **/
	av_log_set_callback(my_logoutput);  
	av_log(NULL, AV_LOG_INFO, "Hello World\n");
}