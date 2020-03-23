/*
 * gnssLogger 
 * logs a timestamp with dummy data onto gnss.txt every 10 seconds
 *
 * Listoe Dev
 * 18/12/09
 */
 
#include "legato.h"
#include "interfaces.h"
#include "le_tty.h"
#include "string.h"
#include "unistd.h"
#include "stdio.h"
#include "time.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "gps.h"

#define GNSS_SAMPLE_INTERVAL_IN_MILLISECONDS (1000)

static const char RedLEDResPath[] = "/sys/devices/platform/expander.0/tri_led_red";
static const char BluLEDResPath[] = "/sys/devices/platform/expander.0/tri_led_blu";
//static const char GrnLEDResPath[] = "/sys/devices/platform/expander.0/tri_led_grn";
//static const char GenericLEDResPath[] = "/sys/devices/platform/expander.0/generic_led";



//--------------------------------------------------------------------------------------------------
/**
 * Writes a string to a file.
 *
 * @return
 * - LE_OK on success.
 * - LE_FAULT if the file couldn't be opened.
 * - LE_IO_ERROR if the write failed.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WriteStringToFile
(
    const char *path,   //< null-terminated file system path to write to.
    const char *s       //< null-terminated string to write (null char won't be written).
)
{
    le_result_t res = LE_OK;
    const size_t sLen = strlen(s);
    FILE *f = fopen(path, "r+");
    if (!f)
    {
        LE_ERROR("Couldn't open %s: %s", path, strerror(errno));
        res = LE_FAULT;
        goto done;
    }

    if (fwrite(s, 1, sLen, f) != sLen)
    {
        LE_ERROR("Write to %s failed", path);
        res = LE_IO_ERROR;
        goto cleanup;
    }

cleanup:
    fclose(f);
done:
    return res;
}


//--------------------------------------------------------------------------------------------------
/*
 * gnssTmer runs every 10 seconds when called as gnssTmerRef handler
 */
//--------------------------------------------------------------------------------------------------

/**
 * Convenience function to get current time as uint64_t.
 *
 * @return
 *      Current time as a uint64_t
 */
//--------------------------------------------------------------------------------------------------
static uint64_t GetCurrentTimestamp(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t utcMilliSec = (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
    return utcMilliSec;
}
/*
static void kickGNSS()
{
	char systemCommand[300] = {0};
    int systemResult;
    
	sprintf(systemCommand, "/legato/systems/current/bin/gnss start");	
    systemResult = system(systemCommand);
    // Return value of -1 means that the fork() has failed (see man system).
    if (0 == WEXITSTATUS(systemResult))
    {
        LE_INFO("Succesfully started gnss> %s", systemCommand);
    }
    else
    {
        LE_ERROR("Error (%d), sys> %s", systemResult, systemCommand);
    }
    
    sprintf(systemCommand, "/legato/systems/current/bin/gnss get posInfo");	
    systemResult = system(systemCommand);
    // Return value of -1 means that the fork() has failed (see man system).
    if (0 == WEXITSTATUS(systemResult))
    {
        LE_INFO("Succesfully kicked> %s", systemCommand);
    }
    else
    {
        LE_ERROR("Error (%d), sys> %s", systemResult, systemCommand);
    }
}
*/

static void displayError()
{
	LE_ASSERT_OK(WriteStringToFile(RedLEDResPath, "1"));
	LE_ASSERT_OK(WriteStringToFile(BluLEDResPath, "0"));
}

static void displayOK()
{
	LE_ASSERT_OK(WriteStringToFile(RedLEDResPath, "0"));
	LE_ASSERT_OK(WriteStringToFile(BluLEDResPath, "1"));
}

static void gnssLogTimer(le_timer_Ref_t gnssLogTimerRef)
{
    //char timestamp[80] = {};
	char timestamp[80] = {0};
	char *display = (char*)malloc(22*sizeof(char));
	// Atomic write example, File Descriptor case.
	//char filenamebuff[255] = {0};
	time_t     now;
    struct tm  ts;
    
    // Get current time
    time(&now);

    // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
    ts = *localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
    
    double latitude;
    double longitude;
    double hAccuracy;
    double altitude;
    double vAccuracy;
    
    uint64_t tnow = GetCurrentTimestamp();
    le_result_t posRes = mangOH_ReadGps(&latitude, &longitude, &hAccuracy, &altitude, &vAccuracy);
	//sprintf(filenamebuff,"sdcard/gnssLog.txt", timestamp);
	FILE* fd = fopen ("sdcard/gnssLog.txt", "a");
	//GetCurrentTimestamp(timestamp);
	
	if (fd == NULL)
	{
		// Print error message and exit.
		LE_INFO("file could not be opened");
	}
	else{
		if (posRes == LE_OK)
		{
			// Write something in fd
		  	
		fprintf(fd, "%lld\t%f\t%f\t%f\t%f\t%f\n", tnow, latitude, longitude, hAccuracy, altitude, vAccuracy);
		sprintf(display, "%f, %f", latitude, longitude);
		piOled_Display("Lattitude Longitude", 0);
		piOled_Display(display, 1);
    displayOK();

		}else{
			fprintf(fd, "%lld\t%s\n", tnow, "null\tnull");
			displayError();
			//kickGNSS();
			//fprintf(fd, "%s %s", timestamp, " gnssLog no data\n");
		}
		 
		// Now write this string to fd
		if (fclose(fd) == 0)
		{
			// Print success message
			//LE_INFO("Data successfuly written");
		}
		else
		{
			LE_INFO("Error closing file");
		}
	}
}

//--------------------------------------------------------------------------------------------------
/*
 * Main program starts here
 */
//--------------------------------------------------------------------------------------------------

COMPONENT_INIT
{
	LE_INFO("gnssLogger application has started");
	
	//char timestamp[80] = {0};
	char systemCommand[300] = {0};
	//time_t     now;
    //struct tm  ts;
    int systemResult;
    
    // Get current time
    //time(&now);

    // Format time, "yyyy-mm-dd hh:mm:ss"
    //ts = *localtime(&now);
    //strftime(timestamp, sizeof(timestamp), "%Y-%m-%d-%H-%M-%S", &ts);
    // move old log file to a date stamped file name
    //sprintf(systemCommand, "mv /mnt/userrw/sdcard/gnssLog.txt /mnt/userrw/sdcard/%s_gnssLog.txt", timestamp);
	sprintf(systemCommand, "lastStartTime=$(cat /mnt/userrw/sdcard/lastStartTime.txt); mkdir /mnt/userrw/sdcard/\"$lastStartTime\"; mv /mnt/userrw/sdcard/gnssLog.txt /mnt/userrw/sdcard/\"$lastStartTime\"/\"$lastStartTime\"_gnssLog.txt");
	
    systemResult = system(systemCommand);
    // Return value of -1 means that the fork() has failed (see man system).
    if (0 == WEXITSTATUS(systemResult))
    {
        LE_INFO("Succesfully backed up gnss log file: sys> %s", systemCommand);
    }
    else
    {
        LE_ERROR("Error gnss log file backup Failed: (%d), sys> %s", systemResult, systemCommand);
    }
	
	//write file header line for first row
	FILE* fd = fopen ("sdcard/gnssLog.txt", "a");
	fprintf(fd, "Time\tLatitude\tLongtitude\tHAccuracy\tAltitude\tVAccuracy\n");
		// Now write this string to fd
	if (fclose(fd) == 0)
	{
			// Print success message
		LE_INFO("Data successfuly written");
	}
	else
	{
		LE_INFO("Error closing file");
	}
	
	le_timer_Ref_t gnssLogTimerRef = le_timer_Create("gnssLog Timer");
    le_timer_SetMsInterval(gnssLogTimerRef, GNSS_SAMPLE_INTERVAL_IN_MILLISECONDS);
    le_timer_SetRepeat(gnssLogTimerRef, 0);

    le_timer_SetHandler(gnssLogTimerRef,gnssLogTimer);
    le_timer_Start(gnssLogTimerRef);
	
}
