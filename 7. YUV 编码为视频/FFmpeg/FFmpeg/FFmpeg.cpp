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

using namespace std;

int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index);

int YUV2H264()
{
	AVFormatContext *pFormatCtx = nullptr;
	AVOutputFormat *fmt = nullptr;
	AVStream *video_st = nullptr;
	AVCodecContext *pCodecCtx = nullptr;
	AVCodec *pCodec = nullptr;

	uint8_t *picture_buf = nullptr;
	AVFrame *picture = nullptr;
	int size;

	//打开视频文件
	FILE *in_file = fopen("111.yuv", "rb");
	if (!in_file) {
		cout << "can not open file!" << endl;
		return -1;
	}

	//352x288
	int in_w = 352, in_h = 288;
	int framenum = 50;
	const char* out_file = "111.H264";

	//[1] --注册所有ffmpeg组件
	avcodec_register_all();
	av_register_all();

	//[2] --初始化AVFormatContext结构体,根据文件名获取到合适的封装格式
	avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, out_file);
	fmt = pFormatCtx->oformat;

	//[3] --打开文件
	if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE)) {
		cout << "output file open fail!";
		return -1;
	}
	//[3]

	//[4] --初始化视频码流
	video_st = avformat_new_stream(pFormatCtx, 0);
	if (video_st == NULL)
	{
		printf("failed allocating output stram\n");
		return -1;
	}
	video_st->time_base.num = 1;
	video_st->time_base.den = 25;
	//[4]

	//[5] --编码器Context设置参数
	pCodecCtx = video_st->codec;
	pCodecCtx->codec_id = fmt->video_codec;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	pCodecCtx->width = in_w;
	pCodecCtx->height = in_h;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;
	pCodecCtx->bit_rate = 400000;
	pCodecCtx->gop_size = 12;

	if (pCodecCtx->codec_id == AV_CODEC_ID_H264)
	{
		pCodecCtx->qmin = 10;
		pCodecCtx->qmax = 51;
		pCodecCtx->qcompress = 0.6;
	}
	if (pCodecCtx->codec_id == AV_CODEC_ID_MPEG2VIDEO)
		pCodecCtx->max_b_frames = 2;
	if (pCodecCtx->codec_id == AV_CODEC_ID_MPEG1VIDEO)
		pCodecCtx->mb_decision = 2;
	//[5]

	//[6] --寻找编码器并打开编码器
	pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if (!pCodec)
	{
		cout << "no right encoder!" << endl;
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		cout << "open encoder fail!" << endl;
		return -1;
	}
	//[6]

	//输出格式信息
	av_dump_format(pFormatCtx, 0, out_file, 1);

	//初始化帧
	picture = av_frame_alloc();
	picture->width = pCodecCtx->width;
	picture->height = pCodecCtx->height;
	picture->format = pCodecCtx->pix_fmt;
	size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
	picture_buf = (uint8_t*)av_malloc(size);
	avpicture_fill((AVPicture*)picture, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

	//[7] --写头文件
	avformat_write_header(pFormatCtx, NULL);
	//[7]

	AVPacket pkt; //创建已编码帧
	int y_size = pCodecCtx->width*pCodecCtx->height;
	av_new_packet(&pkt, size * 3);

	//[8] --循环编码每一帧
	for (int i = 0; i < framenum; i++)
	{
		//读入YUV
		if (fread(picture_buf, 1, y_size * 3 / 2, in_file) < 0)
		{
			cout << "read file fail!" << endl;
			return -1;
		}
		else if (feof(in_file))
			break;

		picture->data[0] = picture_buf; //亮度Y
		picture->data[1] = picture_buf + y_size; //U
		picture->data[2] = picture_buf + y_size * 5 / 4; //V
		//AVFrame PTS
		picture->pts = i;
		int got_picture = 0;

		//编码
		int ret = avcodec_encode_video2(pCodecCtx, &pkt, picture, &got_picture);
		if (ret < 0)
		{
			cout << "encoder fail!" << endl;
			return -1;
		}

		if (got_picture == 1)
		{
			cout << "encoder success!" << endl;

			// parpare packet for muxing
			pkt.stream_index = video_st->index;
			av_packet_rescale_ts(&pkt, pCodecCtx->time_base, video_st->time_base);
			pkt.pos = -1;
			ret = av_interleaved_write_frame(pFormatCtx, &pkt);
			av_free_packet(&pkt);
		}
	}
	//[8]

	//[9] --Flush encoder
	int ret = flush_encoder(pFormatCtx, 0);
	if (ret < 0)
	{
		cout << "flushing encoder failed!" << endl;
		goto end;
	}
	//[9]

	//[10] --写文件尾
	av_write_trailer(pFormatCtx);
	//[10]

end:
	//释放内存
	if (video_st)
	{
		avcodec_close(video_st->codec);
		av_free(picture);
		av_free(picture_buf);
	}
	if (pFormatCtx)
	{
		avio_close(pFormatCtx->pb);
		avformat_free_context(pFormatCtx);
	}

	fclose(in_file);

	return 0;
}

int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index)
{
	int ret;
	int got_frame;
	AVPacket enc_pkt;
	if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities & AV_CODEC_CAP_DELAY))
		return 0;
	while (1) {
		printf("Flushing stream #%u encoder\n", stream_index);
		enc_pkt.data = NULL;
		enc_pkt.size = 0;
		av_init_packet(&enc_pkt);
		ret = avcodec_encode_video2(fmt_ctx->streams[stream_index]->codec, &enc_pkt,
			NULL, &got_frame);
		av_frame_free(NULL);
		if (ret < 0)
			break;
		if (!got_frame)
		{
			ret = 0; break;
		}
		cout << "success encoder 1 frame" << endl;

		// parpare packet for muxing
		enc_pkt.stream_index = stream_index;
		av_packet_rescale_ts(&enc_pkt,
			fmt_ctx->streams[stream_index]->codec->time_base,
			fmt_ctx->streams[stream_index]->time_base);
		ret = av_interleaved_write_frame(fmt_ctx, &enc_pkt);
		if (ret < 0)
			break;
	}
	return ret;
}

int H2642MP4() {

	AVOutputFormat *ofmt = NULL;
	//Input AVFormatContext and Output AVFormatContext
	AVFormatContext *ifmt_ctx_v = NULL, *ifmt_ctx_a = NULL, *ofmt_ctx = NULL;
	AVPacket pkt;
	int ret, i;
	int videoindex_v = 0, videoindex_out = 0;
	int frame_index = 0;
	int64_t cur_pts_v = 0, cur_pts_a = 0;
	const char *in_filename_v = "111.H264";
	const char *out_filename = "222.mp4";//Output file URL
	av_register_all();
	//Input
	if ((ret = avformat_open_input(&ifmt_ctx_v, in_filename_v, 0, 0)) < 0) {
		printf("Could not open input file.");
		goto end;

	}
	if ((ret = avformat_find_stream_info(ifmt_ctx_v, 0)) < 0) {
		printf("Failed to retrieve input stream information");
		goto end;
	}
	
	printf("===========Input Information==========\n");
	av_dump_format(ifmt_ctx_v, 0, in_filename_v, 0);
	//av_dump_format(ifmt_ctx_a, 0, in_filename_a, 0);
	printf("======================================\n");
	//Output
	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
	if (!ofmt_ctx) {
		printf("Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	ofmt = ofmt_ctx->oformat;
	printf("ifmt_ctx_v->nb_streams=%d\n", ifmt_ctx_v->nb_streams);
	for (i = 0; i < ifmt_ctx_v->nb_streams; i++) {
		//Create output AVStream according to input AVStream
		//if(ifmt_ctx_v->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
		{
			AVStream *in_stream = ifmt_ctx_v->streams[i];
			AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
			videoindex_v = i;
			if (!out_stream) {
				printf("Failed allocating output stream\n");
				ret = AVERROR_UNKNOWN;
				goto end;
			}
			videoindex_out = out_stream->index;
			//Copy the settings of AVCodecContext
			if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
				printf("Failed to copy context from input to output stream codec context\n");
				goto end;
			}
			out_stream->codec->codec_tag = 0;
			if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
				out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			//break;
		}
	}
	
	printf("==========Output Information==========\n");
	av_dump_format(ofmt_ctx, 0, out_filename, 1);
	printf("======================================\n");
	//Open output file
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		if (avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE) < 0) {
			printf("Could not open output file '%s'", out_filename);
			goto end;
		}
	}
	//Write file header
	if (avformat_write_header(ofmt_ctx, NULL) < 0) {
		printf("Error occurred when opening output file\n");
		goto end;
	}

	while (1) {
		AVFormatContext *ifmt_ctx;
		int stream_index = 0;
		AVStream *in_stream, *out_stream;
		//Get an AVPacket
		//if(av_compare_ts(cur_pts_v,ifmt_ctx_v->streams[videoindex_v]->time_base,cur_pts_a,ifmt_ctx_a->streams[audioindex_a]->time_base) <= 0)
		{
			ifmt_ctx = ifmt_ctx_v;
			stream_index = videoindex_out;
			if (av_read_frame(ifmt_ctx, &pkt) >= 0) {
				do {
					in_stream = ifmt_ctx->streams[pkt.stream_index];
					out_stream = ofmt_ctx->streams[stream_index];
					printf("stream_index==%d,pkt.stream_index==%d,videoindex_v=%d\n", stream_index, pkt.stream_index, videoindex_v);
					if (pkt.stream_index == videoindex_v) {
						//FIX：No PTS (Example: Raw H.264)
						//Simple Write PTS
						if (pkt.pts == AV_NOPTS_VALUE) {
							printf("frame_index==%d\n", frame_index);
							//Write PTS
							AVRational time_base1 = in_stream->time_base;
							//Duration between 2 frames (us)
							int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
							//Parameters
							pkt.pts = (double)(frame_index*calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
							pkt.dts = pkt.pts;
							pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1)*AV_TIME_BASE);
							frame_index++;
						}
						cur_pts_v = pkt.pts;
						break;
					}
				} while (av_read_frame(ifmt_ctx, &pkt) >= 0);
			}
			else {
				break;
			}
		}
		
		//Convert PTS/DTS
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;
		pkt.stream_index = stream_index;
		printf("Write 1 Packet. size:%5d\tpts:%lld\n", pkt.size, pkt.pts);
		//Write
		if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
			printf("Error muxing packet\n");
			break;
		}
		av_free_packet(&pkt);
	}
	//Write file trailer
	av_write_trailer(ofmt_ctx);

end:
	avformat_close_input(&ifmt_ctx_v);
	//avformat_close_input(&ifmt_ctx_a);
	/* close output */
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);
	if (ret < 0 && ret != AVERROR_EOF) {
		printf("Error occurred.\n");
		return -1;
	}
	return 0;
}

int main(int argc, char *argv[]) {
	// 先将YUV文件转换为H264文件
	YUV2H264();
	// 在将H264转封装为MP4
	H2642MP4();
}