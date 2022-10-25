#include <stdio.h>
extern "C"
{
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
};

#include "pushRTMP.h"

int main(int argc, char* argv[])
{
    int ret, i;
    int frame_index=0;
    int64_t start_time = 0;

    PushRTMPUtil* pushRtmpUtil_ptr =new PushRTMPUtil();

    AVPacket* packet ;

    const char *in_filename = "PatrickStar.mp4";
//    const char *out_filename = "rtmp://47.98.102.250/live/livestream";
//    const char *out_filename = "output.flv";
    const char *out_filename = "rtmp://localhost/live/livestream";

    //Network，网络流初始化
    avformat_network_init();
    //输入（Input）
    if (ret = pushRtmpUtil_ptr->open_input_file(in_filename) < 0) {
        printf( "Could not open input file.");
        goto end;
    }
    if (ret = pushRtmpUtil_ptr->open_output_file(out_filename) < 0) {
        printf( "Could not open output file.");
        goto end;
    }
    if ((ret = pushRtmpUtil_ptr->init_filters()) < 0)
        goto end;
    if (!(packet = av_packet_alloc()))
        goto end;

    start_time = av_gettime();
    unsigned int stream_index;
    while (1) {

//        while(ret = av_read_frame(pushRtmpUtil_ptr->ifmt_ctx, packet) >= 0){
        ret = av_read_frame(pushRtmpUtil_ptr->ifmt_ctx, packet);
        if (ret)
            break;
            stream_index = packet->stream_index;
            av_log(nullptr, AV_LOG_DEBUG, "Demuxer gave frame of stream_index %u\n",
                   stream_index);

            if (pushRtmpUtil_ptr->filter_ctx[stream_index].filter_graph) {
                StreamContext *stream = &pushRtmpUtil_ptr->stream_ctx[stream_index];

                av_log(nullptr, AV_LOG_DEBUG, "Going to reencode&filter the frame\n");

                av_packet_rescale_ts(packet,
                                     pushRtmpUtil_ptr->ifmt_ctx->streams[stream_index]->time_base,
                                     stream->dec_ctx->time_base);
                ret = avcodec_send_packet(stream->dec_ctx, packet);
                if (ret < 0) {
                    av_log(nullptr, AV_LOG_ERROR, "Decoding failed\n");
                    break;
                }

                while (ret >= 0) {
                    ret = avcodec_receive_frame(stream->dec_ctx, stream->dec_frame);
                    if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                        break;
                    else if (ret < 0)
                        goto end;

                    stream->dec_frame->pts = stream->dec_frame->best_effort_timestamp;
                    ret = pushRtmpUtil_ptr->filter_encode_write_frame(stream->dec_frame, stream_index);
                    if (ret < 0)
                        goto end;
                }
            } else {
                /* remux this frame without reencoding */
                av_packet_rescale_ts(packet,
                                     pushRtmpUtil_ptr->ifmt_ctx->streams[stream_index]->time_base,
                                     pushRtmpUtil_ptr->ofmt_ctx->streams[stream_index]->time_base);

                ret = av_interleaved_write_frame(pushRtmpUtil_ptr->ofmt_ctx, packet);
                if (ret < 0)
                    goto end;
            }
            av_packet_unref(packet);
//        }
//        if (ret)
//            break;
    }
    //写文件尾（Write file trailer）
    for (i = 0; i < pushRtmpUtil_ptr->ifmt_ctx->nb_streams; i++) {
        /* flush filter */
        if (!pushRtmpUtil_ptr->filter_ctx[i].filter_graph)
            continue;
        ret = pushRtmpUtil_ptr->filter_encode_write_frame(nullptr, i);
        if (ret < 0) {
            av_log(nullptr, AV_LOG_ERROR, "Flushing filter failed\n");
            goto end;
        }

        /* flush encoder */
        ret = pushRtmpUtil_ptr->flush_encoder(i);
        if (ret < 0) {
            av_log(nullptr, AV_LOG_ERROR, "Flushing encoder failed\n");
            goto end;
        }
    }

    av_write_trailer(pushRtmpUtil_ptr->ofmt_ctx);
    end:
    if (ret < 0)
        av_log(nullptr, AV_LOG_ERROR, "Error occurred: %s\n", av_err2str(ret));

    return ret ? 1 : 0;
}
 
 