/*
 * video.h
 *
 *      Author: gm
 *
 *
 * This file is part of Lin_guider.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef VIDEO_H_
#define VIDEO_H_

#include <stdint.h>
#include <pthread.h>
#include <linux/videodev2.h>

#include <QObject>

#include "maindef.h"


namespace video_drv
{

#define FOURCC_FORMAT		"%c%c%c%c"
#define FOURCC_ARGS(c)		(c) & 0xFF, ((c) >> 8) & 0xFF, ((c) >> 16) & 0xFF, ((c) >> 24) & 0xFF

//----- got from GUVCVIEW --------
#ifndef V4L2_CID_CAMERA_CLASS_LAST
#define V4L2_CID_CAMERA_CLASS_LAST              (V4L2_CID_CAMERA_CLASS_BASE +20)
#endif

#define V4L2_CID_BASE_EXTCTR                            0x0A046D01
#define V4L2_CID_BASE_LOGITECH                          V4L2_CID_BASE_EXTCTR
//this should realy be replaced by V4L2_CID_FOCUS_ABSOLUTE in libwebcam
#define V4L2_CID_FOCUS_LOGITECH                         V4L2_CID_BASE_LOGITECH+3
#define V4L2_CID_LED1_MODE_LOGITECH                     V4L2_CID_BASE_LOGITECH+4
#define V4L2_CID_LED1_FREQUENCY_LOGITECH                V4L2_CID_BASE_LOGITECH+5
#define V4L2_CID_DISABLE_PROCESSING_LOGITECH            V4L2_CID_BASE_LOGITECH+0x70
#define V4L2_CID_RAW_BITS_PER_PIXEL_LOGITECH            V4L2_CID_BASE_LOGITECH+0x71
#define V4L2_CID_LAST_EXTCTR                            V4L2_CID_RAW_BITS_PER_PIXEL_LOGITECH
//-------------------------

typedef enum
{
        INPUT_CONTROL_TYPE_INTEGER = 1,
        INPUT_CONTROL_TYPE_BOOLEAN,
        INPUT_CONTROL_TYPE_MENU,
        INPUT_CONTROL_TYPE_BUTTON,
}cam_control_type_t;


typedef struct
{
        unsigned int i;
        unsigned int id;
        cam_control_type_t type;
        char *name;
        int min, max, step, default_val, enabled;
        char **entries;
}cam_control_t;


#define IOCTL_RETRY 4

enum driver_type
{
	DRV_UVC = 1,
	DRV_PWC,
	DRV_NULL,
	DRV_QHY5,
	DRV_DSI2PRO,
	DRV_QHY6,
	DRV_QHY5II,
	DRV_MAX
};

enum device_type
{
	DT_WEBCAM = 1,
	DT_NULL,
	DT_QHY5,
	DT_DSI2PRO,
	DT_QHY6,
	DT_QHY5II,
	DT_MAX,
	DEVICE_CNT = DT_MAX-1
};

typedef struct
{
	const int type;
	const bool show_dev_string_ui;
	const char *desc;
}device_desc_t;

extern device_desc_t device_desc_list[DEVICE_CNT];

// external types
#define MAX_FMT		48

// internal types
typedef enum
{
  IO_METHOD_READ,
  IO_METHOD_MMAP,
} io_method;



typedef struct time_fract
{
	unsigned int numerator;
	unsigned int denominator;

	static double to_msecs( const struct time_fract &tm )
	{
		double div = tm.denominator != 0 ? tm.denominator : 1;
		return 1000.0 * (double)tm.numerator / div;
	}
	static double to_fps( const struct time_fract &tm )
	{
		double div = tm.numerator != 0 ? tm.numerator : 1;
		return (double)tm.denominator / div;
	}
	static struct time_fract mk_fps( unsigned int num, unsigned int den )
	{
		struct time_fract ret;
		ret.numerator = num;
		ret.denominator = den;
		return ret;
	}
	bool operator==( const struct time_fract &v ) const
	{
		return (v.numerator == this->numerator && v.denominator == this->denominator);
	}
	bool operator!=( const struct time_fract &v ) const
	{
		return (v.numerator != this->numerator || v.denominator != this->denominator);
	}
}time_fract_t;


typedef struct
{
	int type;
	io_method	 io_mtd;
	unsigned int pixel_format;
	unsigned int width;
	unsigned int height;
	time_fract_t fps;
	int		     autogain;
	int	         gain;
	int			 exposure;
	bool         use_calibration;
}captureparams_t;


typedef struct
{
	int type;
	unsigned int width;
	unsigned int height;
}capture_next_params_t;


typedef struct
{
	point_t 		size;
	time_fract_t 	fps_table[MAX_FMT];
}frame_t;

typedef struct
{
	frame_t			frame_table[MAX_FMT];
	unsigned int 	format;
}format_param_t;


typedef struct
{
	format_param_t *format_desc;

	// current indeces
	int format_idx;
	int frame_idx;
	int fps_idx;
}current_format_state_t;


// post param types
#define PP_FPS				0x1
#define PP_FRAME_SIZE		0x2
#define PP_AUTOGAIN			0x4
#define PP_GAIN				0x8
#define PP_EXPO				0x10


typedef struct param_val
{
	void set( int v1 = 0, int v2 = 0 ) { values[0] = v1; values[1] = v2; }
	int values[2];
}param_val_t;

typedef struct
{
	int params;
	int values[8];
}post_param_t;



typedef enum
{
	CI_FPS = 1,
	CI_AUTOGAIN,
	CI_GAIN,
	CI_EXPO
}control_id_t;



class cvideo_base : public QObject
{
	Q_OBJECT

public:
	cvideo_base();
	virtual ~cvideo_base();

	// public functions
	bool start( const char *devname );	// start device (must be called after creation)
	void stop( void );											// stops device (must be called before destruction)
	void pause( bool set );
	void continue_capture( void );
	void process_frame( void *video_dst, int video_dst_size, void *math_dst, int math_dst_size, const void *src, bool is_grey, int bytesused );
	captureparams_t get_capture_params( void ) const;			// rerurns actual params (must be called after start())
	void set_capture_params( const captureparams_t &v );
	capture_next_params_t get_next_params( void ) const;
	void set_next_params( const capture_next_params_t &v );
	current_format_state_t get_current_format_params( void );
	virtual time_fract_t set_fps( const time_fract_t &new_fps ) = 0;
	bool is_initialized( void ) const;
	int  pack_params( control_id_t ctrl, const param_val_t &val, post_param_t *prm );
	int  post_params( const post_param_t &prm );
	cam_control_t *get_cam_control( int ctrl ) const;
	void start_calibration( int frame_cnt );
	void cancel_calibration( void );
	void set_use_calibration( bool use );
	const char* get_name( void ) const;

	static int detect_best_device( int devtype, const char *devname );

signals:
	void renderImage(const void *ptr, int len);
	void calibrationFinished();

private:
	virtual int init_device( void ) = 0;		// get&check capabilities, apply format
	virtual int uninit_device( void ) = 0;		// deinit device
	virtual int start_capturing( void ) = 0;	// turn on stream
	virtual int stop_capturing( void ) = 0;		// stop stream
	virtual int read_frame( void ) = 0;			// read frame
	virtual int set_format( void ) = 0;

	int enum_frame_formats( void );
	int enum_frame_sizes( unsigned int pixfmt, int fmt_idx );
	int enum_frame_intervals( unsigned int pixfmt, unsigned int width, unsigned int height, int fmt_idx, int frm_idx );
	virtual int enum_controls( void );

	int check_posted_params( void );

	void free_controls( void );
protected:
	union data_ptr
	{
		void     *ptr;
		uint8_t  *ptr8;
		uint16_t *ptr16;
		double   *ptrDBL;
		data_ptr( const void *s ) : ptr( (void *)s ) {}
		data_ptr() : ptr( NULL ) {}
	};

	// buffer description
	struct buffer
	{
		buffer() : length(0) {}
		data_ptr start;	// buffer pointer
		size_t length;	// buffer length
	};

	cam_control_t *add_control( int fd, struct v4l2_queryctrl *queryctrl, cam_control_t *control, int *nctrl );	// idea have been got from guvcview
	int xioctl( int fd, int request, void *arg );

	virtual int open_device( void );		// open device
	virtual int close_device( void );		// close device
	virtual int get_vcaps( void );
	int get_format_idx( void ) const;
	int get_frame_idx( void ) const;
	int get_fps_idx( void ) const;

	virtual int set_control( int control_id, const param_val_t &val );
	virtual int get_control( int control_id, param_val_t *val );

	int set_autogain( int val );
	int get_autogain( void );

	int set_gain( int val );
	int get_gain( void );

	int set_exposure( int val );
	int get_exposure( void );

	void init_lut_to8bit( int top = 0 );
	int bpp( void ) const;
	bool is_color( void ) const;

	//struct video_capability vcap;
	struct v4l2_capability vcap;

	bool initialized;  	// if device opened = true
	char dev_name[64];	// device name
	int device_type;
	int next_device_type;

	captureparams_t capture_params;
	capture_next_params_t next_params;	// params that would be applied after restart
	format_param_t 	device_formats[MAX_FMT];	// Assume no device supports more than 16 formats

	bool		is_streaming;
	unsigned	frame_delay;
	bool 		force_fps;		// TRUE if unable to determine FPS from camera

	cam_control_t *controls;
	int 		   num_controls;

	buffer *buffers;
	unsigned int n_buffers;
	unsigned char *tmp_buffer;

	buffer decoded_buffer;
	buffer calibration_buffer;
	int calibration_frame_cnt;
	int calibration_frame;
	bool is_calibrating;
	bool has_calibration;

	buffer lut_to8bit;
	static const int lut_to8bit_len = (1<<16);

	// thread stuff
	pthread_t dev_thread;		// thread desc.
	pthread_cond_t cv;			// sunc cond. var.
	pthread_mutex_t	cv_mutex;	// cond. var. mutex
	int	start_thread_flag;		// start flag
	int	quit_thread_flag;		// quit flag
	int	data_thread_flag;		// data flag
	post_param_t posted_param;
	static void *video_thread( void *param );

	int fd;				// file descriptor

	bool is_v4l_1;
};

}

#endif /*VIDEO_H_*/
