//
//  CGFileCut.cpp
//  CPPTOOCDemo
//
//  Created by 德志 on 2018/10/12.
//  Copyright © 2018年 com.aiiage.www. All rights reserved.
//

#include "CGFileCut.hpp"
#include "PrefixHeader.pch"
#include <string>
#include <memory>
#include <thread>
#include <iostream>

using namespace std;


AVFormatContext *  inputContext = nullptr;
AVFormatContext *  outputContext = nullptr;

int64_t lastReadTime = 0;

static int interrupt_cd(void *ctx){
    int timeout = 3;
    if (av_gettime() - lastReadTime > timeout * 1000 * 1000){
        return -1;
    }
    return 0;
}


int openInput(string url) {

    inputContext = avformat_alloc_context();
    lastReadTime = av_gettime();

    inputContext->interrupt_callback.callback = interrupt_cd;
    int ret = avformat_open_input(&inputContext, url.c_str(), nullptr, nullptr);
    if (ret < 0 ){
        return  -1;
    }

    ret = avformat_find_stream_info(inputContext, nullptr);
    if(ret < 0) {
        return -1;
    }

    return 0;
}

shared_ptr<AVPacket> readPacketFromSource() {

    shared_ptr<AVPacket> pkt (static_cast<AVPacket*>(av_malloc(sizeof(AVPacket))),[&](AVPacket *p){av_packet_free(&p); av_free(&p);});

    av_init_packet(pkt.get());
    lastReadTime = av_gettime();

    int ret = av_read_frame(inputContext, pkt.get());
    if (ret < 0) {
        return nullptr;
    }
    return  pkt;
}


int openoutput( string url){

    int ret = avformat_alloc_output_context2(&outputContext, nullptr, "flv", url.c_str());

    if(ret < 0) {
        return -1;
    }

    ret = avio_open2(&outputContext->pb, url.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
    if(ret < 0) {
        return -1;
    }


    for (int i = 0; i < inputContext->nb_streams; i++){
        AVStream * stream = avformat_new_stream(outputContext, inputContext->streams[i]->codec->codec);

        ret = avcodec_copy_context(stream->codec, inputContext->streams[i]->codec);

        if(ret < 0){
            return -1;
        }
    }

    ret = avformat_write_header(outputContext, nullptr);
    if(ret < 0){
        return -1;
    }

    return 0;
}

void Init() {
    av_register_all();
    avfilter_register_all();
}


void closeInput() {
    if(inputContext != nullptr) {
        avformat_close_input(&inputContext);
    }
}

void closeOutput(){

    if (outputContext != nullptr) {

        for (int i =0; i < outputContext->nb_streams; i++) {
            AVCodecContext * codecContext = outputContext->streams[i]->codec;
            avcodec_close(codecContext);
        }

        avformat_close_input(&outputContext);
    }

}

int witePacket(shared_ptr<AVPacket>packet) {
    return av_interleaved_write_frame(outputContext, packet.get())
}


int _tmian() {

    //第20s开始 去掉8s
    int startPacketNum = 500;
    int discardtPacketNum = 200;

    int packetcount = 0;

    int64_t lastPacketPts = AV_NOPTS_VALUE;
    int64_t lastPts = AV_NOPTS_VALUE;

    Init();
    int ret = openInput("");

    if(ret < 0) {
        return -1;
    }

    ret = openoutput("");

    while (true) {

        auto packet = readPacketFromSource();
        if(packet) {

            packetcount ++;
            if(packetcount <= 500 || packetcount >= 700){

                if(packetcount >= 700){

                    if(packet -> pts - lastPacketPts > 120) {
                        lastPts = lastPacketPts;
                    }else {
                        auto diff = packet->pts - lastPacketPts;
                        lastPts += diff;
                    }
                }
            }

            lastPacketPts = packet->pts;
            if(lastPts != AV_NOPTS_VALUE){
                packet->pts = packet->dts = lastPts;
            }

            ret = WritePacket
        }


    }
}
























