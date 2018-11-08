//
//  FFmpageManger.cpp
//  CPPTOOCDemo
//
//  Created by 德志 on 2018/10/3.
//  Copyright © 2018年 com.aiiage.www. All rights reserved.
//

#include "FFmpageManger.hpp"
#include "PrefixHeader.pch"

#include <string>
#include <memory>
#include <thread>
#include <iostream>
using namespace std;

AVFormatContext * inputContext = nullptr;
AVFormatContext * outputContext = nullptr;

int64_t lastReadPacktTime;

void init() {
    av_register_all();
    avfilter_register_all();
    avformat_network_init();
    av_log_set_level(AV_LOG_ERROR);
}

static int interrupt_cb(void * cxt){
    int timeout = 3;
    if (av_gettime() -lastReadPacktTime > timeout*1000*1000) {
        return  -1;
    }
    return 0;
}

int openInput(string inputurl){
    inputContext = avformat_alloc_context();
    lastReadPacktTime = av_gettime();
    inputContext->interrupt_callback.callback = interrupt_cb;

    int ret = avformat_open_input(&inputContext, inputurl.c_str(),nullptr, nullptr);

    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Find input files stream inform fail");
    }else{
        av_log(NULL, AV_LOG_ERROR, "open input file success");
    }
    return ret;
}

void av_packet_rescale_tc(AVPacket * pkt, AVRational src_tb, AVRational dst_tb) {
    if (pkt -> pts != AV_NOPTS_VALUE){
        pkt->pts = av_rescale_q(pkt->pts, src_tb, dst_tb);
    }
    if (pkt->dts != AV_NOPTS_VALUE){
        pkt->dts = av_rescale_q(pkt->dts, src_tb, dst_tb);
    }
    if (pkt -> duration > 0){
        pkt->duration = av_rescale_q(pkt->duration, src_tb, dst_tb);
    }
}

int writerPacket(shared_ptr<AVPacket> packet) {
    auto inputStream = inputContext -> streams[packet->stream_index];
    auto outputStream = outputContext -> streams[packet->stream_index];
    av_packet_rescale_ts(packet.get(), inputStream->time_base, outputStream->time_base);
    int ret = av_write_frame(outputContext, packet.get());
    return ret;
}

shared_ptr<AVPacket> readPacktFromSource(){
    shared_ptr<AVPacket> packet(static_cast<AVPacket*>(av_malloc(sizeof(AVPacket))), [&](AVPacket *p) { av_packet_free(&p); av_freep(&p);});
    av_init_packet(packet.get());
    lastReadPacktTime = av_gettime();

    int ret = av_read_frame(inputContext, packet.get());

    if (ret < 0){
        return packet;
    }else {
        return nullptr;
    }
}


int openOutput(string outUrl){
    int ret = avformat_alloc_output_context2(&outputContext, nullptr, "mpegts",outUrl.c_str());
    if (ret < 0) {
        cout << "open output context failed " << endl;
    }else {
        cout << "open output context success" << endl ;
    }

    ret = avio_open2(&outputContext->pb, outUrl.c_str(), AVIO_FLAG_WRITE,nullptr, nullptr);
    if (ret < 0) {
        cout << "wirter into fail" << endl;
    }else {
        cout << "wirter into success" << endl;
    }

    for ( int i = 0; i < inputContext->nb_streams; i++) {
       AVStream * stream = avformat_new_stream(outputContext, inputContext->streams[i]->codec->codec);
        ret = avcodec_copy_context(stream->codec, inputContext->streams[i]->codec);
        if (ret < 0) {
            cout << "copy codec context failed" << endl;
        }
    }

    ret = avformat_write_header(outputContext,nullptr);
    if(ret < 0) {
        cout << " format wirte header failed" << endl;
    }
    return ret;
}


void closeOutput() {
    if (outputContext != nullptr){
        for (int i =0 ; i < outputContext->nb_streams; i++){
            AVCodecContext * codecContext = outputContext->streams[i]->codec;
            avcodec_close(codecContext);
        }
        avformat_close_input(&outputContext);
    }
}

int _tmain() {

    init();
    int ret = openInput("rtsp://admin:admin12345@192.168.1.64:554/Streaming/Channels/1");

    if (ret < 0){
        cout << "open inout failed" << endl;
    }

    ret = openOutput("D:\\test.ts");

    while (true) {
        auto packet = readPacktFromSource();
        if (packet != nullptr){
            ret = writerPacket(packet);
            if (ret < 0){
                cout << "writer input faild" << endl;
            }else{
                cout << "writer input success " << endl;
            }
        }
    }
    return 0;
}






