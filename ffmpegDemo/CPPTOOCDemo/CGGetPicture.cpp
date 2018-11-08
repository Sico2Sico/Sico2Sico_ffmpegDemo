//
//  CGGetPicture.cpp
//  CPPTOOCDemo
//
//  Created by 德志 on 2018/10/13.
//  Copyright © 2018年 com.aiiage.www. All rights reserved.
//

#include "CGGetPicture.hpp"
#include "PrefixHeader.pch"
#include <string>
#include <memory>
#include <thread>
#include <iostream>

using namespace std;

AVFormatContext * inputContext = nullptr;
AVFormatContext * outputContext = nullptr;

int64_t lastReadPacketTime = 0;

static int interruput_cb(void * ctx) {
    int timeout = 3;
    if (av_gettime() - lastReadPacketTime > timeout *1000 *1000) {
        return  -1;
    }else {
        return 1;
    }
}

int openinput(string url) {
    inputContext = avformat_alloc_context();
    inputContext->interrupt_callback.callback = interruput_cb;
    int ret = avformat_open_input(&inputContext, url.c_str(), nullptr, nullptr);
    return  ret;
}


shared_ptr<AVPacket> readPacketFrameSource(){
    shared_ptr<AVPacket> packet(static_cast<AVPacket*>(av_malloc(sizeof(AVPacket))), [&](AVPacket *p) { av_packet_free(&p); av_freep(&p);});
    av_init_packet(packet.get());
    int ret = av_read_frame(inputContext, packet.get());

    if (ret < 0) {
        return  nullptr;
    }else{
        return  packet;
    }
}

int openOutput(string url) {

    int ret = avformat_alloc_output_context2(&outputContext,nullptr,"singlejpeg", url.c_str());
    if (ret < 0) {
        ret -1;
    }

    ret = avio_open2(&outputContext->pb, url.c_str(), AVIO_FLAG_WRITE,nullptr,nullptr);
    if (ret < 0) {
        return  -1;
    }

    for (int i = 0; i < inputContext->nb_streams; i++) {

        if(inputContext->streams[i]->codec->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO) {
            continue;
        }

        AVStream * stream = avformat_new_stream(outputContext, inputContext->streams[i]->codec->codec);
        ret = avcodec_copy_context(stream->codec, inputContext->streams[i]->codec);

        if (ret < 0) {
            return  -1;
        }
    }

    ret = avformat_write_header(outputContext,nullptr);



    if (outputContext) {

        for (int i =0; i < outputContext->nb_streams; i++) {
            avcodec_close(outputContext->streams[i]->codec);
        }
        avformat_close_input(&outputContext);
    }

    return  ret;
}




void Init(){
    av_register_all();
    avfilter_register_all();
    av_log_set_level(AV_LOG_ERROR);
}


void closeInput() {

    if (inputContext != nullptr) {
        avformat_close_input(&inputContext);
    }
}


void closeOutput() {

    if (outputContext != nullptr) {
        for (int i =0; i < outputContext->nb_streams; i++) {
            avcodec_close(outputContext->streams[i]->codec);
        }
        avformat_close_input(&outputContext);
    }
}

int WirtePacket(shared_ptr<AVPacket> packet) {
    auto inputStream = inputContext ->streams[packet->stream_index];
    auto outputStream = outputContext->streams[packet->stream_index];
    av_packet_rescale_ts(packet.get(), inputStream->time_base, outputStream->time_base);
    int ret = av_interleaved_write_frame(outputContext, packet.get());
}

int InitDecodecContext(AVStream * inputStream) {

    auto codecID = inputStream->codec->codec_id;
    auto codec = avcodec_find_decoder(codecID);

    if (codec == nullptr) {
        return  -1;
    }

    int ret = avcodec_open2(inputStream->codec, codec, nullptr);
    return  ret;
}

int initEncodec(AVStream * inputStream, AVCodecContext * encodecContext) {

    AVCodec * picCodec = nullptr;
    picCodec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);

    encodecContext =  avcodec_alloc_context3(picCodec);
    encodecContext->codec_id = picCodec->id;

    encodecContext->time_base.num = inputStream->codec->time_base.num;
    encodecContext->time_base.den = inputStream->codec->time_base.den;

    encodecContext->pix_fmt = *picCodec->pix_fmts;
    encodecContext ->width = inputStream->codec->width;
    encodecContext->height = inputStream->codec->height;

    int ret = avcodec_open2(encodecContext, picCodec, nullptr);

    return  ret;
}


bool Decode(AVStream* inputStream,AVPacket* packet, AVFrame *frame)
{
    int gotFrame = 0;
    auto hr = avcodec_decode_video2(inputStream->codec, frame, &gotFrame, packet);
    if (hr >= 0 && gotFrame != 0)
    {
        return true;
    }
    return false;
}

std::shared_ptr<AVPacket> Encode(AVCodecContext * encodeContext, AVFrame * frame){
    int gotoutput = 0;
    std::shared_ptr<AVPacket> pkt(static_cast<AVPacket*>(av_malloc(sizeof(AVPacket))), [&](AVPacket *p) { av_packet_free(&p); av_freep(&p); });
    av_init_packet(pkt.get());

    pkt->data = NULL;
    pkt->size = 0;
    int ret = avcodec_encode_video2(encodeContext, pkt.get(), frame, &gotoutput);

    if (ret >0 && gotoutput) {
        return  pkt;
    }else{
        return  nullptr;
    }
}




int _tmain(){

    Init();

    int ret = openinput("");
    if (ret < 0) {
        return  -1;
    }

    ret = openOutput("");

    if (ret < 0) {
        return  -1;
    }

    AVCodecContext *encodecContext = nullptr;
    InitDecodecContext(inputContext->streams[0]);

    AVFrame * videoFrame = av_frame_alloc();
    initEncodec(inputContext->streams[0], encodecContext);

    while (true) {

        auto packet = readPacketFrameSource();
        if (packet && packet->stream_index == 0) {
            if (Decode(inputContext->streams[0], packet.get(), videoFrame)) {

                auto packetEncode = Encode(encodecContext, videoFrame);
                if (packetEncode) {
                    ret = WirtePacket(packetEncode);

                    if (ret > 0) {
                        break;
                    }
                }
            }
        }
    }
    return 1;
}

























