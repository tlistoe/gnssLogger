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


static void gnssLogTimer(le_timer_Ref_t gnssLogTimerRef)
{
    //char timestamp[80] = {};
	char timestamp[80] = {0};
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
		
		fprintf(fd, "%lld\t%f\t%f\n", tnow, latitude, longitude);
		}else{
			fprintf(fd, "%s %s", timestamp, " gnssLog no data\n");
		}
		

		 
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
	}
}

//--------------------------------------------------------------------------------------------------
/*
 * Main program starts here
 */
//--------------------------------------------------------------------------------------------------

COMPONENT_INIT
{
	LE_INFO("gnssLogTemp application has started");
	
	char timestamp[80] = {0};
	char systemCommand[300] = {0};
	time_t     now;
    struct tm  ts;
    int systemResult;
    
    // Get current time
    time(&now);

    // Format time, "yyyy-mm-dd hh:mm:ss"
    ts = *localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d-%H-%M-%S", &ts);
    // move old log file to a date stamped file name
    sprintf(systemCommand, "mv /mnt/userrw/sdcard/gnssLog.txt /mnt/userrw/sdcard/%s_gnssLog.txt", timestamp);

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
	fprintf(fd, "Time\tlatitude\tlongtitude\n");
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
