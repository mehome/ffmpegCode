#pragma once
#include "EvHeade.h"
#include "sei_packet.h"

inline AVFormatContext * av_open_file(const char * file)
{
	AVFormatContext * formatContext = avformat_alloc_context();

	int ret = avformat_open_input(&formatContext, "../sp.mp4", NULL, NULL);
	if (ret != 0) {
		avformat_close_input(&formatContext);
		return NULL;
	}

	ret = avformat_find_stream_info(formatContext, NULL);
	if (ret != 0)
	{
		avformat_close_input(&formatContext);
		return NULL;
	}

	return formatContext;
}

inline int resetPacket(AVPacket * packet, AVPacket * pkt)
{
	//�����Զ�������
	char buffer[128];
	sprintf(buffer, "pts:%lld dts:%lld", packet->pts, packet->dts);
	int size = strlen(buffer);
	char * data = buffer;
	//��ȡ�Զ������ݳ���
	size_t sei_packet_size = get_sei_packet_size(size);

	av_new_packet(pkt, packet->size + (int)sei_packet_size);
	memset(pkt->data, 0, packet->size + sei_packet_size);
	pkt->pts = packet->pts;
	pkt->dts = packet->dts;

	bool addInHead = true;
	if (addInHead)
	{
		//���ԭʼ����
		memcpy(pkt->data + sei_packet_size, packet->data, packet->size);
		//����Զ�������
		unsigned char * sei = (unsigned char*)pkt->data;
		fill_sei_packet(sei, false, data, size);
	}
	else
	{
		//���ԭʼ����
		memcpy(pkt->data, packet->data, packet->size);

		//����Զ�������
		unsigned char * sei = (unsigned char*)pkt->data + packet->size;
		fill_sei_packet(sei, false, data, size);
	}

	return 0;
}

//#define NEW_API

inline int decodeVideo(AVCodecContext * context, AVPacket * packet, AVFrame * frame)
{
#ifdef NEW_API
	int ret = avcodec_send_packet(context, packet);
	if (ret == 0)ret = avcodec_receive_frame(context, frame);
#else
	int got_picture_ptr = 0;
	int ret = avcodec_decode_video2(context, frame, &got_picture_ptr, packet);
#endif
	if (got_picture_ptr > 0) {
		printf("decode:%lld %d %d\n", packet->pts, frame->width, frame->height);
		return 1;
	}
	else
	{
		if (ret < 0)
		{
			char str[1024];
			av_make_error_string(str, 1024, ret);
			printf("decodeVideo Error:%d %s\n", ret, str);
		}
		else
		{
			return 0;
		}
	}
	return ret;
}

inline int getVideoId(AVFormatContext * formatContext)
{
	int videoIndex = -1;
	for (size_t i = 0; i < formatContext->nb_streams; i++)
	{
#ifdef NEW_API
		if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
#else
		if (formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
#endif
		{
			videoIndex = (int)i;
			break;
		}
	}
	return videoIndex;
}

inline AVCodecContext * openCodec(AVStream * stream)
{
#ifdef NEW_API
	AVCodecParameters * paramter = formatContext->streams[videoIndex]->codecpar;

	AVCodec * codec = avcodec_find_decoder(paramter->codec_id);

	AVCodecContext * codecContext = avcodec_alloc_context3(codec);

	avcodec_parameters_to_context(codecContext, paramter);
#else
	AVCodec * codec = avcodec_find_decoder(stream->codec->codec_id);
	AVCodecContext * codecContext = stream->codec;
#endif
	int ret = avcodec_open2(codecContext, codec, NULL);
	if (ret != 0) {
		avcodec_close(codecContext);
		avcodec_free_context(&codecContext);
		return NULL;
	}

	return codecContext;
}

#ifndef NEW_API
inline inline AVPacket * av_packet_alloc()
{
	AVPacket * packet = (AVPacket*)av_malloc(sizeof(AVPacket));
	av_init_packet(packet);
	return packet;
}

inline inline void av_packet_free(AVPacket **ppacket)
{
	if (ppacket == NULL) return;
	AVPacket *packet = *ppacket;
	if (packet == NULL) return;
	av_free_packet(packet);
	av_free(packet);
	*ppacket = NULL;
}
#endif