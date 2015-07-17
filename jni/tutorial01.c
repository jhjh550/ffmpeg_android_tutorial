#include <jni.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>

#include <stdio.h>
#include <android/log.h>


#define LOG_TAG ("ndkTest")

#define LOGD(... ) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define LOGE(... ) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))




jstring Java_com_example_jhjh_ndktest_tutorial01_ffmepgTest
( JNIEnv* env, jobject thiz, jstring jStrFile, jstring jStrPath)
{

    char* filepath = (char*)(*env)->GetStringUTFChars(env, jStrFile, NULL);
    char* dirpath = (char*)(*env)->GetStringUTFChars(env, jStrPath, NULL);

    AVFormatContext *pFormatCtx = NULL;


    ///////////////////////////////////////////////
    // OPENNING THE FILE
    ///////////////////////////////////////////////

    av_register_all();



    // open video file
    if(avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0){
        return (*env)->NewStringUTF(env, filepath);
    }


    if(avformat_find_stream_info(pFormatCtx, NULL)<0)
        return (*env)->NewStringUTF(env, "couldn't find stream information");


    // Dump information about file onto standard error
    av_dump_format(pFormatCtx, 0, filepath, 0);


    int i;
    //AVCodecContext *pCodecCtxOrig = NULL;
    AVCodecContext *pCodecCtx = NULL;

    // Find the first video stream
    int videoStream = -1;

    for(i=0; i<pFormatCtx->nb_streams; i++){
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            videoStream = i;
            break;
        }
    }


    if(videoStream == -1)
        return (*env)->NewStringUTF(env, "Didn't find a video stream");




    AVCodec *pCodec = NULL;

    // Get a pointer to the codec context for the video stream
    pCodecCtx=pFormatCtx->streams[videoStream]->codec;

    // Find the decoder for the video stream
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodecCtx == NULL){
        return (*env)->NewStringUTF(env, "Unsupported codec !!!");
    }



    // OPEN CODEC
    if(avcodec_open2(pCodecCtx, pCodec, NULL)<0){
        return (*env)->NewStringUTF(env, "Could not open codec !");
    }


    ///////////////////////////////////////////////
    // STORING THE DATA
    ///////////////////////////////////////////////

    AVFrame* pFrame = NULL;
    AVFrame* pFrameRGB = NULL;

    // Allocate video frame
    pFrame = av_frame_alloc();

    // Allocate an AVFrame structure
    pFrameRGB = av_frame_alloc();
    if(pFrameRGB == NULL)
        return (*env)->NewStringUTF(env, "Couldn't allocate AVFRame");

    uint8_t *buffer = NULL;
    int numBytes;

    // Determine required buffer size and allocate buffer
    numBytes = avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
    buffer = (uint8_t*)av_malloc(numBytes*sizeof(uint8_t));

    avpicture_fill((AVPicture*)pFrameRGB, buffer, PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);


    ///////////////////////////////////////////////
    // READING THE DATA
    ///////////////////////////////////////////////
    struct SwsContext *sws_ctx = NULL;
    int frameFinished;
    AVPacket packet;

    // initialize SWS context for software scaling
    sws_ctx = sws_getContext(pCodecCtx->width,
                        pCodecCtx->height,
                        pCodecCtx->pix_fmt,
                        pCodecCtx->width,
                        pCodecCtx->height,
                        PIX_FMT_RGB24,
                        SWS_BILINEAR,
                        NULL,
                        NULL,
                        NULL
    );

    i=0;
    while(av_read_frame(pFormatCtx, &packet)>=0){
        // is this a packet from the video stream?
        if(packet.stream_index == videoStream){
            // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

            // Did we get a video frame ?
            if(frameFinished){

                double pts;
                if(packet.pts != AV_NOPTS_VALUE){
                    pts = av_frame_get_best_effort_timestamp(pFrame);
                }else{
                    pts = 0;
                }

                pts *= av_q2d(pFormatCtx->streams[videoStream]->time_base);



                // Convert the image from its native format to RGB
                sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
                                    pFrame->linesize,
                                    0,
                                    pCodecCtx->height,
                                    pFrameRGB->data,
                                    pFrameRGB->linesize
                );


                // save the frame to disk
                if(++i<=5){
                    FILE* pFile;
                    char szFilename[256];
                    int y;


                    //return (*env)->NewStringUTF(env, "Save Frame Failed");


                    // Open file
                    sprintf(szFilename, "%sframe%d.ppm", dirpath, i);


                    pFile = fopen(szFilename, "wb");
                    if(pFile == NULL)
                        return (*env)->NewStringUTF(env, "Save Frame Failed");



                    // Write header
                    fprintf(pFile, "P6\n%d %d\n255\n", pCodecCtx->width, pCodecCtx->height);

                    // Write pixel data
                    for(y=0; y<pCodecCtx->height; y++)
                        fwrite(pFrameRGB->data[0]+y*pFrameRGB->linesize[0], 1, pCodecCtx->width*3, pFile);

                    // Close file
                    fclose(pFile);
                }
            }
        }

        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }

    // Free the RGB image
    av_free(buffer);
    av_free(pFrameRGB);


    // Free the YUV frame
    av_free(pFrame);


    // Close the codecs
    avcodec_close(pCodecCtx);


    // Close the video file
    avformat_close_input(&pFormatCtx);




    (*env)->ReleaseStringUTFChars(env, jStrFile, filepath);
    (*env)->ReleaseStringUTFChars(env, jStrPath, dirpath);


    return (*env)->NewStringUTF(env, "Program ended successfully");
}
























