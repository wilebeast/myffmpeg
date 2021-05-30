//
// Created by bytedance on 2021/5/29.
// https://juejin.cn/post/6844903732891615246
//

#include <stdio.h>
#include <libavutil/log.h>
#include <libavformat/avio.h>
#include <libavformat/avformat.h>

#define ERROR_STR_SIZE 1024

int main(int argc, char *argv[]) {
    int err_code;
    char errors[1024];

    char *src_filename = NULL;
    char *dst_filename = NULL;

    int audio_stream_index;

    //上下文
    AVFormatContext *fmt_ctx = NULL;
    AVFormatContext *ofmt_ctx = NULL;

    //支持各种各样的输出文件格式，MP4，FLV，3GP等等
    const AVOutputFormat *output_fmt = NULL;

    AVStream *in_stream = NULL;
    AVStream *out_stream = NULL;

    AVPacket pkt;

    av_log_set_level(AV_LOG_DEBUG);

    if (argc < 3) {
        av_log(NULL, AV_LOG_DEBUG, "argc < 3！\n");
        return -1;
    }

    src_filename = argv[1];
    dst_filename = argv[2];

    if (src_filename == NULL || dst_filename == NULL) {
        av_log(NULL, AV_LOG_DEBUG, "src or dts file is null!\n");
        return -1;
    }


    if ((err_code = avformat_open_input(&fmt_ctx, src_filename, NULL, NULL)) < 0) {
        av_strerror(err_code, errors, 1024);
        av_log(NULL, AV_LOG_DEBUG, "打开输入文件失败: %s, %d(%s)\n",
               src_filename,
               err_code,
               errors);
        return -1;
    }

    if ((err_code = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
        av_strerror(err_code, errors, 1024);
        av_log(NULL, AV_LOG_DEBUG, "failed to find stream info: %s, %d(%s)\n",
               src_filename,
               err_code,
               errors);
        return -1;
    }

    av_dump_format(fmt_ctx, 0, src_filename, 0);

    if (fmt_ctx->nb_streams < 2) {
        //流数小于2，说明这个文件音频、视频流这两条都不能保证，输入文件有错误
        av_log(NULL, AV_LOG_ERROR, "输入文件错误，流不足2条\n");
        exit(1);
    }

    //拿到文件中音频流
    /**只需要修改这里AVMEDIA_TYPE_VIDEO参数**/
    audio_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO/*AVMEDIA_TYPE_VIDEO*/, -1,
                                             -1, NULL, 0);
    if (audio_stream_index < 0) {
        av_log(NULL, AV_LOG_DEBUG, " 获取音频流失败%s,%s\n",
               av_get_media_type_string(AVMEDIA_TYPE_VIDEO),
               src_filename);
        return AVERROR(EINVAL);
    }

    in_stream = fmt_ctx->streams[audio_stream_index];
    //参数信息
    AVCodecParameters *in_codecpar = in_stream->codecpar;


    // 输出上下文
    ofmt_ctx = avformat_alloc_context();

    //根据目标文件名生成最适合的输出容器
    output_fmt = av_guess_format(NULL, dst_filename, NULL);
    if (!output_fmt) {
        av_log(NULL, AV_LOG_DEBUG, "根据目标生成输出容器失败！\n");
        exit(1);
    }

    ofmt_ctx->oformat = output_fmt;

    //新建输出流
    out_stream = avformat_new_stream(ofmt_ctx, NULL);
    if (!out_stream) {
        av_log(NULL, AV_LOG_DEBUG, "创建输出流失败！\n");
        exit(1);
    }

    // 将参数信息拷贝到输出流中，我们只是抽取音频流，并不做音频处理，所以这里只是Copy
    if ((err_code = avcodec_parameters_copy(out_stream->codecpar, in_codecpar)) < 0) {
        av_strerror(err_code, errors, ERROR_STR_SIZE);
        av_log(NULL, AV_LOG_ERROR,
               "拷贝编码参数失败！, %d(%s)\n",
               err_code, errors);
    }

    out_stream->codecpar->codec_tag = 0;

    //初始化AVIOContext,文件操作由它完成
    if ((err_code = avio_open(&ofmt_ctx->pb, dst_filename, AVIO_FLAG_WRITE)) < 0) {
        av_strerror(err_code, errors, 1024);
        av_log(NULL, AV_LOG_DEBUG, "文件打开失败 %s, %d(%s)\n",
               dst_filename,
               err_code,
               errors);
        exit(1);
    }



    av_dump_format(ofmt_ctx, 0, dst_filename, 1);


    //初始化 AVPacket， 我们从文件中读出的数据会暂存在其中
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;


    // 写头部信息
    if (avformat_write_header(ofmt_ctx, NULL) < 0) {
        av_log(NULL, AV_LOG_DEBUG, "写入头部信息失败！");
        exit(1);
    }

    //每读出一帧数据
    while (av_read_frame(fmt_ctx, &pkt) >= 0) {
        if (pkt.stream_index == audio_stream_index) {
            pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base,
                                       (AV_ROUND_NEAR_INF |
                                       AV_ROUND_PASS_MINMAX));
            pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base,
                                       (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

            pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
            pkt.pos = -1;
            pkt.stream_index = 0;
            //将包写到输出媒体文件
            av_interleaved_write_frame(ofmt_ctx, &pkt);
            //减少引用计数，避免内存泄漏
            av_packet_unref(&pkt);
        }
    }

    //写尾部信息
    av_write_trailer(ofmt_ctx);

    //最后别忘了释放内存
    avformat_close_input(&fmt_ctx);
    avio_close(ofmt_ctx->pb);

    return 0;
}

