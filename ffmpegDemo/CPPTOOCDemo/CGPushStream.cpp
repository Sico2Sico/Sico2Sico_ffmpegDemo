//
//  CGPushStream.cpp
//  CPPTOOCDemo
//
//  Created by 德志 on 2018/10/14.
//  Copyright © 2018年 com.aiiage.www. All rights reserved.
//

#include "CGPushStream.hpp"
#include "PrefixHeader.pch"
#include <string>
#include <memory>
#include <thread>
#include <iostream>

using namespace std;

AVFormatContext * inputContext = nullptr;
AVFormatContext * outputContext = nullptr;
AVFrame
int openInput(string url){

    inputContext  = avformat_alloc_context();
    AVDictionary * options = nullptr;
    av_dict_set(&options, "rtsp_transport", "udp", 0);
    int ret = avformat_open_input(&inputContext, url.c_str(), nullptr, &options);

    if (ret < 0) {
        return -1;
    }

    ret = avformat_find_stream_info(inputContext, nullptr);

    return  ret;
}

shared_ptr<AVPacket> readPacketFrameSource() {

    shared_ptr<AVPacket> packet(static_cast<AVPacket*>(av_malloc(sizeof(AVPacket))), [&](AVPacket *p) { av_packet_free(&p); av_freep(&p);});

    av_init_packet(packet.get());

    int ret = av_read_frame(inputContext, packet.get())

    if (ret < 0) {
        return  nullptr
    }else{
        return  packet;
    }
}

void av_packet_reacalse_ts(AVPacket* pkt, AVRational src_t, AVRational std_t ) {

    if (pkt->pts != AV_NOPTS_VALUE) {
        pkt->pts = av_rescale_q(pkt->pts, src_t, std_t);
    }
    if (pkt->dts != AV_NOPTS_VALUE) {
        pkt->dts = av_rescale_q(pkt->pts, src_t, std_t);
    }

    if (pkt->duration > 0)
        pkt->duration = av_rescale_q(pkt->duration, std_t, std_t);
}

int WitePacket (shared_ptr<AVPacket> packet) {

    auto inputStream = inputContext ->streams[packet->stream_index];
    auto outputStream = outputContext-> streams[packet->stream_index];
    av_packet_rescale_ts(packet.get(), inputStream->time_base, outputStream->time_base);
    return  av_interleaved_write_frame(outputContext, packet.get());
}

int oepnoutput(string url ){

    int ret = avformat_alloc_output_context2(&outputContext,nullptr, "flv", url.c_str());
    if (ret < 0) {
        return  -1;
    }

    ret = avio_open2(&outputContext->pb,url.c_str(), AVIO_FLAG_READ_WRITE,nullptr, nullptr);

    if (ret < 0) {
        return  -1;
    }

    for (int i = 0; i < outputContext->nb_streams; i++) {
        AVStream * stream = avformat_new_stream(outputContext, inputContext->streams[i]->codec->codec);

        ret = avcodec_copy_context(stream->codec,inputContext->streams[i]->codec);
    }

    ret = avformat_write_header(outputContext, nullptr);

}




















