extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/log.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
}

extern "C" void update_progress(float progress);

extern "C"
int ffmpeg_main(int argc, char** argv) {
    AVFormatContext* inputCtx = nullptr;
    AVFormatContext* outputCtx = nullptr;
    AVPacket pkt;
    int ret;

    const char* input = nullptr;
    const char* output = nullptr;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-i") && i + 1 < argc) input = argv[++i];
        else if (i == argc - 1) output = argv[i];
    }

    if (!input || !output) return -1;

    av_log_set_level(AV_LOG_INFO);
    avformat_network_init();

    AVDictionary* options = nullptr;
    av_dict_set(&options, "genpts", "1", 0);
    av_dict_set(&options, "fflags", "+genpts+igndts+discardcorrupt", 0);
    av_dict_set_int(&options, "analyzeduration", 10000000, 0);
    av_dict_set_int(&options, "probesize", 10000000, 0);

    if ((ret = avformat_open_input(&inputCtx, input, nullptr, &options)) < 0) {
        av_log(nullptr, AV_LOG_ERROR, "❌ Could not open input: %s\n", av_err2str(ret));
        av_dict_free(&options);
        return ret;
    }
    av_dict_free(&options);

    if ((ret = avformat_find_stream_info(inputCtx, nullptr)) < 0) {
        av_log(nullptr, AV_LOG_ERROR, "❌ Failed to find stream info: %s\n", av_err2str(ret));
        return ret;
    }

    int64_t input_duration_us = inputCtx->duration;
    if (input_duration_us <= 0) {
        // fallback: estimate from longest stream
        input_duration_us = 0;
        for (unsigned i = 0; i < inputCtx->nb_streams; i++) {
            AVStream* st = inputCtx->streams[i];
            if (st->duration != AV_NOPTS_VALUE) {
                int64_t dur_us = av_rescale_q(st->duration, st->time_base, {1, AV_TIME_BASE});
                if (dur_us > input_duration_us)
                    input_duration_us = dur_us;
            }
        }
    }

    int64_t global_start_us = INT64_MAX;
    for (unsigned i = 0; i < inputCtx->nb_streams; i++) {
        if (inputCtx->streams[i]->start_time != AV_NOPTS_VALUE) {
            int64_t start_us = av_rescale_q(inputCtx->streams[i]->start_time, inputCtx->streams[i]->time_base, {1, AV_TIME_BASE});
            if (start_us < global_start_us)
                global_start_us = start_us;
        }
    }
    if (global_start_us == INT64_MAX) global_start_us = 0;

    if ((ret = avformat_alloc_output_context2(&outputCtx, nullptr, "mp4", output)) < 0) {
        av_log(nullptr, AV_LOG_ERROR, "❌ Could not create output context: %s\n", av_err2str(ret));
        return ret;
    }

    outputCtx->flags |= AVFMT_FLAG_GENPTS | AVFMT_FLAG_FLUSH_PACKETS;
    outputCtx->max_interleave_delta = 1000000;

    int video_index = -1;
    int* stream_mapping = (int*)av_malloc_array(inputCtx->nb_streams, sizeof(*stream_mapping));
    if (!stream_mapping) return AVERROR(ENOMEM);
    memset(stream_mapping, -1, inputCtx->nb_streams * sizeof(*stream_mapping));

    int fake_pts_counter[64] = {0};

    int64_t last_dts[64] = {AV_NOPTS_VALUE};
    int64_t first_video_dts_us = AV_NOPTS_VALUE;
    int64_t audio_offset_us = 0;
    int64_t last_video_pkt_duration = 0;

    for (unsigned i = 0; i < inputCtx->nb_streams; i++) {
        AVStream* in_stream = inputCtx->streams[i];
        AVCodecParameters* in_codecpar = in_stream->codecpar;

        if (in_codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE)
            continue;

        // ✅ Set the video_index only once (first video stream)
        if (in_codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_index == -1)
            video_index = i;

        AVStream* out_stream = avformat_new_stream(outputCtx, nullptr);
        if (!out_stream) {
            av_log(nullptr, AV_LOG_ERROR, "❌ Failed to create output stream\n");
            av_free(stream_mapping);
            return AVERROR_UNKNOWN;
        }

        if ((ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar)) < 0) {
            av_log(nullptr, AV_LOG_ERROR, "❌ Failed to copy codec parameters\n");
            av_free(stream_mapping);
            return ret;
        }

        if (in_codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            out_stream->time_base = (AVRational){1, 90000};
        else if (in_codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            out_stream->time_base = (AVRational){1, 48000};
        else
            out_stream->time_base = in_stream->time_base;

        stream_mapping[i] = out_stream->index;
    }

    if (!(outputCtx->oformat->flags & AVFMT_NOFILE)) {
        if ((ret = avio_open(&outputCtx->pb, output, AVIO_FLAG_WRITE)) < 0) {
            av_log(nullptr, AV_LOG_ERROR, "❌ Could not open output file: %s\n", av_err2str(ret));
            av_free(stream_mapping);
            return ret;
        }
    }

    AVDictionary* mux_opts = nullptr;
    av_dict_set(&mux_opts, "movflags", "faststart+write_colr+default_base_moof", 0);
    av_dict_set(&mux_opts, "copytb", "1", 0);
    av_dict_set(&mux_opts, "avoid_negative_ts", "make_zero", 0);

    if ((ret = avformat_write_header(outputCtx, &mux_opts)) < 0) {
        av_log(nullptr, AV_LOG_ERROR, "❌ Error writing header: %s\n", av_err2str(ret));
        av_dict_free(&mux_opts);
        av_free(stream_mapping);
        return ret;
    }
    av_dict_free(&mux_opts);

    while ((ret = av_read_frame(inputCtx, &pkt)) >= 0) {
        if (pkt.size <= 0 || !pkt.data) {
            av_packet_unref(&pkt);
            continue;
        }

        int in_index = pkt.stream_index;
        int out_index = stream_mapping[in_index];
        if (out_index < 0) {
            av_packet_unref(&pkt);
            continue;
        }

        AVStream* in_stream = inputCtx->streams[in_index];
        AVStream* out_stream = outputCtx->streams[out_index];
        AVCodecParameters* in_codecpar = in_stream->codecpar;

        int64_t raw_pts = (pkt.pts == AV_NOPTS_VALUE) ? pkt.dts : pkt.pts;
        int64_t raw_dts = (pkt.dts == AV_NOPTS_VALUE) ? pkt.pts : pkt.dts;

        if (raw_pts == AV_NOPTS_VALUE || raw_dts == AV_NOPTS_VALUE)
            raw_pts = raw_dts = fake_pts_counter[in_index]++;

        int64_t stream_start_us = (in_stream->start_time != AV_NOPTS_VALUE)
                                  ? av_rescale_q(in_stream->start_time, in_stream->time_base, {1, AV_TIME_BASE})
                                  : 0;

        int64_t pts_us = av_rescale_q(raw_pts, in_stream->time_base, {1, AV_TIME_BASE}) - stream_start_us;
        int64_t dts_us = av_rescale_q(raw_dts, in_stream->time_base, {1, AV_TIME_BASE}) - stream_start_us;

        // ✅ Progress update
        if (input_duration_us > 0 && dts_us >= 0 && in_codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            static int64_t last_update_time_us = 0;
            int64_t now = av_gettime_relative();
            if (now - last_update_time_us > 500000) { // update every 0.5 seconds
                float progress = (float)dts_us / input_duration_us;
                if (progress > 1.0f) progress = 1.0f;
                update_progress(progress);
                last_update_time_us = now;
            }
        }

        // ✨ Audio Sync Fix: align to first video timestamp
        if (in_codecpar->codec_type == AVMEDIA_TYPE_VIDEO && first_video_dts_us == AV_NOPTS_VALUE) {
            first_video_dts_us = dts_us;
        }
        if (in_codecpar->codec_type == AVMEDIA_TYPE_AUDIO && first_video_dts_us != AV_NOPTS_VALUE) {
            audio_offset_us = first_video_dts_us;
            dts_us -= audio_offset_us;
            pts_us -= audio_offset_us;
            if (pts_us < 0) pts_us = 0;
            if (dts_us < 0) dts_us = 0;
        }

        // Rescale to output time base
        int64_t out_pts = av_rescale_q(pts_us, {1, AV_TIME_BASE}, out_stream->time_base);
        int64_t out_dts = av_rescale_q(dts_us, {1, AV_TIME_BASE}, out_stream->time_base);

        // CFR Logic for video
        if (in_codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            AVRational fr = in_stream->avg_frame_rate.num > 0 ? in_stream->avg_frame_rate : (AVRational){25, 1};
            int64_t step = av_rescale_q(1, av_inv_q(fr), out_stream->time_base);
            if (last_dts[in_index] >= 0) {
                out_dts = last_dts[in_index] + step;
                if (out_pts < out_dts) out_pts = out_dts;
            } else {
                out_pts = out_dts = 0;
            }
        }

        if (last_dts[in_index] != AV_NOPTS_VALUE && out_dts <= last_dts[in_index]) {
            out_dts = last_dts[in_index] + 1;
            if (out_pts < out_dts) out_pts = out_dts;
        }
        last_dts[in_index] = out_dts;

        pkt.pts = out_pts;
        pkt.dts = out_dts;
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        if (pkt.duration <= 0) pkt.duration = 1;

        pkt.stream_index = out_index;
        pkt.pos = -1;

        if ((ret = av_interleaved_write_frame(outputCtx, &pkt)) < 0) {
            av_log(nullptr, AV_LOG_ERROR, "❌ Error writing packet: %s\n", av_err2str(ret));
            av_packet_unref(&pkt);
            break;
        }

        if (pkt.stream_index == video_index)
            last_video_pkt_duration = pkt.duration;

        av_packet_unref(&pkt);
    }

    if (video_index >= 0 && last_dts[video_index] != AV_NOPTS_VALUE) {
        AVStream* video_stream = outputCtx->streams[video_index];
        int64_t final_duration = last_dts[video_index] + last_video_pkt_duration;

        outputCtx->duration = av_rescale_q(final_duration,
                                           video_stream->time_base,
                                           AV_TIME_BASE_Q);

        av_log(nullptr, AV_LOG_INFO, "📏 Patched duration: %" PRId64 " us\n", outputCtx->duration);
    }


    av_write_trailer(outputCtx);
    avformat_close_input(&inputCtx);
    if (!(outputCtx->oformat->flags & AVFMT_NOFILE)) avio_closep(&outputCtx->pb);
    avformat_free_context(outputCtx);
    av_free(stream_mapping);

    return 0;
}
