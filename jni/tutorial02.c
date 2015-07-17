#include <jni.h>
#include <GLES/gl.h>
#include <GLES/glext.h>


#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>

#include <stdio.h>
#include <android/log.h>


#define LOG_TAG ("ndkTest")

#define LOGD(... ) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define LOGE(... ) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

GLuint texture;
int g_nPandaWidth;
int g_nPandaHeight;


void Java_com_example_jhjh_ndktest_MainActivity_nativeCreated(JNIEnv* env, jobject thiz){
	glClearColor(0.4f, 0.4f, 0.4f, 0.4f);
	glGenTextures(1, &texture);
}


void Java_com_example_jhjh_ndktest_MainActivity_nativeChanged(JNIEnv* env, jobject thiz, jint w, jint h){
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrthof(0.0f, w, h, 0.0f, 1.0f, -1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glViewport(0,0,w,h);

    g_nPandaWidth = w;
    g_nPandaHeight = h;
}


jstring Java_com_example_jhjh_ndktest_MainActivity_ffmepgTest( JNIEnv* env, jobject thiz, jstring jStrFile, jstring jStrPath)
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

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // is this a packet from the video stream?
        if(packet.stream_index == videoStream){
            // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

            // Did we get a video frame ?
            if(frameFinished){

                // Convert the image from its native format to RGB
                sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
                                    pFrame->linesize,
                                    0,
                                    pCodecCtx->height,
                                    pFrameRGB->data,
                                    pFrameRGB->linesize
                );

                LOGE("Textured %d", i++);

                glBindTexture(GL_TEXTURE_2D, texture);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, g_nPandaWidth, g_nPandaHeight, GL_RGBA, GL_UNSIGNED_BYTE, pFrameRGB->data[0]);

                int g_nX = 0;
                            int g_nY = 0;
                            GLfloat vertices[12] = {
                            			g_nX				,	g_nY+g_nPandaHeight	,	0.0f,	// LEFT  | BOTTOM
                            			g_nX+g_nPandaWidth	,	g_nY+g_nPandaHeight	,	0.0f,	// RIGHT | BOTTOM
                            			g_nX				,	g_nY				,	0.0f,	// LEFT  | TOP
                            			g_nX+g_nPandaWidth	,	g_nY				,	0.0f	// RIGHT | TOP
                            	};

                            	GLfloat texture[8] = {
                            			0	, 1,
                            			1	, 1,
                            			0	, 0,
                            			1	, 0
                            	};


                            	glEnable(GL_TEXTURE_2D);

                            	glBindTexture(GL_TEXTURE_2D, texture);

                                glEnableClientState(GL_VERTEX_ARRAY);
                            	glVertexPointer(3, GL_FLOAT, 0, vertices);

                                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                            	glTexCoordPointer(2, GL_FLOAT, 0, texture);
                            	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                                glDisableClientState(GL_VERTEX_ARRAY);

                            	glDisable(GL_TEXTURE_2D);

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
























