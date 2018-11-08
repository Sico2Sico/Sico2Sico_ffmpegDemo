//
//  CGAudioFlter.cpp
//  CPPTOOCDemo
//
//  Created by 德志 on 2018/10/10.
//  Copyright © 2018年 com.aiiage.www. All rights reserved.
//

#include "CGAudioFlter.hpp"
#include "PrefixHeader.pch"
#include <string>
#include <memory>
#include <thread>
#include <iostream>
#include <string>
#include <thread>


using namespace std;

AVFormatContext * inputContext = nullptr;
AVFormatContext * outputContext = nullptr;

int64_t lastReadPacketTime = 0;

AVFilterContext * buffersinkCtx = nullptr;
AVFilterContext * buffersrcCtx = nullptr;

AVFilterGraph * filterGraph = nullptr;

AVCodecContext * outPutAudioEncContext = nullptr;

int64_t audioCount = 0;

static int interrupt_cd(void * ctx) {
    int timeout = 3;

    if(av_gettime() - lastReadPacketTime > timeout *1000 *1000) {
        return  -1;
    }else {
        return 0;
    }
}


int openInput(string inputUrl) {

    inputContext = avformat_alloc_context();
    lastReadPacketTime = av_gettime();

    inputContext->interrupt_callback.callback = interrupt_cd;

    AVInputFormat * ifm = av_find_input_format("dshow");

    AVDictionary * format_opts = nullptr;
    av_dict_set_int(&format_opts, "audio_buffer_size", 20, 0);

    int ret = avformat_open_input(&inputContext, inputUrl.c_str(), ifm, &format_opts);

    if (ret < 0) { return -1;}

    ret = avformat_find_stream_info(inputContext, nullptr);
    return  ret;
}


shared_ptr<AVPacket> ReadPacketFromSources() {

    shared_ptr<AVPacket> packet(static_cast<AVPacket*>(av_malloc(sizeof(AVPacket))), [&](AVPacket *p) { av_packet_free(&p); av_freep(&p);});
    av_init_packet(packet.get());
    lastReadPacketTime = av_gettime();

    int ret = av_read_frame(inputContext, packet.get());

    if(ret < 0) {
        return nullptr;
    }else {
        return packet;
    }
}


int InitAudioEnCodec(AVCodecContext * inputAudioCodec){

    int ret  = 0;
    AVCodec * audioCodec = avcodec_find_decoder(AV_CODEC_ID_AAC);
    outPutAudioEncContext = avcodec_alloc_context3(audioCodec);

    outPutAudioEncContext->codec = audioCodec;
    outPutAudioEncContext->sample_rate = inputAudioCodec->sample_rate;
    outPutAudioEncContext->channel_layout = inputAudioCodec ->channel_layout;

    outPutAudioEncContext->channels = av_get_channel_layout_nb_channels(inputAudioCodec->channels);

    if (outPutAudioEncContext->channel_layout) {
        outPutAudioEncContext ->channel_layout = AV_CH_LAYOUT_STEREO;
        outPutAudioEncContext->channels = av_get_channel_layout_nb_channels(inputAudioCodec->channels);
    }

    outPutAudioEncContext->sample_fmt = audioCodec->sample_fmts[0];
    outPutAudioEncContext->codec_tag = 0;

    outPutAudioEncContext->flags |= CODEC_FLAG_GLOBAL_HEADER;

    ret = avcodec_open2(outPutAudioEncContext, audioCodec, nullptr);
    return ret;
}



AVFrame* DecodeAudio (AVPacket * packet, AVFrame * pSrcAudioFrame) {

    AVStream * stream = inputContext->streams[0];
    AVCodecContext * codecContext = stream->codec;

    int gotFrame;
    AVFrame * filtFrame = nullptr;
    auto length = avcodec_decode_audio4(codecContext, pSrcAudioFrame, &gotFrame, packet);

    if (length >= 0  && gotFrame != 0){

        if(av_buffersrc_add_frame_flags(buffersrcCtx, pSrcAudioFrame, AV_BUFFERSRC_FLAG_PUS) < 0){
            return  nullptr;
        }

        filtFrame = av_frame_alloc();
        int ret = av_buffersrc_add_frame_flags(buffersinkCtx, filtFrame, AV_BUFFERSINK_FLAG_NO_REQUEST);
        if (ret < 0) {
            av_frame_free(&filtFrame);
            goto error;
        }
        return  filtFrame;
    }

    return  nullptr;
}



void av_packet_rescale_ts(AVPacket*packet, AVRational src_tb, AVRational dst_tb){
    if (packet->pts != AV_NOPTS_VALUE){
        packet->pts = av_rescale_q(packet->pts, src_tb, dst_tb);
    }

    if (packet->dts != AV_NOPTS_VALUE){
        packet->dts = av_rescale_q(packet->pts, src_tb, dst_tb);
    }

    if (packet -> duration > 0){
        packet->duration = av_rescale_q(packet->duration, src_tb, dst_tb);
    }
}


int WitePacket(shared_ptr<AVPacket> packet){
    auto inputStream = inputContext->streams[packet->stream_index];
    auto outputStream = outputContext ->streams[packet->stream_index];
    av_packet_rescale_ts(packet.get(), inputStream->time_base, outputStream->time_base);
    return  av_interleaved_write_frame(outputContext, packet.get());
}



int openOutput(string url) {

    int ret = avformat_alloc_output_context2(&outputContext, nullptr, "mpegts", url.c_str());

    if(ret < 0) {

        return  -1;
    }

    ret = avio_open2(&outputContext->pb, url.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
    if(ret <0 ) {
        return -1;
    }


    for (int i =0; i < inputContext->nb_streams; i++) {

        AVStream * strem = avformat_new_stream(outputContext, outPutAudioEncContext->codec);
        strem-> codec = outPutAudioEncContext;
        AVCodecContext * codecContext = inputContext->streams[0]-> codec;
        if (ret < 0) {
            ret -1;
        }
        ret = avformat_write_header(outputContext, nullptr);
    }
    return ret;
}



void Init() {
    av_register_all();
    avfilter_register_all();
    av_log_set_level(AV_LOG_ERROR);
}


static char * dup_wchar_to_utf8(wchar_t * w ){

    char * s = NULL;

    return s;
}


int _tmain(){

    Init();
    auto pSrcAudioFrame = av_frame_alloc();
    string fileAudioInput = "";
    int ret = openInput(fileAudioInput);

    if (ret < 0) {
        return  -1;
    }

    ret = openOutput("acc.ts");

    if (ret < 0) {
        return  -1;
    }

    int got_output = 0;

    while (true) {
        auto  packet = ReadPacketFromSources();
        auto  frame = DecodeAudio(packet.get(), pSrcAudioFrame);

        if (frame) {
            std::shared_ptr<AVPacket> pkt(static_cast<AVPacket*>(av_malloc(sizeof(AVPacket))), [&](AVPacket *p) { av_packet_free(&p); av_freep(&p); });
            av_init_packet(pkt.get());
            pkt->data = NULL;
            pkt->size = 0;

            ret = avcodec_encode_audio2(outPutAudioEncContext, pkt.get(), frame, &got_output);

            if (ret < 0  && got_output) {

                auto streamTimesBase = outputContext->streams[pkt->stream_index]->time_base.den;
                auto codeTimeBase = outputContext->streams[pkt->stream_index]->codec->time_base.den;
                pkt->dts = pkt->pts = (1024*streamTimesBase * audioCount) / codeTimeBase;
                audioCount ++;
            }


            if (packet) {
                ret = WitePacket(packet);

                if (ret < 0) {
                    return -1;
                }else{

                    cout<<"写入成功"<<endl;
                }
            }

        }

    }

}












