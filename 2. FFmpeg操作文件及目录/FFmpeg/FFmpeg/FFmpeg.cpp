// FFmpegTest.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。

#include "pch.h"
#include <iostream>

extern "C" {
#include <stdio.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/timestamp.h"
#include "libavutil/log.h"
}

// FFmpeg 删除文件操作
void ffmpegDelFile() {
	int ret;
	ret = avpriv_io_delete("222.txt");  // 在项目目录下创建的文件（测试时需要创建好）
	printf("Del File Code : %d %s\n", ret, av_err2str(ret));
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Failed to delete file \n");
	}
	else {
		av_log(NULL, AV_LOG_INFO, "Delete File Success！\n ");
	}
}

// FFmpeg 重命名或移动文件
void ffmpegMoveFile(char* src, char* dst) {
	int ret;

	ret = avpriv_io_move(src, dst);

	printf("Move File Code : %d \n", ret);

	// 重命名时，如果文件不存在，ret也会0
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Failed to Move File %s!\n ", src);
	}
	else {
		av_log(NULL, AV_LOG_INFO, "Success Move File %s!\n", src);
	}
}

// FFmpeg 目录操作
void ffmpegDir() {

	int ret;

	// 上下文
	AVIODirContext *dirCtx = NULL;
	AVIODirEntry *dirEntry = NULL;

	// 注意Windows下会返回-40，也就是Function not implement，方法未实现，也就是说windows下不支持此方法
	ret = avio_open_dir(&dirCtx, "./", NULL);

	if (ret < 0) {
		// 输出错误日志
		printf("cant open dir，msg = %s", av_err2str(ret));
		return;
	}

	av_log(NULL, AV_LOG_INFO, "Open Dir Success!");

	while (1) {
		ret = avio_read_dir(dirCtx, &dirEntry);
		if (ret < 0) {
			printf("cant read dir : %s", av_err2str(ret));
			// 防止内存泄漏
			goto __failed;
		}
		av_log(NULL, AV_LOG_INFO, "read dir success");
		if (!dirEntry) {
			break;
		}
		printf("Entry Name = %s", dirEntry->name);
		// 释放资源
		avio_free_directory_entry(&dirEntry);
	}
	// 释放资源
__failed:
	avio_close_dir(&dirCtx);
}

// 使用FFmpeg打印多媒体文件的Meta信息
void ffmpegVideoMeta() {

	av_log_set_level(AV_LOG_INFO);

	AVFormatContext *fmt_ctx = NULL;

	av_register_all();

	int ret;
	// 参数为 AVFormatContext上下文、文件名、指定的输入格式（一般为NULL，由ffmpeg自行解析）、附加参数（一般为NULL）
	ret = avformat_open_input(&fmt_ctx, "111.mp4", NULL, NULL);

	if (ret < 0) {
		printf("Cant open File: %s\n", av_err2str(ret));
	}

	// 参数为AVFormatContext上下文、流索引值（一般不用关心，直接写0）、文件名、是否是输入出文件（1：是  0：不是）	
	av_dump_format(fmt_ctx, 0, "111.mp4", 0);

	// 关闭打开的多媒体文件
	avformat_close_input(&fmt_ctx);
}


int main(int argc, char* argv[]) {

	av_register_all();

	/** 使用FFmpeg删除文件 **/
	//ffmpegDelFile();  // 删除文件 (部分Windows机型运行会存在问题 ret = -22)

	/** 使用FFmpeg重命名文件 **/
	//char src[] = "111.txt";
	//char dst[] = "222.txt";
	//ffmpegMoveFile(src, dst);  //  重命名文件

	/** 使用FFmpeg操作文件目录（注:此操作可能在Windows下面不支持） **/
	//ffmpegDir();  // 文件目录操作

	/** 使用FFmpeg操作文件目录使用FFmpeg打印多媒体的Mate信息 **/
	ffmpegVideoMeta(); // FFmpeg打印多媒体文件的Meta信息


	return 0;
}
