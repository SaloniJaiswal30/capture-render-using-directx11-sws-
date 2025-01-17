#pragma once

#include "resource.h"
#include <time.h>
#include<DirectXMath.h>
#include <windows.h>

#include "ScreenCapture.h"

clock_t start, stop, nDiff;																	// To measure a time duration.
clock_t start_decode, stop_decode, nDiff_decode;											// To measure a time duration of decode.
clock_t time_for_capture = 0;				int number_of_frames_captured = 0;				// To measure the average time for capturing a frame.
clock_t time_for_encode = 0;				int number_of_frames_encoded = 0;				// To measure the average time for encoding a frame.
clock_t time_for_decode = 0;				int number_of_packets_decoded = 0;				// To measure the average time for decoding a packet.
clock_t time_for_scaling_rgb_to_yuv = 0;	int number_of_rgb_frames_scaled = 0;			// To measure the average time for scaling a frame from rgb to yuv.
clock_t time_for_scaling_yuv_to_rgb = 0;	int number_of_yuv_frames_scaled = 0;			// To measure the average time for scaling a frame from yuv to rgb.

int number_of_I_frames = 0;			// Count of the I frames.
int number_of_P_frames = 0;			// Count of the I frames.

long total_size_of_packets = 0;		// Total size of the packets received from encoder.
int number_of_packets = 0;			// Count of the packets received from encoder.

char *is_keyframe;					// Array containing the type char of frames.


struct SwsContext *rgb_to_yuv_SwsContext = NULL;	// The context for scaling a frame from rgb to yuv.
struct SwsContext *yuv_to_rgb_SwsContext = NULL;	// The context for scaling a frame from yuv to rgb.

AVFrame *inframe, *outframe;						// The frames used in rgb to yuv scaling.
AVFrame *iframe, *oframe;							// The frames used in yuv to rgb scaling.

AVFrame *tFrame, *gFrame;
AVFrame *frame = NULL, *sw_frame = NULL;			// The frames used for hw accelerated decode.

AVCodecContext *encodingCodecContext = NULL;		// The context of the codec used as encoder.
AVCodecContext *decodingCodecContext = NULL;		// The context of the codec used as decoder.		

FILE *log_file;										// The log file which will be created in the working directory.
FILE *captured_file;								// The captured file.
FILE *h264_file;									// H.264 video file.
FILE *decoded_file;									// The decoded image file.







void decode(AVPacket *pkt);

void encode(AVFrame *frame, AVPacket *pkt);

void createDirectory(const char * folder_name);

void driver(HWND hWnd);


