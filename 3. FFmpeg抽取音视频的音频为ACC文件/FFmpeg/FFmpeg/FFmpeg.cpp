// FFmpegTest.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include "AACFormat.h"

extern "C" {
#include <stdio.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/timestamp.h"
#include "libavutil/log.h"
}

// 使用FFmpeg从视频中抽取音频
void extractAudio_HE_ACC() {
	/************************************************************
	 * 目前只能抽取HE-AAC格式的MP4文件(LC及其他格式抽取后不能播放)。
	 *
	 * TODO：研究HE-AAC和LC-AAC的区别，整理一下AAC的格式相关的知识
	 ************************************************************/

	 // 设置日志输出等级
	av_log_set_level(AV_LOG_INFO);

	AVFormatContext *fmt_ctx = NULL;
	AVPacket pkt;

	av_register_all();

	int ret;
	int len;
	int audio_index = -1;

	// 打开输入文件
	ret = avformat_open_input(&fmt_ctx, "111.mp4", NULL, NULL);

	// 检查打开输入文件是否成功
	if (ret < 0) {
		printf("cant open file，error message = %s", av_err2str(ret));
		return;
	}

	// 打开输出文件
	FILE* dst_fd = fopen("111.aac", "wb");  // w 写入  b 二进制文件

	// 检查输出文件打开是否成功，如果失败，就输出日志，并关闭输出文件的引用
	if (!dst_fd) {
		av_log(NULL, AV_LOG_ERROR, "Can't Open Out File!\n");
		avformat_close_input(&fmt_ctx);
	}

	// 获取到音频流(使用av_find_best_stream：多媒体文件中拿到想使用的最好的一路流）

	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;

	ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	for (int i = 0; i < fmt_ctx->nb_streams; i++) {
		if (fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_index = i;
			break;
		}
	}

	printf("Audio Stream Index = %d", audio_index);

	// 检查发现音频流的结果
	if (audio_index < 0) {
		av_log(NULL, AV_LOG_ERROR, "Can't find Best Audio Stream!\n");
		//printf("Reason = %s", av_err2str(ret));
		// 关闭输出文件和输出文件的引用
		avformat_close_input(&fmt_ctx);
		fclose(dst_fd);
		return;
	}

	while (av_read_frame(fmt_ctx, &pkt) >= 0) {
		if (pkt.stream_index == audio_index) {
			printf("Has Read An Audio Packet\n");
			char adts_header_buf[7];
			adts_header(adts_header_buf, pkt.size);
			fwrite(adts_header_buf, 1, 7, dst_fd);
			len = fwrite(pkt.data, 1, pkt.size, dst_fd);
			if (len != pkt.size) {
				av_log(NULL, AV_LOG_WARNING, "Waring! Length of data not equal size of pkt!\n");
			}
			fflush(dst_fd);
		}
		// 将引用基数减一
		av_packet_unref(&pkt);
		//av_free_packet(&pkt);
	}

	// 关闭文件（输入/输出）
	avformat_close_input(&fmt_ctx);
	if (dst_fd) {
		fclose(dst_fd);
	}
}

// 使用FFmpeg的API从视频中抽取音频
void extractAudio() {
	/**
	* 调用 av_guess_format 让ffmpeg帮你找到一个合适的文件格式。
	* 调用 avformat_new_stream 为输出文件创建一个新流。
	* 调用 avio_open 打开新创建的文件。
	* 调用 avformat_write_header 写文件头。
	* 调用 av_interleaved_write_frame 写文件内容。
	* 调用 av_write_trailer 写文件尾。
	* 调用 avio_close 关闭文件。
	**/

	av_log_set_level(AV_LOG_INFO);
	av_register_all();

	FILE *dst_fd = NULL;

	int ret;

	AVFormatContext *fmt_ctx = NULL;  // 输入文件的AVFormatContext上下文
	AVStream *in_stream = NULL;

	AVPacket pkt;

	AVFormatContext *ofmt_ctx = NULL;
	AVOutputFormat *output_fmt = NULL;
	AVStream *out_stream = NULL;

	int audio_stream_index = -1;

	ret = avformat_open_input(&fmt_ctx, "1111.mp4", NULL, NULL);

	// 检查打开输入文件是否成功
	if (ret < 0) {
		printf("cant open file，error message = %s", av_err2str(ret));
		return;
	}

	/*retrieve audio stream*/
	if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
		av_log(NULL, AV_LOG_DEBUG, "failed to find stream information!\n");
		return;
	}

	in_stream = fmt_ctx->streams[1];

	AVCodecParameters *in_codecpar = in_stream->codecpar;
	if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO) {
		av_log(NULL, AV_LOG_ERROR, "The Codec type is invalid!\n");
		return;
	}

	//out file
	ofmt_ctx = avformat_alloc_context();
	output_fmt = av_guess_format(NULL, "222.aac", NULL);
	if (!output_fmt) {
		av_log(NULL, AV_LOG_DEBUG, "Cloud not guess file format \n");
		return;
	}

	ofmt_ctx->oformat = output_fmt;

	out_stream = avformat_new_stream(ofmt_ctx, NULL);
	if (!out_stream) {
		av_log(NULL, AV_LOG_DEBUG, "Failed to create out stream!\n");
		return;
	}

	if (fmt_ctx->nb_streams < 2) {
		av_log(NULL, AV_LOG_ERROR, "the number of stream is too less!\n");
		return;
	}

	if ((ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Failed to copy codec parameter, %d(%s)\n");
	}

	out_stream->codecpar->codec_tag = 0;

	if ((ret = avio_open(&ofmt_ctx->pb, "222.aac", AVIO_FLAG_WRITE)) < 0) {
		av_log(NULL, AV_LOG_DEBUG, "Could not open file! \n");
	}

	/*initialize packet*/
	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;

	/*find best audio stream*/
	audio_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if (audio_stream_index < 0) {
		av_log(NULL, AV_LOG_ERROR, "Failed to find Audio Stream!\n");
	}

	if (avformat_write_header(ofmt_ctx, NULL) < 0) {
		av_log(NULL, AV_LOG_DEBUG, "Error occurred when opening output file");
	}

	/*read frames from media file*/
	while (av_read_frame(fmt_ctx, &pkt) >= 0) {
		if (pkt.stream_index == audio_stream_index) {
			pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.dts = pkt.pts;
			pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
			pkt.pos = -1;
			pkt.stream_index = 0;
			av_interleaved_write_frame(ofmt_ctx, &pkt);
			av_packet_unref(&pkt);
		}
	}

	av_write_trailer(ofmt_ctx);

	/*close input media file*/
	avformat_close_input(&fmt_ctx);
	if (dst_fd) {
		fclose(dst_fd);
	}

	avio_close(ofmt_ctx->pb);
	return;


}

int main(int argc, char* argv[]) {

	av_register_all();
	
	/** 使用FFmpeg从视频中抽取音频 **/
	//extractAudio_HE_ACC(); // (仅支持音频为HE-ACC格式的MP4)
	extractAudio();  // 不限格式，使用FFmpeg写入头

	return 0;
}
