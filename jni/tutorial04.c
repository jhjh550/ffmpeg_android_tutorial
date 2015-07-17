#include <jni.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>


#include <stdio.h>
#include <android/log.h>


#define LOG_TAG ("video_sync")
#define SWR_CH_MAX 32

#define LOGD(... ) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define LOGE(... ) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

#include <pthread.h>



// Handle to a program object
GLuint programObject;


// Attribute location
GLint positionLoc;
GLint texCoordLoc;

// Sampler location
GLint samplerLocY;
GLint samplerLocU;
GLint samplerLocV;


// Texture handle
GLuint textureIdYUV[3];

AVFrame* pDrawFrame = NULL;



static void checkError(const char* msg){

	GLint error ;
	for (error = glGetError(); error; error = glGetError())	{
		LOGD("after %s() glError (0x%x)---\n", msg,error);
	}
}



GLuint loadShader(GLuint type, const char* source){
	GLuint shader = glCreateShader(type);
	LOGD("glCreateShader result is : (%d)\n", shader);
	checkError("glCreateShader");


	if (shader)	{

		glShaderSource(shader, 1, &source, NULL);
		checkError("glShaderSource");

		glCompileShader(shader);
		checkError("glCompileShader");

		GLint compiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		checkError("glGetShaderiv");

		if (!compiled)		{

			GLint infoLen = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
			checkError("glGetShaderiv");

			if (infoLen){

				char *buf = (char*)malloc(infoLen);
				if (buf){

					glGetShaderInfoLog(shader, infoLen, NULL, buf);
					checkError("glGetShaderInfoLog");
					LOGE("can not compile shader %d:%s\n", type, buf);
					free(buf);
				}

				glDeleteShader(shader);
				shader = 0;
			}
		}
	}

	return shader;
}


GLuint createProgram(const char* vertex, const char* fragment){

	GLuint program = 0;
	GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertex);

	if (vertexShader == 0){
		LOGE("error load vertex shader : vertexShader (%d) ==\n", vertexShader);
		return 0;
	}

	GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER,fragment);

	if (fragmentShader == 0){
		LOGE("error load fragment shader : fragmentShader (%d) --\n", fragmentShader);
		return 0;
	}


	program = glCreateProgram();

	if (program){

		// shader attach
		glAttachShader(program, vertexShader);
		checkError("glAttachShader");


		glAttachShader(program, fragmentShader);
		checkError("glAttachShader");

		// link program handle
		glLinkProgram(program);
		GLint linkStatus = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

		if (linkStatus != GL_TRUE){

			LOGE("Error glGetProgramiv -----------------------\n");
			GLint linkLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &linkLength);

			if (linkLength){
				char *buffer = (char*) malloc(linkLength);
				if (buffer){
					glGetProgramInfoLog(program, linkLength, NULL, buffer);
					LOGE("can not link program : %s\n", buffer);
					free(buffer);
				}


			//	glDeleteProgram(program);
			//	program = 0;

			}

			glDeleteProgram(program);
			program = 0;
		}
	}

	return program;
}


int OpenGLInit(){

	GLbyte vShaderStr[] =
      "attribute vec4 a_position;   \n"
      "attribute vec2 a_texCoord;   \n"
      "varying vec2 v_texCoord;     \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position; \n"
      "   v_texCoord = a_texCoord;  \n"
      "}                            \n";

   GLbyte fShaderStr[] =
      "varying vec2 v_texCoord;                            		\n"

      "uniform sampler2D sampler0;                        		\n"
      "uniform sampler2D sampler1;                        		\n"
      "uniform sampler2D sampler2;                        		\n"
      "void main()                                         		\n"
      "{                                                   		\n"
      	"mediump float y = texture2D(sampler0, v_texCoord).r;	\n"
		"mediump float u = texture2D(sampler1, v_texCoord).r;	\n"
		"mediump float v = texture2D(sampler2, v_texCoord).r;	\n"

		"y = 1.1643 * (y - 0.0625);								\n"
		"u = u - 0.5;											\n"
		"v = v - 0.5;											\n"


		"mediump float r = y + 1.5958 * v;						\n"
		"mediump float g = y - 0.39173 * u - 0.81290 * v;		\n"
		"mediump float b = y + 2.017 * u;						\n"


		"gl_FragColor = vec4(r, g, b, 1.0);						\n"

      "}                                                   		\n";

	// highp vs mediump


	// Load the shaders and get a linked program object
	programObject = createProgram( vShaderStr, fShaderStr);

	// Get the attribute location
	positionLoc = glGetAttribLocation( programObject, "a_position");
	texCoordLoc = glGetAttribLocation( programObject, "a_texCoord");

	// Get the sampler location
	samplerLocY = glGetUniformLocation( programObject, "sampler0");
	samplerLocU = glGetUniformLocation( programObject, "sampler1");
	samplerLocV = glGetUniformLocation( programObject, "sampler2");


	glClearColor(0.1,0.1,0.1,0);

	glGenTextures(3, textureIdYUV);
	glBindTexture(GL_TEXTURE_2D, textureIdYUV[0]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    glBindTexture(GL_TEXTURE_2D, textureIdYUV[1]);


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);



    glBindTexture(GL_TEXTURE_2D, textureIdYUV[2]);



	// Set filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


	return GL_TRUE;
}


void setup(int width, int height){
	glViewport(0, 0, width, height);
	LOGD("set up view port %d %d", width, height);
}





typedef struct PacketQueue{
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
} PacketQueue;




typedef struct VideoState{
    AVFrame             *frame;

    AVFormatContext     *foramtCtx;

    AVStream            *audioStream;
    AVCodecContext      *audioCtx;
    struct SwrContext   *swrCtx;    

    AVStream            *videoStream;
    AVCodecContext      *videoCtx;

    PacketQueue audioq;
    PacketQueue videoq;

    double 				pts;
    int64_t				timestamp;
    
} VideoState;

VideoState *is;


void packet_queue_init(PacketQueue* q){
    memset(q, 0, sizeof(PacketQueue));

    pthread_cond_init(&(q->cond), NULL);
    pthread_mutex_init(&(q->mutex), NULL);
}


int packet_queue_put(PacketQueue *q, AVPacket* pkt){

    AVPacketList *pkt1;
    if(av_dup_packet(pkt)<0){
        return -1;
    }

    pkt1 = av_malloc(sizeof(AVPacketList));
    if(!pkt1)
        return -1;

    pkt1->pkt = *pkt;
    pkt1->next = NULL;

    pthread_mutex_lock(&(q->mutex));

    if(!q->last_pkt)
        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;

    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size;


    pthread_cond_signal(&(q->cond));
    pthread_mutex_unlock(&(q->mutex));

    return 0;
}

int quit = 0;
static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block){
    AVPacketList *pkt1;
    int ret;

    pthread_mutex_lock(&(q->mutex));
    for(;;){
        if(quit){
            return -1;
            break;
        }

        pkt1 = q->first_pkt;
        if(pkt1){
            q->first_pkt = pkt1->next;
            if(!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size;
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;

            break;

        }else if(!block){
            ret = 0;
            break;

        }else{
            pthread_cond_wait(&(q->cond), &(q->mutex));
        }
    }

    pthread_mutex_unlock(&(q->mutex));
    return ret;

}



void  Java_com_example_jhjh_ndktest_Tutorial04_nativeGetAudioData
(JNIEnv* env, jobject obj, jbyteArray array){


    jclass cls = (*env)->GetObjectClass(env, obj);
    jmethodID play = (*env)->GetMethodID(env, cls, "PlaySound", "([BI)V");


    int channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    unsigned char *buffer = (unsigned char*)malloc(
        channels * is->audioCtx->sample_rate* av_get_bytes_per_sample(AV_SAMPLE_FMT_S16));

    unsigned char* pointers[SWR_CH_MAX] = {NULL};
    pointers[0] = &buffer[0];
    int total = 0;




     // Create a copy of the packet that we can modify and manipulate
     AVPacket decodingPacket;
     av_init_packet(&decodingPacket);

     packet_queue_get(&(is->audioq), &decodingPacket, 1);

     while (decodingPacket.size > 0)
     {
         // Try to decode the packet into a frame
         int frameFinished = 0;
         int result = avcodec_decode_audio4(is->audioCtx, is->frame, &frameFinished, &decodingPacket);

         if (result < 0 || frameFinished == 0)
         {
            LOGD("result : %d, frameFinished : %d", result, frameFinished);
             break;
         }

         int numSamplesOut = swr_convert(is->swrCtx,
                                         pointers,
                                         is->audioCtx->sample_rate,
                                         (const unsigned char**)is->frame->extended_data,
                                         is->frame->nb_samples);


         int tempBufSize = numSamplesOut * channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);

         jbyte* bytes = (*env)->GetByteArrayElements(env, array, NULL);
         memcpy(bytes, buffer, tempBufSize);
         (*env)->ReleaseByteArrayElements(env, array, bytes, 0);
         (*env)->CallVoidMethod(env, obj, play, array, tempBufSize);



         decodingPacket.size -= result;
         decodingPacket.data += result;

     }


    //LOGD("nativeGetAudioData");
    free(buffer);

}


void cleanupFFMPEG(){

    if(is->frame != NULL){
        av_free(is->frame);
        is->frame = NULL;
    }

    if(is->audioCtx != NULL){
        avcodec_close(is->audioCtx);
        is->audioCtx = NULL;
    }

    if(is->swrCtx != NULL){
        swr_free(&(is->swrCtx));
        is->swrCtx = NULL;
    }

    if(is->foramtCtx != NULL){
        avformat_close_input(&(is->foramtCtx));
        is->foramtCtx = NULL;
    }

    if(is->audioStream != NULL){
        is->audioStream = NULL;
    }

    if(is != NULL){
        av_free(is);
        is = NULL;
    }

}

void Java_com_example_jhjh_ndktest_Tutorial04_nativeCleanupFFMPEG
(JNIEnv* env, jobject obj){

    cleanupFFMPEG();
}



jint Java_com_example_jhjh_ndktest_Tutorial04_nativeInitFFMPEG
(JNIEnv* env, jobject obj, jstring path){

    char* filepath = (char*)(*env)->GetStringUTFChars(env, path, NULL);

    is = av_mallocz(sizeof(VideoState));

    av_register_all();
    is->frame = avcodec_alloc_frame();
	is->timestamp = 0;


    if(!is->frame){
        //"Error allocating the frame"
        cleanupFFMPEG();
        return -1;
    }

    if(avformat_open_input(&(is->foramtCtx), filepath, NULL, NULL)){
        // " File not exist"
        cleanupFFMPEG();
        return -2;
    }
    if(avformat_find_stream_info(is->foramtCtx, NULL)<0){
        // "couldn't find stream information"
        cleanupFFMPEG();
        return -3;
    }


    int i;
    for ( i = 0; i<is->foramtCtx->nb_streams; ++i)
    {
        if (is->foramtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            is->audioStream = is->foramtCtx->streams[i];
        }else if (is->foramtCtx->streams[i]->codec->codec_type ==  AVMEDIA_TYPE_VIDEO){
            is->videoStream = is->foramtCtx->streams[i];
        }

    }


    ///////////////////////////////////////////////////////////////
    // AUDIO OPEN //

    if (is->audioStream == NULL)
    {
        //"Could not find any audio stream in the file"
        cleanupFFMPEG();
        return -4;
    }

    is->audioCtx = is->audioStream->codec;

    is->audioCtx->codec = avcodec_find_decoder(is->audioCtx->codec_id);
    if (is->audioCtx->codec == NULL)
    {
        //"Couldn't find a proper decoder"
        cleanupFFMPEG();
        return -5;
    }
    else if (avcodec_open2(is->audioCtx, is->audioCtx->codec, NULL) != 0)
    {

        // "Couldn't open the context with the decoder"
        cleanupFFMPEG();
        return -6;
    }

    LOGD("This stream has %d channels and a sample rate of %d Hz",
        is->audioCtx->channels ,is->audioCtx->sample_rate);
    LOGD("The data is in the format %s ",
        av_get_sample_fmt_name(is->audioCtx->sample_fmt));



    is->swrCtx = swr_alloc_set_opts(NULL,
                        AV_CH_LAYOUT_STEREO,
                        AV_SAMPLE_FMT_S16,
                        is->audioCtx->sample_rate,
                        av_get_default_channel_layout(is->audioCtx->channels),
                        is->audioCtx->sample_fmt,
                        is->audioCtx->sample_rate,
                        0,
                        NULL);


    if (is->swrCtx == NULL)
    {
        //"Couldn't create the SwrContext"
        cleanupFFMPEG();
        return -5;
    }

    if (swr_init(is->swrCtx) != 0)
    {
        //"Couldn't initialize the SwrContext"
        cleanupFFMPEG();
        return -6;
    }

    packet_queue_init(&(is->audioq));



    ///////////////////////////////////////////////////////////////
    // VIDEO OPEN //
    if (is->videoStream == NULL)
    {
        //"Could not find any video stream in the file"
        cleanupFFMPEG();
        return -7;
    }

    is->videoCtx = is->videoStream->codec;

    is->videoCtx->codec = avcodec_find_decoder(is->videoCtx->codec_id);
    if (is->videoCtx->codec == NULL)
    {
        //"Couldn't find a proper video decoder"
        cleanupFFMPEG();
        return -8;
    }
    else if (avcodec_open2(is->videoCtx, is->videoCtx->codec, NULL) != 0)
    {

        // "Couldn't open the context with the decoder"
        cleanupFFMPEG();
        return -9;
    }

    packet_queue_init(&(is->videoq));

    (*env)->ReleaseStringUTFChars(env, path, filepath);
    return is->audioCtx->sample_rate;

}


jint Java_com_example_jhjh_ndktest_Tutorial04_nativeStartQueuePacket
(JNIEnv* env, jobject obj)
{


    AVPacket packet;
    av_init_packet(&packet);

	for(;;){

		if(is->videoq.nb_packets < 50){
			if( av_read_frame(is->foramtCtx, &packet) != 0){
				break;
			}else{
				if (packet.stream_index == is->audioStream->index){
					packet_queue_put(&(is->audioq), &packet);

				}else if (packet.stream_index == is->videoStream->index){
					packet_queue_put(&(is->videoq), &packet);

				}
			}
		}
    }

   LOGD("StartAudioPlay END");
   return 0;

}



jint Java_com_example_jhjh_ndktest_Tutorial04_nativeInitRenderer
(JNIEnv* env, jobject obj)
{
    OpenGLInit();
}


jint Java_com_example_jhjh_ndktest_Tutorial04_nativeChangeView
(JNIEnv* env, jobject obj, jint width, jint height)
{
    setup(width, height);
}

int flag = 0;
void draw(){

	if(pDrawFrame == NULL)
		return;

	GLfloat vVertices[] = { -1.0f,  1.0f, 0.0f,  // Position 0
                            0.0f,  0.0f,        // TexCoord 0

                           -1.0f, -1.0f, 0.0f,  // Position 1
                            0.0f,  1.0f,        // TexCoord 1

                            1.0f, -1.0f, 0.0f,  // Position 2
                            1.0f,  1.0f,        // TexCoord 2

                            1.0f,  1.0f, 0.0f,  // Position 3
                            1.0f,  0.0f         // TexCoord 3

                         };
   	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

	glClear( GL_COLOR_BUFFER_BIT);


	// Use the program object
	glUseProgram( programObject);

	// Load the vertex position
	glVertexAttribPointer( positionLoc, 3, GL_FLOAT, GL_FALSE, 5* sizeof(GLfloat), vVertices);

	// Load the texture coordinate
	glVertexAttribPointer( texCoordLoc, 2, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), &vVertices[3]);




	glEnableVertexAttribArray( positionLoc);
	glEnableVertexAttribArray( texCoordLoc);


	if(flag ==0){
		glActiveTexture( GL_TEXTURE0);
		glBindTexture( GL_TEXTURE_2D, textureIdYUV[0]);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, is->videoCtx->width,
			is->videoCtx->height, 0,
			GL_LUMINANCE, GL_UNSIGNED_BYTE, pDrawFrame->data[0]);

		glUniform1i( samplerLocY, 0);



		glActiveTexture( GL_TEXTURE1);
		glBindTexture( GL_TEXTURE_2D, textureIdYUV[1]);


		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,
			is->videoCtx->width/2, is->videoCtx->height/2, 0,
			GL_LUMINANCE, GL_UNSIGNED_BYTE, pDrawFrame->data[1]);

		glUniform1i( samplerLocU, 1);



		glActiveTexture( GL_TEXTURE2);
		glBindTexture( GL_TEXTURE_2D, textureIdYUV[2]);


		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,
			is->videoCtx->width/2, is->videoCtx->height/2, 0,
				GL_LUMINANCE, GL_UNSIGNED_BYTE, pDrawFrame->data[2]);

		glUniform1i( samplerLocV, 2);

		flag = 1;
	}else{

		glActiveTexture( GL_TEXTURE0);
		glBindTexture( GL_TEXTURE_2D, textureIdYUV[0]);

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
			is->videoCtx->width, is->videoCtx->height,
			GL_LUMINANCE, GL_UNSIGNED_BYTE, pDrawFrame->data[0]);

		glUniform1i( samplerLocY, 0);


		glActiveTexture( GL_TEXTURE1);
		glBindTexture( GL_TEXTURE_2D, textureIdYUV[1]);

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
			is->videoCtx->width/2, is->videoCtx->height/2,
				GL_LUMINANCE, GL_UNSIGNED_BYTE, pDrawFrame->data[1]);


		glUniform1i( samplerLocU, 1);


		glActiveTexture( GL_TEXTURE2);
		glBindTexture( GL_TEXTURE_2D, textureIdYUV[2]);



		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
			is->videoCtx->width/2, is->videoCtx->height/2,
				GL_LUMINANCE, GL_UNSIGNED_BYTE, pDrawFrame->data[2]);

		glUniform1i( samplerLocV, 2);
	}



	glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);

	//pthread_cond_signal(&s_vsync_cond);

}


jint Java_com_example_jhjh_ndktest_Tutorial04_nativeDraw
(JNIEnv* env, jobject obj)
{
    /////////////////////////////
    // DECODING PACKET

    AVPacket decodingPacket;
    av_init_packet(&decodingPacket);
    packet_queue_get(&(is->videoq), &decodingPacket, 1);

    pDrawFrame = av_frame_alloc();
    int frameFinished;
	avcodec_decode_video2(is->videoCtx, pDrawFrame, &frameFinished, &decodingPacket);


	double pts;
    if(decodingPacket.dts != AV_NOPTS_VALUE) {
      pts = av_frame_get_best_effort_timestamp(pDrawFrame);
    } else {
      pts = 0;
    }
    pts *= av_q2d(is->videoStream->time_base);



	if(frameFinished){
		draw();

		int64_t current = av_gettime();
		int64_t pts_delta = (pts - is->pts)*1000000;
		int64_t delta = current - is->timestamp;

		if(pts_delta > 0 && (pts_delta - delta) >0){
			av_usleep(pts_delta-delta);
		}


		is->timestamp = av_gettime();
		is->pts = pts;

	}

}










