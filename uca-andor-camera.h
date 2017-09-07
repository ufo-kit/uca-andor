/* AUTHOR: Sven Werchner
 * DATE: 06.12.2013
 *
 */


#ifndef __UCA_ANDOR_CAMERA_H
#define __UCA_ANDOR_CAMERA_H

#include <glib-object.h>
#include <uca/uca-camera.h>

G_BEGIN_DECLS

#define UCA_TYPE_ANDOR_CAMERA               (uca_andor_camera_get_type())
#define UCA_ANDOR_CAMERA(obj)               (G_TYPE_CHECK_INSTANCE_CAST((obj), UCA_TYPE_ANDOR_CAMERA, UcaAndorCamera))
#define UCA_IS_ANDOR_CAMERA(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj), UCA_TYPE_ANDOR_CAMERA))
#define UCA_ANDOR_CAMERA_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST((klass), UCA_TYPE_ANDOR_CAMERA, UcaAndorCameraClass))
#define UCA_IS_ANDOR_CAMERA_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass), UCA_TYPE_ANDOR_CAMERA))
#define UCA_ANDOR_CAMERA_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS((obj), UCA_TYPE_ANDOR_CAMERA, UcaAndorCameraClass))

#define UCA_ANDOR_CAMERA_ERROR              uca_andor_camera_error_quark()

typedef enum {
    ANDOR_NOERROR = 0,
    UCA_ANDOR_CAMERA_ERROR_LIBANDOR_INIT,
    UCA_ANDOR_CAMERA_ERROR_LIBANDOR_GENERAL,
    UCA_ANDOR_CAMERA_ERROR_UNSUPPORTED,
    UCA_ANDOR_CAMERA_ERROR_INVALID_MODE,
    UCA_ANDOR_CAMERA_ERROR_MODE_NOT_AVAILABLE,
    UCA_ANDOR_CAMERA_ERROR_ENUM_OUT_OF_RANGE
} UcaAndorCameraError;

typedef enum {						/* SimplePreAmpGainControl enumerated definition */
	UCA_ANDOR_CAMERA_SPAGC_11BIT_HIGH_CAPACITY,
	UCA_ANDOR_CAMERA_SPAGC_11BIT_LOW_NOISE,
	UCA_ANDOR_CAMERA_SPAGC_16BIT
}  UcaAndorCameraSPAGC;

typedef enum {						/* ShutteringMode enumerated definition */
	UCA_ANDOR_CAMERA_SHUTTERING_MODE_ROLLING,
	UCA_ANDOR_CAMERA_SHUTTERING_MODE_GLOBAL
}  UcaAndorCameraShutteringMode;

typedef enum {						/* PixelReadoutRate enumerated definition */
/* WARNING : experimentally read enum on actual camera is different from definition in Andor's documentation :
 *	| ANDOR DOC	| ACTUAL CAMERA	| Index	|
 *	|---------------|---------------|-------|
 *	| 280 MHz	| (not implem.) | 0	|
 *	| 200 MHz	| 100 MHz	| 1 	|
 *	| 100 MHz	| 200 MHz	| 2	|
 *	| (not def.)	| 280 MHz	| 3	|
 */
	UCA_ANDOR_CAMERA_PIXEL_READOUT_RATE_100MHZ = 1,	/* offset */
	UCA_ANDOR_CAMERA_PIXEL_READOUT_RATE_200MHZ,
	UCA_ANDOR_CAMERA_PIXEL_READOUT_RATE_280MHZ
}  UcaAndorCameraPixelReadoutRate;

typedef enum {						/* FanSpeed enumerated definition */
	UCA_ANDOR_CAMERA_FAN_SPEED_OFF,
	UCA_ANDOR_CAMERA_FAN_SPEED_LOW,
	UCA_ANDOR_CAMERA_FAN_SPEED_ON
}  UcaAndorCameraFanSpeed;

typedef enum {						/* AOIBinning enumerated definition */
	UCA_ANDOR_CAMERA_AOI_BINNING_1X1,
	UCA_ANDOR_CAMERA_AOI_BINNING_2X2,	
	UCA_ANDOR_CAMERA_AOI_BINNING_3X3,
	UCA_ANDOR_CAMERA_AOI_BINNING_4X4,
	UCA_ANDOR_CAMERA_AOI_BINNING_8X8
}  UcaAndorCameraAOIBinning;

typedef enum {						/* CycleMode enumerated definition */
	UCA_ANDOR_CAMERA_CYCLE_MODE_FIXED,
	UCA_ANDOR_CAMERA_CYCLE_MODE_CONTINUOUS
}  UcaAndorCameraCycleMode;

typedef struct _UcaAndorCamera          UcaAndorCamera;
typedef struct _UcaAndorCameraClass     UcaAndorCameraClass;
typedef struct _UcaAndorCameraPrivate   UcaAndorCameraPrivate;


struct _UcaAndorCamera {
    /*< private >*/
    UcaCamera parent;

    UcaAndorCameraPrivate *priv;
};

struct _UcaAndorCameraClass {
    /*< private >*/
    UcaCameraClass parent;
};

GType uca_andor_camera_get_type (void);

G_END_DECLS

#endif
