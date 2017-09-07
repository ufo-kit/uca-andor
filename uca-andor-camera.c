/*
 * Copyright (C) 2014-2017 Karlsruhe Institute of Technology
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA 02110, USA
 */


/** WARNING:
 *	This plugin has been tested under the following configuration:
 *		Ubuntu 16.04 LTS 
 *		Kernel version = 4.4.0
 *		Camera 'Andor Neo' ; model 'DC-152Q-FOO-FI' ; Ser.No = SCC-1837
 *		Internal firmware version = V3
 * 	There is no warranty that this plugin will be compatible under any other configuration, especially when using newer firmware version.
 */

#include <uca/uca-camera.h>	
#include <gio/gio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include "atcore.h"
#include "atutility.h"			
#include "uca-andor-camera.h" 	
#include "uca-andor-enums.h"		

/** 
 * atutility:
 * Additional utilities functions provided by Andor.
 * WARNING: Some functions in this header use C++ syntaxe: it is needed to comment them manually in order to the plugin to compile.s
 */

#define UCA_ANDOR_CAMERA_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UCA_TYPE_ANDOR_CAMERA, UcaAndorCameraPrivate))


static void uca_andor_initable_iface_init(GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (UcaAndorCamera, uca_andor_camera, UCA_TYPE_CAMERA,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, uca_andor_initable_iface_init))

#define TIMEBASE_INVALID        0xDEAD
#define NUM_BUFFERS             10
#define WAIT_BUFFER_TIMEOUT	10000 		/* Time allowed for the camera to return buffer before raising error; original = 10000 (10s) */

/* Available values for check_access function's parameters */
#define CHECK_ACCESS_WRITE	0		
#define CHECK_ACCESS_READ	1		
#define CHECK_ACCESS_WARN	TRUE		
#define CHECK_ACCESS_SILENT	FALSE		


/**
 * UcaAndorCameraError
 * @ANDOR_NOERROR:				
 * @UCA_ANDOR_CAMERA_ERROR_LIBANDOR_INIT:	
 * @UCA_ANDOR_CAMERA_ERROR_LIBANDOR_GENERAL:	
 * @UCA_ANDOR_CAMERA_ERROR_UNSUPPORTED:
 * @UCA_ANDOR_CAMERA_ERROR_INVALID_MODE:
 * @UCA_ANDOR_CAMERA_ERROR_MODE_NOT_AVAILABLE:
 * @UCA_ANDOR_CAMERA_ERROR_ENUM_OUT_OF_RANGE:
 *
 * This enumerated is currently not used : when error detected and GError is available, just set by default LIBANDOR_GENERAL
 */


struct _UcaAndorCameraPrivate {
    guint camera_number;
    AT_H handle;
    GError *construct_error;

    gchar*  name;
    gchar*  model;
    guint64 aoi_left;
    guint64 aoi_top;
    guint64 aoi_width;
    guint64 aoi_height;
    guint64 aoi_stride;
    guint64 sensor_width;
    guint64 sensor_height;
    gdouble pixel_width;
    gdouble pixel_height;
    gint trigger_mode;
    gdouble frame_rate;
    gdouble exp_time;
    gdouble sensor_temperature;
    gdouble target_sensor_temperature;
    gint fan_speed;
    gchar* cycle_mode;
    gboolean is_sim_cam;
    gboolean is_cam_acquiring;

    AT_WC*  pixel_encoding_wchar; 	/* pixel encoding under AT_WC* format needed to pass it into AT_ConvertBuffer */
    AT_U8*  image_buffer;
    AT_U8*  aligned_buffer;
    AT_64   image_size;               	/* full image (frame + padding + metadata if enabled) memory size in bytes */

};

static gint andor_overrideables [] = {
    PROP_NAME,
    PROP_EXPOSURE_TIME,
    PROP_ROI_X,
    PROP_ROI_Y,
    PROP_ROI_WIDTH,
    PROP_ROI_HEIGHT,
    PROP_SENSOR_WIDTH,
    PROP_SENSOR_HEIGHT,
    PROP_SENSOR_PIXEL_WIDTH,
    PROP_SENSOR_PIXEL_HEIGHT,
    PROP_IS_RECORDING,
    PROP_SENSOR_BITDEPTH,	
    PROP_HAS_CAMRAM_RECORDING,
    PROP_HAS_STREAMING,
    0,
};

enum {				
    PROP_ROI_STRIDE = N_BASE_PROPERTIES,
    PROP_SENSOR_TEMPERATURE,
    PROP_TARGET_SENSOR_TEMPERATURE,
    PROP_FAN_SPEED,
    PROP_CYCLE_MODE,
    PROP_FRAMERATE,
    N_PROPERTIES
};

/////////////////////////////////////////////////////////////////////////////

/* 	##############################
 * 	# INTERNAL UTILITY FUNCTIONS #
 * 	##############################
 */

static gchar*
identify_andor_error (int error)
/**
 * Return a string containing the error corresponding to the error number returned from andor camera
 * The following 'switch' matches the defined AT errors in the SDK's file 'atcore.h' and in the documentation
 */
{
	gchar* string;
	switch (error) {
		/* atcore.h errors */
		case 0:
			string = "No error ... (identify_andor_error function has been called for a bad reason, please fix)";
			break;
		case 1:
			string = "Camera Handle uninitialized";
			break;
		case 2:
			string = "Feature is not implemented for this camera";
			break;
		case 3:
			string = "Feature is read only";
			break;
		case 4:
			string = "Feature is currently not readable";
			break;
		case 5:
			string = "Feature is currently not writable / Command is not currently executable";
			break;
		case 6:
			string = "Value is either out of range or unavailable";
			break;
		case 7:
			string = "Index is currently not available";
			break;
		case 8:
			string = "Index is not implemented on this camera";
			break;
		case 9:
			string = "String value exceed maximum allowed length";
			break;
		case 10:
			string = "Connection or Disconnection error";
			break;
		case 11:
			string = "No Internal Event or Internal Error";
			break;
		case 12:
			string = "Invalid handle";
			break;
		case 13:
			string = "Waiting for buffer timed out";
			break;
		case 14:
			string = "Input buffer queue reached maximum capacity";
			break;
		case 15:
			string = "Queued buffer / returned frame size conflict";
			break;
		case 16:
			string = "A queued buffer was not aligned on an 8-byte boundary";
			break;
		case 17:
			string = "An error has occurred while communicating with hardware";
			break;
		case 18:
			string = "Index / String is not currently available";
			break;
		case 19:
			string = "Index / String is not implemented on this camera";
			break;
		case 20:
			string = "Passed feature = NULL";
			break;
		case 21:
			string = "Passed handle = NULL";
			break;
		case 22:
			string = "Feature not implemented";
			break;
		case 23:
			string = "Readable not set";
			break;
		case 24:
			string = "Readonly not set"; 	
			break;
		case 25:
			string = "Writable not set";
			break;
		case 26:
			string = "Min value = NULL";
			break;
		case 27:
			string = "Max value = NULL";
			break;
		case 28:
			string = "Function returned NULL value";
			break;
		case 29:
			string = "Function returned NULL string";
			break;
		case 30:
			string = "Feature index count = NULL";
			break;
		case 31:
			string = "Available not set";
			break;
		case 32:
			string = "Passed string lenght = NULL";
			break;
		case 33:
			string = "EvCallBack parameter = NULL";
			break;
		case 34:
			string = "Pointer to queue = NULL";
			break;
		case 35:
			string = "Wait pointer = NULL";
			break;
		case 36:
			string = "Pointer size = NULL";
			break;
		case 37:
			string = "No memory allocated for current action";
			break;
		case 38:
			string = "Unable to connect, device already in use";
			break;
		case 39:
			string = "Device not found";
			break;
		case 100:
			string = "The software was not able to retrieve data from the card or camera fast enough to avoid the internal hardware buffer bursting";
			break;
		/* atutility.h errors */
		case 1002:
			string = "Invalid output pixel encoding";
			break;
		case 1003:
			string = "Invalid input pixel encoding";
			break;
		case 1004:
			string = "Input buffer does not include metadata";
			break;
		case 1005:
			string = "Corrupted metadata";
			break;
		case 1006:
			string = "Metadata not found";
			break;
		default :
			string = "Unknown error...";
			break;
	} 
	return string;
}

static gboolean		
check_access (UcaAndorCameraPrivate *priv, const AT_WC* feature, int access, gboolean warn)
/**
 * Check access of the feature passed in parameters :
 *	- check if implemented
 *	- if access = CHECK_ACCESS_READ		check if readable
 *	- if access = CHECK_ACCESS_WRITE 	check if read_only then if writable
 * Return TRUE if access is OK, return FALSE if not
 * 	- if warn = CHECK_ACCESS_WARN		will display warning if access is not allowed/available or if an error occur
 *	- if warn = CHECK_ACCESS_SILENT		will display warning only if an error occur
 * Does not display error when read access fail if camera = SIMCAM (to not overload output... but errors are still displayed if its for writting)
 */
{
	int errnum;
	AT_BOOL testbool;
	errnum = AT_IsImplemented (priv->handle, feature, &testbool);
	if (!errnum && testbool){
		if (access == CHECK_ACCESS_READ) {
			errnum = AT_IsReadable (priv->handle, feature, &testbool);
			if (!errnum && !testbool) {
				if (warn) g_warning ("READ ACCESS ERROR: feature '%S' is currently not readable", feature);
				return FALSE;
			}
			else if (errnum) {
				g_warning ("Check access failed for '%S': AT_IsReadable returned error: %s (%d)",feature, identify_andor_error(errnum), errnum);
				return FALSE;
			}
		}
		else if (access == CHECK_ACCESS_WRITE) {
			errnum = AT_IsReadOnly (priv->handle, feature, &testbool);
			if (!errnum && testbool) {
				if (warn) g_warning ("WRITE ACCESS ERROR: feature '%S' is read only", feature);
				return FALSE;
			}
			else if (errnum) {
				g_warning ("Check access failed for '%S': AT_IsReadOnly returned error: %s (%d)",feature,identify_andor_error(errnum), errnum);
				return FALSE;
			}
			errnum = AT_IsWritable (priv->handle, feature, &testbool);
			if (!errnum && !testbool) {
				if (warn) g_warning ("WRITE ACCESS ERROR: feature '%S' is currently not writable", feature);
				return FALSE;
			}
			else if (errnum) {
				g_warning ("Check access failed for '%S': AT_IsWritable returned error: %s (%d)",feature,identify_andor_error(errnum), errnum);
				return FALSE;
			}			
		}
		else {
			g_warning("Could not check access '%d' for feature '%S', access not recognised", access, feature);
		}
		return TRUE;
	}
	else if (!errnum && !testbool){
		if (warn && !(priv->is_sim_cam && access==CHECK_ACCESS_READ) ) /* Disable 'not implemented' display if camera is SIMCAM and checking READ access */
			g_warning ("ACCESS ERROR: feature '%S' is not implemeted on camera '%s'", feature, priv->name);
		return FALSE;
	}
	else {
		g_warning ("Check access failed for '%S': AT_IsImplemented returned error: %s (%d)",feature,identify_andor_error(errnum), errnum);
		return FALSE;
	}
return FALSE; 		/* by default .. should not be encountered */
}

static gboolean
write_integer (UcaAndorCameraPrivate *priv, const AT_WC* property, guint64 value)
{
    int error;
    AT_64 max=0, min=0;

    if (!check_access (priv, property, CHECK_ACCESS_WRITE, CHECK_ACCESS_WARN)) return FALSE;		

    error = AT_GetIntMax (priv->handle, property, &max);	
    if (error) {							
	g_warning ("Could not read maximum allowable '%S'value: %s (%d)", property, identify_andor_error(error), error);
	return FALSE;
    }

    error = AT_GetIntMin (priv->handle, property, &min);	
    if (error) {							
	g_warning ("Could not read minimum allowable '%S' value: %s (%d)", property, identify_andor_error(error), error);
	return FALSE;
    }

    if ((value < min) || (value > max)) {			
	g_warning ("Value %d is out of range for feature %S: current range is [%d ; %d]", (int) value, property, (int) min, (int) max);
	return FALSE;
    }

    error = AT_SetInt (priv->handle, property, value);
    if (error != AT_SUCCESS) {
        g_warning ("Could not write integer '%S': %s (%d)", property, identify_andor_error(error), error);
        return FALSE;
    }
    return TRUE;
}

static gboolean
read_integer (UcaAndorCameraPrivate *priv, const AT_WC* property, guint64 *value)
{
    guint64 temp;
    if (!check_access (priv, property, CHECK_ACCESS_READ, CHECK_ACCESS_WARN)) return FALSE;		
    int error = AT_GetInt (priv->handle, property, (AT_64 *) &temp);
    if (error != AT_SUCCESS) {
        g_warning ("Could not read integer '%S': %s (%d)", property, identify_andor_error(error), error);
        return FALSE;
    }
    *value = temp;
    return TRUE;
}

static gboolean
write_double (UcaAndorCameraPrivate *priv, const AT_WC* property, double value)
{
    int error;
    double max=0, min=0;

    if (!check_access (priv, property, CHECK_ACCESS_WRITE, CHECK_ACCESS_WARN)) return FALSE;		

    error = AT_GetFloatMax (priv->handle, property, &max);
    if (error) {						
	g_warning ("Could not read maximum allowable '%S'value: %s (%d)", property, identify_andor_error(error), error);
	return FALSE;
    }

    error = AT_GetFloatMin (priv->handle, property, &min);	
    if (error) {							
	g_warning ("Could not read minimum allowable '%S' value: %s (%d)", property, identify_andor_error(error), error);
	return FALSE;
    }

    if ((value < min) || (value > max)) {			
	g_warning ("Value %f is out of range for feature %S: current range is [%f ; %f]", (float) value, property, (float) min, (float) max);
	return FALSE;
    }

    error = AT_SetFloat (priv->handle, property, value);
    if (error != AT_SUCCESS) {
        g_warning ("Could not write double '%S': %s (%d)", property, identify_andor_error(error), error);
        return FALSE;
    }
    return TRUE;
}

static gboolean
read_double (UcaAndorCameraPrivate *priv, const AT_WC* property, double *value)
{
    double temp;
    if (!check_access (priv, property, CHECK_ACCESS_READ, CHECK_ACCESS_WARN)) return FALSE;		
    int error = AT_GetFloat (priv->handle, property, &temp);
    if (error != AT_SUCCESS) {
        g_warning ("Could not read double '%S': %s (%d)", property, identify_andor_error(error), error);
        return FALSE;
    }
    *value = temp;
    return TRUE;
}

static gboolean		
read_double_max (UcaAndorCameraPrivate *priv, const AT_WC* property, double *value) 
{
	double temp;
    if (!check_access (priv, property, CHECK_ACCESS_READ, CHECK_ACCESS_WARN)) return FALSE;	
	int error = AT_GetFloatMax (priv->handle, property, &temp);
    	if (error != AT_SUCCESS) {
        	g_warning ("Could not read double max of '%S': %s (%d)", property, identify_andor_error(error), error);
        	return FALSE;
    	}
	*value = temp;
	return TRUE;
}

static gboolean		
read_double_min (UcaAndorCameraPrivate *priv, const AT_WC* property, double *value) 
{
	double temp;
    if (!check_access (priv, property, CHECK_ACCESS_READ, CHECK_ACCESS_WARN)) return FALSE;	
	int error = AT_GetFloatMin (priv->handle, property, &temp);
    	if (error != AT_SUCCESS) {
        	g_warning ("Could not read double min of '%S': %s (%d)", property, identify_andor_error(error), error);
        	return FALSE;
    	}
	*value = temp;
	return TRUE;
}

static gboolean
write_enum_index (UcaAndorCameraPrivate *priv, const AT_WC* property, int value)
{
    int count;
    if (!check_access (priv, property, CHECK_ACCESS_WRITE, CHECK_ACCESS_WARN)) return FALSE;		

    int error = AT_GetEnumCount (priv->handle, property, &count);
    if (error != AT_SUCCESS) {
        g_warning ("Cannot read enum count '%S': %s (%d)", property, identify_andor_error(error), error);
        return FALSE;
    }

    if (value >= count || value < 0) {
        g_warning ("Enumeration value (%d) out of range [0, %i] for feature '%S'", value, count - 1, property);
        return FALSE;
    }

    error = AT_SetEnumIndex (priv->handle, property, value);
    if (error != AT_SUCCESS) {
        g_warning ("Could not set enum '%S': %s (%d)", property, identify_andor_error(error), error);
        return FALSE;
    }
    return TRUE;
}

static gboolean
read_enum_index (UcaAndorCameraPrivate *priv, const AT_WC* property, int *value)
{
    int temp;
    if (!check_access (priv, property, CHECK_ACCESS_READ, CHECK_ACCESS_WARN)) return FALSE;		

    int error = AT_GetEnumIndex (priv->handle, property, &temp);
    if (error != AT_SUCCESS) {
        g_warning ("Could not read index '%S': %s (%d)", property, identify_andor_error(error), error);
        return FALSE;
    }
    *value = temp;
    return TRUE;
}

static gboolean
write_string (UcaAndorCameraPrivate *priv, const AT_WC *property, const gchar *value)
{
    AT_WC* wide_value;
    size_t len;
    gboolean result;
 
    if (!check_access (priv, property, CHECK_ACCESS_WRITE, CHECK_ACCESS_WARN)) return FALSE;		

    result = TRUE;
    len = strlen (value);
    wide_value = g_malloc0 ((len + 1) * sizeof (AT_WC));
    mbstowcs (wide_value, value, len);

    int error = AT_SetEnumString (priv->handle, property, wide_value);
    if (error != AT_SUCCESS) {
        g_warning ("Could not write string '%s' to '%S': %s (%d)", value, property, identify_andor_error(error), error);
        result = FALSE;
    }

    g_free (wide_value);
    return result;
}

static gboolean
read_string (UcaAndorCameraPrivate *priv, const AT_WC *property, gchar **value)
{
    AT_WC* wide_value;
    int index, error;
    gboolean result = TRUE;

    if (!check_access (priv, property, CHECK_ACCESS_READ, CHECK_ACCESS_WARN)) return FALSE;		

    error = AT_GetEnumIndex (priv->handle, property, &index);
    if (error != AT_SUCCESS) {
	g_warning ("Could not read index for '%S': %s (%d)", property, identify_andor_error(error), error);
        return FALSE;
    }

    wide_value = g_malloc0 (1023 * sizeof (AT_WC));

    error = AT_GetEnumStringByIndex (priv->handle, property, index, wide_value, 1023);
    if (error != AT_SUCCESS) {
	g_warning ("Could not read string '%S': %s (%d)", property, identify_andor_error(error), error);
	g_free (wide_value); 
        result = FALSE;
    }

    *value = g_malloc0 ((wcslen (wide_value) + 1) * sizeof (gchar));
    wcstombs (*value, wide_value, wcslen (wide_value));

    g_free (wide_value);
    return result;
}

static gboolean												
read_boolean (UcaAndorCameraPrivate *priv, const AT_WC *property, gboolean *value)	
{
	gboolean temp;
    if (!check_access (priv, property, CHECK_ACCESS_READ, CHECK_ACCESS_WARN)) return FALSE;			

	int error = AT_GetBool (priv->handle, property, (AT_BOOL *) &temp);
	if (error != AT_SUCCESS) {
		g_warning ("Could not read boolean '%S': %s (%d)", property, identify_andor_error(error), error);
		return FALSE;
	}
	*value = temp;
	return TRUE;
}

static gboolean										
write_boolean (UcaAndorCameraPrivate *priv, const AT_WC* property, gboolean value)	
{
    if (!check_access (priv, property, CHECK_ACCESS_WRITE, CHECK_ACCESS_WARN)) return FALSE;		

    int error = AT_SetBool (priv->handle, property, value);
    if (error != AT_SUCCESS) {
        g_warning ("Could not write boolean '%S': %s (%d)", property, identify_andor_error(error), error);
        return FALSE;
    }

    return TRUE;
}


static gint64
extract_uint_from_string (gchar* string)			
/**
 * Function used for extracting bitdeph value (uint) from andor's returned string
 *	-> assume that the string contain an unique number of 1 or 2 numeral(s) (nothing else!)
 */
{
	gint64 num;
	gchar extract[2];
	long unsigned int i=0;
	while (i < (strlen(string))) {
		if (string[i]=='0' || string[i]=='1' || string[i]=='2' || string[i]=='3' || string[i]=='4' || string[i]=='5' || string[i]=='6' ||  
		    string[i]=='7' || string[i]=='8' || string[i]=='9') { 		/* buldozzer method to check if character is a numeral... */

			if (string[i+1]=='0' || string[i+1]=='1' || string[i+1]=='2' || string[i+1]=='3' || string[i+1]=='4' || string[i+1]=='5' || string[i+1]=='6' ||  
		    	    string[i+1]=='7' || string[i+1]=='8' || string[i+1]=='9') {
				extract[0]=string[i];
				extract[1]=string[i+1];
			}
			else {
				extract[1]='\0';
				extract[0]=string[i];
			}

			num = atoi (extract);
			return num;
		}
		i++;
	}
	g_warning("Could not extract BitDepth uint from returned string '%s', returned value: 0 by default",string);
	return 0;
}

static void
calculate_frame_number (UcaAndorCameraPrivate *priv, AT_64 timestamp)
/**
 * Calculate and return the frame number since beginning of acquisition according to user's parameters :
 *	- if trigger source = AUTO: measure delta time between each frame and according to framerate, calculate the new frame number
 *				    can be used to ensure that no frame has been missed during recording
 * 	WARNING: this is an approximation, if delta time does not perfectly match the framerate, the number set is a truncation of what has been calculated
 *
 *	- if trigger source = SOFTWARE or EXTERNAL: frame_number is incremented each time grab function is used... but there is no warranty that no frame has been missed
 *	 					    because the plugin does not have access to the framerate used for the experiment
 * NOTE: this function does not check if metadata is enabled, it should not be called if this is not the case
 */
{
	switch (priv->trigger_mode) {
		case UCA_CAMERA_TRIGGER_SOURCE_AUTO:
			if (priv->last_frame_number == 0) {
				priv->last_frame_number = 1;	
				priv->last_frame_clock = timestamp;
				priv->frame_number = 1;
			}
			else {
				double temp_float = ((double)(timestamp - priv->last_frame_clock)/(double)priv->timestamp_clock_frequency) 
						     * priv->frame_rate / priv->accumulate_count;
				priv->frame_number = priv->last_frame_number + (gint) temp_float;
				priv->last_frame_number = priv->frame_number;
				priv->last_frame_clock = timestamp;
			}
			return;
		default:
			priv->frame_number++;
			return;
	}

}

static void
add_time_to_frame (AT_64 timestamp, AT_U8 *data, int frame_number)
/**
 * Overwrite the first 28 Bytes of the picture (14 pixels at 2 Bytes/pixel) with frame number and timestamp raw value :
 *	- pixels 0 to 3 (4 pixels): frame number coded in BCD-packed: each pixel contains 2 digit (going from highest power to lowest) on the last 8 bits eg :
 *	 _______________	 _______________
 *	|  (0)	|  (0)	|	| digi0	| digi1	|		
 *	     1 Byte		     1 Byte		[...]		(if number = 1042, digi[] = [1,0,4,2])
 * 	|----------------1 pixel----------------|
 *
 *	- pixels 4 to 13 (10 pixels): timestamp in binary (64 bits) converted into BCD-packed (20 digits) following the same process.
 * 	- WARNING: this function assumes that the frame_number has maximum 8 digits
 * NOTE: this function does not check if metadata is enabled, it should not be called if this is not the case
 */
{
	unsigned short temp1, temp2;	
	AT_64 pow=1e7, offset=0;	

/* Write frame_number on pixels no:0-3 (bytes no:0-7) */

	for (int i=0; i<4; i++) {				/* Naviguate through 4 first pixels */
		temp1 = ((frame_number-offset) - ((frame_number-offset) % pow)) / pow;			/* retrieve 1st digit */
		pow /= 10;
		temp2 = ((frame_number-offset) - ((frame_number-offset) % pow)) / pow - (temp1*10);	/* retrieve 2nd digit */	
		offset += ((temp1*10) + temp2) * pow;							/* suppress digits that have been grabbed from number */
		pow /= 10;
		*data = (unsigned short) (temp1*16 + temp2);	/* write digits into pixel (pixel size = unsigned short size = 2 bytes) */
		data += 2;					/* Move through the memory */
	}

/* Write timestamp on pixels no:4-13 (bytes no:8-27) */

	pow = 1e17;
	offset = 0;
	
	/* We "cheat" for the first itteration (first pixel) because we cannot set pow = 2e19 (overflow of AT_64 format (unsigned long)) (FIXME: can we?)*/
	temp1 = ((timestamp-offset) - ((timestamp-offset) % (int)1e19)) / 1e19;	
	temp2 = ((timestamp-offset) - ((timestamp-offset) % (int)1e18)) / 1e18 - (temp1*10);	
	offset += ((temp1*10) + temp2) * pow;						
	*data = (unsigned short) (temp1*16 + temp2);
	data += 2;

	for (int i=0; i<9; i++) {						
		temp1 = ((timestamp-offset) - ((timestamp-offset) % pow)) / pow;	
		pow /= 10;
		temp2 = ((timestamp-offset) - ((timestamp-offset) % pow)) / pow - (temp1*10);	
		offset += ((temp1*10) + temp2) * pow;						
		pow /= 10;
		*data = (unsigned short) (temp1*16 + temp2);
		data += 2;				
	}

}

static int
convert_and_concatenate_buffer (UcaAndorCameraPrivate *priv, AT_U8 *input_buffer, gpointer data)
/**
 * In the specific case where METADATA is used, convert buffer into correct pixel encoding + remove padding + remove METADATA from data + overwrite 4 first pixels with
 * timestamp clock value retrieved from metadata.
 * TODO: for now, it assumes that METADATA and Timestamp are enabled while FrameInfo is disabled, we should check that in the future to be more 
 * reliable and if we want to improve (currently not possible because features described in documentation are not implemented on actual camera)
 * NOTE: this function does not check if metadata is enabled, it should not be called if this is not the case
 */
{
	int error_number, ticks_offset, ticks_cid, framedata_size, framedata_cid, framedata_offset;
	AT_U8 *end_metadata, *temp_buffer;
	AT_64 timestamp;

/* Extracting timestamp from metadata */

	end_metadata = input_buffer + priv->image_size; 				/* Metadata has to be read starting from end of data memory space */
	ticks_cid = *(int *)(end_metadata - METADATA_LENGTH_SIZE - METADATA_CID_SIZE);	/* Get first block's CID (which should be Ticks) */
	if (ticks_cid != METADATA_CID_TICKS) {						/* We are not in Tick block -> unsupported */
		g_warning ("Metadata format error: espected reading 'Tick' block (of CID = 1) but got CID = %d instead", ticks_cid);
		return 1005;
	}
	ticks_offset = METADATA_LENGTH_SIZE + METADATA_CID_SIZE	+ METADATA_TIMESTAMP_SIZE;	/* offset from end of metadata to begin of 'Ticks' block's memory space */
	timestamp = *(AT_64 *)(end_metadata - ticks_offset);					/* Get the value of timestamp */

/* Converting buffer to frame stripped from padding and metadata */

	framedata_size = *(int *)(end_metadata - ticks_offset - METADATA_LENGTH_SIZE); /* Get the second block's size in bytes (which should be FrameData) */
	framedata_cid = *(int *)(end_metadata - ticks_offset - METADATA_LENGTH_SIZE - METADATA_CID_SIZE);	/* Get second block's CID (which should be 'Frame Data') */
	if (framedata_cid != METADATA_CID_FRAMEDATA) {		
		g_warning ("Metadata format error: espected reading 'FrameData' block (of CID = 0) but got CID = %d instead", framedata_cid);
		return 1005;
	}
	/* Remember NOT to count CID_SIZE because it is already included in framedata_size (the length block contain data length + cid length) */
	framedata_offset = ticks_offset + METADATA_LENGTH_SIZE + framedata_size; 
	temp_buffer = (AT_U8 *)(end_metadata - framedata_offset);
	error_number = AT_ConvertBuffer (temp_buffer, (AT_U8 *) data, priv->aoi_width, priv->aoi_height, priv->aoi_stride, priv->pixel_encoding_wchar, L"Mono16");
	if (error_number)
		return error_number;

/* Concatenating timestamp onto frame */

	calculate_frame_number(priv, timestamp);
	add_time_to_frame(timestamp, data, priv->frame_number);

	return 0;
}

/////////////////////////////////////////////////////////////////////////////

/*	##################
 *	# ERROR HANDLING #
 *	##################
 */

static GParamSpec *andor_properties [N_PROPERTIES] = { NULL, };

GQuark
uca_andor_camera_error_quark (void)
{
    return g_quark_from_static_string ("uca-andor-camera-error-quark");
}	

gboolean
check_error (int error_number, const char* message, GError **error)
{
    if (error_number != AT_SUCCESS) {
        g_set_error (error, UCA_ANDOR_CAMERA_ERROR, UCA_ANDOR_CAMERA_ERROR_LIBANDOR_GENERAL,
		     "Andor error '%s': %s (%i)", message, identify_andor_error(error_number), error_number);
        return FALSE;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

/*	#############
 *	# CALLBACKS #
 *	#############
 */

int 
AT_EXP_CONV watch_for_PixelEncoding (AT_H Handle, const AT_WC* Feature, void* Context) 
{
	UcaAndorCameraPrivate *priv = Context;
	int index, error_number;

	if (!read_enum_index (priv, L"PixelEncoding", &index))
		return 0;
	priv->pixel_encoding_wchar = g_malloc0 (1023 * sizeof (AT_WC));
	error_number = AT_GetEnumStringByIndex (priv->handle, L"PixelEncoding", index, priv->pixel_encoding_wchar, 1023);
	return 0;
}


/////////////////////////////////////////////////////////////////////////////

/*	######################################
 *	# USER (LIBUCA) ACCESSIBLE FUNCTIONS #
 *	######################################
 */

static void
uca_andor_camera_start_recording (UcaCamera *camera, GError **error)
{
    UcaAndorCameraPrivate *priv;
    g_return_if_fail (UCA_IS_ANDOR_CAMERA(camera));
    priv = UCA_ANDOR_CAMERA_GET_PRIVATE (camera);

    AT_GetInt (priv->handle, L"ImageSizeBytes", &priv->image_size);

    if (priv->image_buffer != NULL)
        g_free (priv->image_buffer);

    priv->image_buffer = g_malloc0 ((NUM_BUFFERS * priv->image_size + 8) * sizeof (gchar));
    priv->aligned_buffer = (AT_U8*) (((unsigned long) priv->image_buffer + 7) & ~0x7);

    AT_Flush (priv->handle);

    for (int i = 0; i < NUM_BUFFERS; i++)
        AT_QueueBuffer (priv->handle, priv->aligned_buffer + i * priv->image_size, priv->image_size);

    AT_SetEnumString(priv->handle, L"CycleMode", L"Continuous");

    if (!check_error (AT_Command (priv->handle, L"AcquisitionStart"),
                      "Could not start acquisition", error))
	return;

    check_error (!read_boolean (priv, L"CameraAcquiring", &priv->is_cam_acquiring),
                 "Could not read CameraAcquiring", error);	
}

static void
uca_andor_camera_stop_recording (UcaCamera *camera, GError **error)
{
    UcaAndorCameraPrivate *priv;

    g_return_if_fail (UCA_IS_ANDOR_CAMERA(camera));
    priv = UCA_ANDOR_CAMERA_GET_PRIVATE (camera);

    if (!check_error (AT_Command (priv->handle, L"AcquisitionStop"), "Cannot stop acquisition", error))
        return;

    check_error (AT_Flush (priv->handle),				
		 "Could not flush out remaining queued buffers", error);

    check_error (AT_GetBool (priv->handle, L"CameraAcquiring", &priv->is_cam_acquiring),
                 "Could not read CameraAcquiring", error);
}

static gboolean
uca_andor_camera_grab (UcaCamera *camera, gpointer data, GError **error)
{
	UcaAndorCameraPrivate *priv;
	AT_U8* buffer;
	int err, size;	
	
	g_return_val_if_fail (UCA_IS_ANDOR_CAMERA(camera), FALSE);
	priv = UCA_ANDOR_CAMERA_GET_PRIVATE(camera);

	err = AT_WaitBuffer (priv->handle, &buffer, &size, WAIT_BUFFER_TIMEOUT);
	if (!check_error (err, "Could not grab frame", error)) return FALSE;

	/* Decoding buffer */
		err = AT_ConvertBuffer (buffer, (AT_U8*) data, priv->aoi_width, priv->aoi_height, priv->aoi_stride, priv->pixel_encoding_wchar, L"Mono16");
		if (!check_error (err, "Could not convert buffer", error))
			return FALSE;

	/* Re-queue used buffer -> Useless in 'Fixed' cycle mode ... but as long as we flush out at both end and start of acquisition it does not matter */
		err=AT_QueueBuffer (priv->handle, buffer, size);
		if (!check_error (err, "Could not queue new buffer", error))
			return FALSE;

	return TRUE;	
}

static void
uca_andor_camera_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
    g_return_if_fail (UCA_IS_ANDOR_CAMERA(object));
    UcaAndorCameraPrivate *priv = UCA_ANDOR_CAMERA_GET_PRIVATE(object);
    guint64 val_uint64 = 0;
    double val_double = 0.0;
    gint val_enum = 0;
    gboolean val_bool = FALSE;	

    switch (property_id) {
        case PROP_EXPOSURE_TIME:
            val_double = g_value_get_double (value);
            if (write_double (priv, L"ExposureTime", val_double)) 
                read_double (priv, L"ExposureTime", &priv->exp_time);	/* When writting a float, we should always immediatly read it afterward to get the actual value */
            break;
        case PROP_ROI_WIDTH:
            val_uint64 = g_value_get_uint (value);
            if (write_integer (priv, L"AOIWidth", val_uint64))
                priv->aoi_width = (guint) val_uint64;
            break;
        case PROP_ROI_HEIGHT:
            val_uint64 = g_value_get_uint (value);
            if (write_integer (priv, L"AOIHeight", val_uint64))
                priv->aoi_height = (guint) val_uint64;
            break;
        case PROP_ROI_X:
            val_uint64 = g_value_get_uint (value);
            if (write_integer (priv, L"AOILeft", val_uint64))
                priv->aoi_left = val_uint64;
            break;
        case PROP_ROI_Y:
            val_uint = g_value_get_uint (value);
            if (write_integer (priv, L"AOITop", val_uint))
                priv->aoi_top = val_uint;
            break;
        case PROP_SENSOR_WIDTH:
            val_uint = g_value_get_uint (value);
            if (write_integer (priv, L"SensorWidth", val_uint))
                priv->sensor_width = val_uint;
            break;
        case PROP_SENSOR_HEIGHT:
            val_uint = g_value_get_uint (value);
            if (write_integer (priv, L"SensorHeight", val_uint))
                priv->sensor_height = val_uint;
            break;
        case PROP_SENSOR_PIXEL_WIDTH:
            val_double = g_value_get_double (value);
            if (write_double (priv, L"PixelWidth", val_double))
                priv->pixel_width = val_double;
            break;
        case PROP_SENSOR_PIXEL_HEIGHT:
            val_double = g_value_get_double (value);
            if (write_double (priv, L"PixelHeight", val_double))
                priv->pixel_height = val_double;
            break;
        case PROP_SENSOR_BITDEPTH:
            break;
#if 0
        case PROP_TRIGGER_MODE:
            val_enum = g_value_get_enum (value);
            if (write_enum_index (priv, L"TriggerMode", val_enum))
                priv->trigger_mode = val_enum;
            break;
#endif
        case PROP_FRAMERATE:
            val_double = g_value_get_double (value);
            if (write_double (priv, L"FrameRate", val_double))
                priv->frame_rate = g_value_get_float (value);
            break;
        case PROP_TARGET_SENSOR_TEMPERATURE:
            val_double = g_value_get_double (value);
            if (write_double (priv, L"TargetSensorTemperature", val_double))
                priv->target_sensor_temperature = val_double;
            break;
        case PROP_FAN_SPEED:
            val_enum = g_value_get_int (value);
            if (write_enum_index (priv, L"FanSpeed", val_enum))
                priv->fan_speed = val_enum;
            break;
        case PROP_CYCLE_MODE:
            if (write_string (priv, L"CycleMode", g_value_get_string (value))) {
                g_free (priv->cycle_mode);
                priv->cycle_mode = g_value_dup_string (value);
            }
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
uca_andor_camera_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    g_return_if_fail (UCA_IS_ANDOR_CAMERA(object));
    UcaAndorCameraPrivate *priv = UCA_ANDOR_CAMERA_GET_PRIVATE(object);
    guint64 val_uint64 = 0;
    double val_double = 0.0;
    gint val_enum = 0;
    gchar* val_char;
    gboolean val_bool = FALSE;		

    switch (property_id) {
        case PROP_NAME:
            g_value_set_string (value, priv->name);
            break;
        case PROP_EXPOSURE_TIME:
            if (read_double (priv, L"ExposureTime", &val_double))
                g_value_set_double (value, val_double);
            break;
        case PROP_ROI_WIDTH:
            if (read_integer (priv, L"AOIWidth", &val_uint64))
                g_value_set_uint (value, val_uint64);
            break;
        case PROP_ROI_HEIGHT:
            if (read_integer (priv, L"AOIHeight", &val_uint64))
                g_value_set_uint (value, val_uint64);
            break;
        case PROP_ROI_X:
            if (read_integer (priv, L"AOILeft", &val_uint64))
                g_value_set_uint (value, val_uint64);
            break;
        case PROP_ROI_Y:
            if (read_integer (priv, L"AOITop", &val_uint64))
                g_value_set_uint (value, val_uint64);
            break;
        case PROP_SENSOR_WIDTH:
            if (read_integer (priv, L"SensorWidth", &val_uint64))
                g_value_set_uint (value, val_uint64);
            break;
        case PROP_SENSOR_HEIGHT:
            if (read_integer (priv, L"SensorHeight", &val_uint64))
                g_value_set_uint (value, val_uint64);
            break;
        case PROP_SENSOR_PIXEL_WIDTH:
            if (read_double (priv, L"PixelWidth", &val_double))
                g_value_set_double (value, val_double*1e-6);
            break;
        case PROP_SENSOR_PIXEL_HEIGHT:
            if (read_double (priv, L"PixelHeight", &val_double))
                g_value_set_double (value, val_double*1e-6);
            break;
        case PROP_SENSOR_BITDEPTH:
            g_value_set_uint (value, 16);
            break;
#if 0
        case PROP_TRIGGER_MODE:
            if (read_enum_index (priv, L"TriggerMode", &val_enum))
                g_value_set_enum (value, val_enum);
            break;
#endif
        case PROP_ROI_STRIDE:
            if (read_integer (priv, L"AOIStride", &val_uint64))
                g_value_set_uint (value, val_uint64);
            break;
        case PROP_FRAMERATE:
            if (read_double (priv, L"FrameRate", &val_double))
                g_value_set_float (value, (float) val_double);
            break;
        case PROP_SENSOR_TEMPERATURE:
            if (read_double (priv, L"SensorTemperature", &val_double))
                g_value_set_double (value, val_double);
            break;
        case PROP_TARGET_SENSOR_TEMPERATURE:
            if (read_double (priv, L"TargetSensorTemperature", &val_double))
                g_value_set_double (value, val_double);
            break;
        case PROP_FAN_SPEED:
            if (read_enum_index (priv, L"FanSpeed", &val_enum))
                g_value_set_int (value, val_enum);
            break;
        case PROP_CYCLE_MODE:
            if (read_string (priv, L"CycleMode", &val_char)) {
                g_value_set_string (value, val_char);
                g_free (val_char);
            }
            break;
        case PROP_IS_RECORDING:
            g_value_set_boolean (value, priv->is_cam_acquiring);
            break;
        case PROP_HAS_CAMRAM_RECORDING:
            g_value_set_boolean (value, FALSE);
            break;
        case PROP_HAS_STREAMING:
            g_value_set_boolean (value, TRUE);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
} 

static gboolean
ufo_andor_camera_initable_init (GInitable *initable,
                                GCancellable *cancellable,
                                GError **error)
{
    UcaAndorCamera *cam;
    UcaAndorCameraPrivate *priv;

    cam = UCA_ANDOR_CAMERA (initable);
    priv = cam->priv;

    g_return_val_if_fail (UCA_IS_ANDOR_CAMERA(initable), FALSE);

    if (priv->construct_error != NULL) {
        if (error)
            *error = g_error_copy (priv->construct_error);

        return FALSE;
    }

    return TRUE;
}

static void
uca_andor_initable_iface_init (GInitableIface *iface)
{
    iface->init = ufo_andor_camera_initable_init;
}

static void
uca_andor_camera_finalize (GObject *object)
{
    UcaAndorCameraPrivate *priv;
    g_return_if_fail (UCA_IS_ANDOR_CAMERA(object));
    priv = UCA_ANDOR_CAMERA_GET_PRIVATE(object);
    int error_number;		

    if (!priv->is_sim_cam) {	/* callbacks are only registered (and thus need to be unregistered) if this is the actual camera */
	error_number = AT_UnregisterFeatureCallback(priv->handle, L"PixelEncoding", watch_for_PixelEncoding, (void *) priv);				
	if (error_number) g_error ("Could not unregister PixelEncoding Callback: %s (%d)", identify_andor_error(error_number), error_number);	

    }

    if (AT_Close (priv->handle) != AT_SUCCESS)
        return;

    if (AT_FinaliseLibrary () != AT_SUCCESS)
        return;

    if (AT_FinaliseUtilityLibrary () != AT_SUCCESS)	
        return;

    g_free (priv->image_buffer);
    g_free (priv->model);
    g_free (priv->name);

    if (priv->construct_error != NULL)
        g_clear_error (&(priv->construct_error));

    G_OBJECT_CLASS (uca_andor_camera_parent_class)->finalize (object);
}

static void
uca_andor_camera_trigger (UcaCamera *camera, GError **error)
{
}

static void
uca_andor_camera_class_init (UcaAndorCameraClass *klass)
{
    GObjectClass *oclass = G_OBJECT_CLASS(klass);
    UcaCameraClass *camera_class = UCA_CAMERA_CLASS(klass);

    oclass->set_property = uca_andor_camera_set_property;
    oclass->get_property = uca_andor_camera_get_property;
    oclass->finalize = uca_andor_camera_finalize;

    camera_class->start_recording = uca_andor_camera_start_recording;
    camera_class->stop_recording = uca_andor_camera_stop_recording;
    camera_class->grab = uca_andor_camera_grab;
    camera_class->trigger = uca_andor_camera_trigger;

    for (guint i = 0; andor_overrideables [i] != 0; i++)
        g_object_class_override_property (oclass, andor_overrideables [i], uca_camera_props [andor_overrideables [i]]);

    andor_properties [PROP_ROI_STRIDE] =
        g_param_spec_uint ("roi-stride",
                "ROI Stride",
                "The stride of the region (or area) of interest",
                0,G_MAXINT, 1,
                G_PARAM_READABLE);

    andor_properties [PROP_SENSOR_TEMPERATURE] =
        g_param_spec_double ("sensor-temperature",
                "sensor-temp",
                "The current temperature of the sensor",
                -100.0, 100.0, 20.0,
                G_PARAM_READABLE);

    andor_properties [PROP_TARGET_SENSOR_TEMPERATURE] =
        g_param_spec_double ("target-sensor-temperature",
                "target-sensor-temp",
                "The temperature that is to be reached before acquisition may start",
                -100.0, 100.0, 20.0,
                G_PARAM_READWRITE);

    andor_properties [PROP_FAN_SPEED] =
        g_param_spec_int ("fan-speed",
                "fan-speed",
                "The speed by which the fan is rotating",
                -100, 100, 0,
                G_PARAM_READWRITE);

    andor_properties [PROP_CYCLE_MODE] =
        g_param_spec_string ("cycle-mode",
                "cylce mode",
                "The currently used cycle mode for the acquisition",
                "F",
                G_PARAM_READWRITE);

    andor_properties [PROP_FRAMERATE] =
        g_param_spec_float ("frame-rate",
                "frame rate",
                "The current frame rate of the camera",
                1.0f, 100.0f, 100.0f,
                G_PARAM_READWRITE);

    for (guint id = N_BASE_PROPERTIES; id < N_PROPERTIES; id++)
        g_object_class_install_property (oclass, id, andor_properties [id]);

    g_type_class_add_private (klass, sizeof (UcaAndorCameraPrivate));
}

static void
uca_andor_camera_init (UcaAndorCamera *self)
{
    UcaAndorCameraPrivate *priv;
    AT_H handle;
    AT_WC *model;
    AT_BOOL cam_acq;
    GError **error;
    int error_number;

    self->priv = priv = UCA_ANDOR_CAMERA_GET_PRIVATE (self);
    priv->construct_error = NULL;
    priv->image_buffer = NULL;

    error = &(priv->construct_error);

    error_number = AT_InitialiseLibrary ();
    if (!check_error (error_number, "Could not initialize library", error))
        return;
    error_number = AT_InitialiseUtilityLibrary (); 					
    if (!check_error (error_number, "Could not initialize utility library", error))	
        return;

    priv->camera_number = 0;
    priv->is_sim_cam = FALSE;

    error_number = AT_Open (priv->camera_number, &handle);
    if (!check_error (error_number, "Could not open Handle", error)) return;

    priv->handle = handle;

/*  Retreiving informations at initialisation */
    priv->model = g_malloc0 (1023 * sizeof (gchar));
    model = g_malloc0 (1023 * sizeof (AT_WC));
    error_number = AT_GetString (handle, L"CameraModel", model, 1023);
    if (!check_error (error_number, "Cannot read CameraModel", error)) return;

    gchar* modelchar = g_malloc0 ((wcslen (model) + 1) * sizeof (gchar));
    wcstombs (modelchar, model, wcslen (model));
    strcpy (priv->model, modelchar);

    priv->is_sim_cam = strcmp (modelchar, "SIMCAM CMOS") == 0;

    if (priv->is_sim_cam == FALSE) {
        AT_WC* name = g_malloc0 (1023*sizeof (AT_WC));
        error_number = AT_GetString (handle, L"CameraName", name, 1023);
        priv->name = g_strdup (priv->model);
	if (!check_error (error_number, "Cannot read name", error)) return;
    }
    else {
        priv->name = g_strdup ("SIMCAM CMOS (model)");
    }

    error_number = AT_GetFloat (handle, L"ExposureTime", &priv->exp_time);
    if (!check_error (error_number, "Cannot read ExposureTime", error)) return;

    error_number = AT_GetInt (handle, L"AOIWidth", (AT_64 *) &priv->aoi_width);
    if (!check_error (error_number, "Cannot read AOIWidth", error)) return;

    error_number = AT_GetInt (handle, L"AOIHeight", (AT_64 *) &priv->aoi_height);
    if (!check_error (error_number, "Cannot read AOIHeight", error)) return;

    error_number = AT_GetInt (handle, L"AOILeft", (AT_64 *) &priv->aoi_left);
    if (!check_error (error_number, "Cannot read AOILeft", error)) return;

    error_number = AT_GetInt (handle, L"AOITop", (AT_64 *) &priv->aoi_top);
    if (!check_error (error_number, "Cannot read AOITop", error)) return;

    error_number = AT_GetInt (handle, L"AOIStride", (AT_64 *) &priv->aoi_stride);
    if (!check_error (error_number, "Cannot read AOIStride", error)) return;

    error_number = AT_GetInt (handle, L"SensorWidth", (AT_64 *) &priv->sensor_width);
    if (!check_error (error_number, "Cannot read SensorWidth", error)) return;

    error_number = AT_GetInt (handle, L"SensorHeight", (AT_64 *) &priv->sensor_height);
    if (!check_error (error_number, "Cannot read SensorHeight", error)) return;

    error_number = AT_GetFloat (handle, L"PixelWidth", &priv->pixel_width);
    if (!check_error (error_number, "Cannot read PixelWidth", error)) return;

    error_number = AT_GetFloat (handle, L"PixelHeight", &priv->pixel_height);
    if (!check_error (error_number, "Cannot read PixelHeight", error)) return;

    error_number = AT_GetEnumIndex (handle, L"TriggerMode", &priv->trigger_mode);
    if (!check_error (error_number, "Cannot read TriggerMode", error)) return;

    error_number = AT_GetFloat (handle, L"FrameRate", &priv->frame_rate);
    if (!check_error (error_number, "Cannot read FrameRate", error)) return;

    error_number = AT_GetFloat (handle, L"SensorTemperature", &priv->sensor_temperature);
    if (!check_error (error_number, "Cannot read SensorTemperature", error)) return;

    error_number = AT_GetFloat (handle, L"TargetSensorTemperature", &priv->target_sensor_temperature);
    if (!check_error (error_number, "Cannot read TargetSensorTemperature", error)) return;

    error_number = AT_GetEnumIndex (handle, L"FanSpeed", &priv->fan_speed);
    if (!check_error (error_number, "Cannot read FanSpeed", error)) return;

    priv->cycle_mode = NULL;

    error_number = AT_GetBool (handle, L"CameraAcquiring", &cam_acq);
    if (!check_error (error_number, "Cannot read CameraAcquiring", error)) return;
    priv->is_cam_acquiring = (gboolean) cam_acq;

    uca_camera_register_unit (UCA_CAMERA (self), "frame-rate", UCA_UNIT_COUNT);

    if (!priv->is_sim_cam) {	/* Features and Callbacks only implemented on actual camera and not on SIMCAM */

	/*  CALLBACKS registration (initialisations) */
	error_number = AT_RegisterFeatureCallback(handle, L"PixelEncoding", watch_for_PixelEncoding, (void *) priv);	
	if (!check_error (error_number, "Could not register PixelEncoding Callback", error)) return;
    }
							

    g_free (model);
    g_free (modelchar);

}

G_MODULE_EXPORT GType
camera_plugin_get_type (void)
{
    return UCA_TYPE_ANDOR_CAMERA;
}
