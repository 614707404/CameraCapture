//
// Created by 戴威涛 on 2022/9/11.
//

#ifndef CAMERATORTMP_PUSHRTMP_H
#define CAMERATORTMP_PUSHRTMP_H
extern "C"{
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
};
struct StreamContext {
    AVCodecContext *dec_ctx;
    AVCodecContext *enc_ctx;

    AVFrame *dec_frame;
};

typedef struct FilteringContext {
    // buffersink 用于输出
    AVFilterContext *buffersink_ctx;
    // buffersrc 用于输入
    AVFilterContext *buffersrc_ctx;
    AVFilterGraph *filter_graph;

    AVPacket *enc_pkt;
    AVFrame *filtered_frame;
} FilteringContext;

class PushRTMPUtil{
public:
    PushRTMPUtil(){}
    ~PushRTMPUtil(){
        for (int i = 0; i < ifmt_ctx->nb_streams; i++) {
            avcodec_free_context(&stream_ctx[i].dec_ctx);
            if (ofmt_ctx && ofmt_ctx->nb_streams > i && ofmt_ctx->streams[i] && stream_ctx[i].enc_ctx)
                avcodec_free_context(&stream_ctx[i].enc_ctx);
            av_frame_free(&stream_ctx[i].dec_frame);
        }
        av_free(stream_ctx);
        avformat_close_input(&ifmt_ctx);
        if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
            avio_closep(&ofmt_ctx->pb);
        avformat_free_context(ofmt_ctx);
    }
    int open_camera();
    int open_input_file(const char *filename);
    int open_output_file(const char *filename);
    int init_in_fmt_cnt();
    int flush_encoder(unsigned int stream_index);
    int filter_encode_write_frame(AVFrame *frame, unsigned int stream_index);
    int encode_write_frame(unsigned int stream_index, int flush);
    int init_filters(void);
    int init_filter(FilteringContext* fctx, AVCodecContext *dec_ctx,
                    AVCodecContext *enc_ctx, const char *filter_spec);
    AVFormatContext *ifmt_ctx = NULL;
    AVFormatContext *ofmt_ctx = NULL;
    StreamContext *stream_ctx = NULL;
    FilteringContext *filter_ctx = NULL;
    int video_index = -1;
};


#endif //CAMERATORTMP_PUSHRTMP_H
