//
//  CGAVvioRead.cpp
//  CPPTOOCDemo
//
//  Created by 德志 on 2018/10/4.
//  Copyright © 2018年 com.aiiage.www. All rights reserved.
//

#include "CGAVvioRead.hpp"
#include "PrefixHeader.pch"
#include <string>
#include <memory>
#include <thread>
#include <iostream>

using namespace std;

class updSocket {
    public : updSocket(){

    }
};


AVFormatContext * inputContext = nullptr;
AVFormatContext * outputContext = nullptr;
int64_t lastReadPacketTime = 0;


static int interrupt_cb(void *cxt){
    int timeout = 3;
    if (av_gettime() - lastReadPacketTime > timeout * 1000* 1000){
        return  -1;
    }else {
        return  0;
    }
}

static int readUdpSocket(void* opaque , uint8_t* buf, int buf_size){
    return 0;
}

int OpenInput() {
    lastReadPacketTime = av_gettime();

//    int size = 32*1024;
//    uint8_t * iobuffer = (uint8_t*)av_malloc(size);
//    AVIOContext * avio = avio_alloc_context(iobuffer, size, 0,&updSocket ,readUdpSocket,NULL, NULL);
//    inputContext = avformat_alloc_context();
//    inputContext->pb = avio;
//
//    inputContext->start_time_realtime = av_gettime();
//
//    int ret = avformat_open_input(&inputContext, nullptr, nullptr, nullptr);
//    if (ret < 0) {
//        return -1;
//    }
//
//    ret = avformat_find_stream_info(inputContext, nullptr);
//
//    if (ret < 0 ) {
//        return -1;
//    }

    return 0;
}

shared_ptr<AVPacket> readPacketFromSource() {
   shared_ptr<AVPacket> packet(static_cast<AVPacket*>(av_malloc(sizeof(AVPacket))), [&](AVPacket *p) { av_packet_free(&p); av_freep(&p);});
    av_init_packet(packet.get());

    lastReadPacketTime = av_gettime();
    int ret = av_read_frame(inputContext, packet.get());
    if (ret < 0 ){
        return nullptr;
    }else {
        return packet;
    }
}


void av_packet_rescale_ts(AVPacket * pkt , AVRational src_tb, AVRational dst_tb) {

    if (pkt->pts != AV_NOPTS_VALUE) {
        pkt->pts = av_rescale_q(pkt->pts, src_tb, dst_tb);
    }

    if (pkt->dts != AV_NOPTS_VALUE) {
        pkt->dts = av_rescale_q(pkt->dts, src_tb, dst_tb);
    }

    if(pkt->duration > 0){
        pkt->duration = av_rescale_q(pkt->duration, src_tb, dst_tb);
    }
}


int wirtePacket(shared_ptr<AVPacket> packet) {
    auto inputStream = inputContext->streams[packet->stream_index];
    auto outputStream = outputContext->streams[packet->stream_index];
    av_packet_rescale_ts(packet.get(), inputStream->time_base, outputStream->time_base);
    return  av_interleaved_write_frame(outputContext, packet.get());
}

int openOutput(string url) {

    int ret = avformat_alloc_output_context2(&outputContext, nullptr, "flv", url.c_str());
    if(ret < 0){
        return  -1;
    }

    ret = avio_open2(&outputContext->pb,url.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
    if(ret < 0) {
        return -1;
    }

    ret = avformat_write_header(outputContext, nullptr);

    return ret;
}


void closeInput() {

    if (inputContext != nullptr) {
        avformat_close_input(&inputContext);
    }
}

void closeOutput() {

    if(outputContext != nullptr){
        for (int i = 0; i < outputContext->nb_streams; i++) {
            AVCodecContext *codecContext = outputContext->streams[i]->codec;
            avcodec_close(codecContext);
        }
        avformat_close_input(&outputContext);
    }
}


void Init(){
    av_register_all();
    avfilter_register_all();
    avformat_network_init();
    av_log_set_level(AV_LOG_ERROR);
}

int _main() {

    Init();

    int ret = OpenInput();
    if(ret < 0 ){return  -1;}

    ret = openOutput("rtmp://127.0.0.1/live/stream0");

    while (true) {
        auto packet = readPacketFromSource();
        if (packet != nullptr) {
            ret = wirtePacket(packet);
        }

    }
}



