
/*******************************************************************************************
 * Copyright (c) 2008 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica *
 *                      Universita' Campus BioMedico - Italy                               *
 *                                                                                         *
 * This program is free software; you can redistribute it and/or modify it under the terms *
 * of the GNU General Public License as published by the Free Software Foundation; either  *
 * version 2 of the License, or (at your option) any later version.                        *
 *                                                                                         *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY         *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A         *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.                *
 *                                                                                         *
 * You should have received a copy of the GNU General Public License along with this       *
 * program; if not, write to the:                                                          *
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,                    *
 * MA  02111-1307, USA.                                                                    *
 *                                                                                         *
 * --------------------------------------------------------------------------------------- *
 * Project:  Capwap                                                                        *
 *                                                                                         *
 * Author: Donato Capitella (d.capitella@gmail.com)                                        *
 *                                            				                     		   *
 *******************************************************************************************/ 

#include <unistd.h>
#include <wait.h>
#include <sys/stat.h>
#include <time.h>
#include "CWCommon.h"
#include "WUM.h"

/* WUA Constants */
#define BUF_SIZE        1024
#define CUP_UNPACK_DIR  "/tmp/cup.unpack"
#define BACKUP_DIR      "/tmp/cup.backup"
#define CUD_FILE_NAME   "update.cud"
#define LOG_FILE        "/var/log/wua.log"

#define WTP_DIR_VAR     "WTP_DIR"
#define CUP_DIR_VAR     "CUP_DIR"

/* Execute the system() function 
 * and return CW_FALSE in case of failure. */
#define SYSTEM_ERR(cmd) \
do {  \
    int exit_c = system(cmd);  \
    if (!WIFEXITED(exit_c) || WEXITSTATUS(exit_c) != 0 ) { \
       return CW_FALSE; \
    } \
} while(0)

/* CUPWAP Update Descriptor */
struct CWUpdateDescriptor {
    char version[BUF_SIZE];
    char pre_script[BUF_SIZE];
    char post_script[BUF_SIZE];
};

/* Default values for CUD */
struct CWUpdateDescriptor cud = {
    .version[0] = '\0',
    .pre_script[0] = '\0',
    .post_script[0] = '\0',
};

/* Function prototypes */
CWBool Unzip(char *filename, char *destdir);
CWBool MakeDir(char *dirname);
CWBool BackupCurrentWTP(char *WTPDir); 
CWBool CheckCUPIntegrity();
CWBool ParseCUD();

CWBool StartWTP(char *WTPDir);
void CleanTmpFiles(char *);
CWBool RestoreBackupWTP(char *WTPDir);

CWBool WUAStage1(char *CupPath, char *WTPDir);
CWBool WUAStage2();

void WaitForWTPTermination();
void daemonize();

/* Log related varibales and functions */
FILE *log_file = NULL;
CWBool WUAInitLog(char *logFile);
void WUALog(char *msg, ...);
void WUALogClose();

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s cup_file\n", argv[0]);
    }

    daemonize();

    WTPUpdateAgent(argv[1]);
    return 0;
}

/*
 * WTPUpdateAgent - main routine of the WUA
 *
 * CupPath: pathname of the capwap update package
 *
 * This function executes an update session, which is divided
 * into three stages:
 *
 *   - Stage 1: preparation for the update (unpack CUP and 
 *     backup current WTP)
 *
 *   - Stage 2: executes pre-update script, copies new files, 
 *     then executes post-update script
 *
 *   - Final Stage: cleans temp files and start new WTP 
 */
void WTPUpdateAgent(char *CupPath)
{
    int exit_status = EXIT_SUCCESS;
    char WTPDir[BUF_SIZE];
    
    if ( !WUAInitLog(LOG_FILE) ) {
        goto quit_wua;
    }

    WaitForWTPTermination();

    WUALog("Update Session Started.");

    if ( !getcwd(WTPDir, BUF_SIZE) ) {
        WUALog("The impossible happened: can't get current working directory!");
        exit_status = EXIT_FAILURE;
	goto quit_wua;
    }

    if ( !WUAStage1(CupPath, WTPDir) ) {
        exit_status = EXIT_FAILURE;
	goto quit_wua;
    }
    
    if ( !WUAStage2(WTPDir) ) {
        exit_status = EXIT_FAILURE;
        if ( !RestoreBackupWTP(WTPDir) ) {
	    WUALog("***CRITICAL ERROR*** -> Can't restore backup WTP!!!");
	    exit(EXIT_FAILURE);
	}
	goto quit_wua;
    }

/* Final Stage */
quit_wua:
    CleanTmpFiles(CupPath);
    StartWTP(WTPDir);
    WUALog("Update Session Ended %s", (exit_status == EXIT_SUCCESS) ? "Successefully" : "With Errors");
    WUALogClose();
    exit(exit_status);
}

/*
 * WaitForWTPTermination 
 * This function returns when the WTP terminates. A lock on a file
 * is used for synchronization purposes.
 */
void WaitForWTPTermination()
{
    struct flock fl;
    int fd;
    
    /* The following lock in set just for synchronization purposes */
    fl.l_type   = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 0;
    fl.l_pid    = getpid();

    if ((fd = open(WTP_LOCK_FILE, O_WRONLY)) < 0) {
    	WUALog("Error while opening lock file: %s\n", WTP_LOCK_FILE);
    }

    fcntl(fd, F_SETLKW, &fl);
   
    close(fd);
    remove(WTP_LOCK_FILE);

    return;
}

/*
 * State 1 of the update process
 *
 * - Unpack & Check CUP file
 * - Backup current WTP  
 */
CWBool WUAStage1(char *CupPath, char *WTPDir)
{
    WUALog("Entering Stage 1...");

    if (!MakeDir(CUP_UNPACK_DIR)) {
        return CW_FALSE;
    }
    
    if (!Unzip(CupPath, CUP_UNPACK_DIR)) {
        WUALog("Something wrong happened while unzipping the CUP archive.");
        return CW_FALSE;
    }
/*
    if (!CheckCUPIntegrity()) {
        WUALog("Problems with the CUP archive.");
        return CW_FALSE;
    }
  */ 
    if (!ParseCUD()) {
        WUALog("Error while parsing CUD.");
        return CW_FALSE;
    }

    WUALog("Update Version: %s", cud.version);

    if (!MakeDir(BACKUP_DIR)) {
        return CW_FALSE;
    }

    if (!BackupCurrentWTP(WTPDir)) {
        WUALog("Can't backup current WTP.");
    	return CW_FALSE;
    }

    WUALog("Stage 1 completed successfully...");
    
    return CW_TRUE;
}

/*
 * State 2 of the update process
 *
 * - Set scripts variables
 * - Execute preupdate script
 * - Copy new files
 * - Execute postupdate script  
 */
CWBool WUAStage2(char *WTPDir) 
{
    int ret;
    char cmd_buf[BUF_SIZE];

    WUALog("Entering Stage 2...");

    /* Prepare env variables for the scripts */
    ret = setenv(WTP_DIR_VAR, WTPDir, 1);   
    if (ret != 0) {
        WUALog("Error while setting env variable.");
        return CW_FALSE;
    }

    ret = setenv(CUP_DIR_VAR, CUP_UNPACK_DIR, 1);   
    if (ret != 0) {
        WUALog("Error while setting env variable.");
        return CW_FALSE;
    }

    /* Execute pre-update script, if any */
    if (strlen(cud.pre_script) > 0) {
        SYSTEM_ERR(cud.pre_script);
    }

    /* Copy new WTP files */
    ret = snprintf(cmd_buf, BUF_SIZE, "cp -r %s/WTP/* %s/", CUP_UNPACK_DIR, WTPDir);
    if (ret < 0 || ret >= BUF_SIZE) {
    	return CW_FALSE;
    }
    SYSTEM_ERR(cmd_buf);
    
    /* Execute post-upsate script, if any */
    if (strlen(cud.post_script) > 0) {
        SYSTEM_ERR(cud.post_script);
    }

    WUALog("Stage 2 completed successefully.");
    return CW_TRUE;
}

/*
 * Unzip - unzips a gzipped archive into the provided directory.
 *
 * Notes: this implementation relies on the tar command. 
 */
CWBool Unzip(char *filename, char *destdir)
{
    char cmd_buf[BUF_SIZE];
    int ret;

    ret = snprintf(cmd_buf, BUF_SIZE, "tar xzf %s -C %s", filename, destdir);
    if (ret < 0 || ret >= BUF_SIZE) {
    	return CW_FALSE;
    }
    
    SYSTEM_ERR(cmd_buf);

    return CW_TRUE;
}

/*
 * StartWTP - starts a WTP daemon
 *
 * WTPDir: the directory where the WTP binary resides.
 */
CWBool StartWTP(char *WTPDir)
{
    char cmd_buf[BUF_SIZE];
    int pid, ret;

    ret = snprintf(cmd_buf, BUF_SIZE, "%s/WTP", WTPDir);
    if (ret < 0 || ret >= BUF_SIZE) {
    	return CW_FALSE;
    }

    pid = fork();

    if (pid == 0) {
        execl(cmd_buf, "WTP", WTPDir, NULL);
	exit(EXIT_FAILURE);
    } else if (pid < 0) {
        WUALog("Error while forking");
        return CW_FALSE;
    }

    return CW_TRUE;
}

CWBool RestoreBackupWTP(char *WTPDir) 
{
    char cmd_buf[BUF_SIZE];
    int ret;

    WUALog("Restoring old WTP...");
    
    /* Del WTPDir content */
    ret = snprintf(cmd_buf, BUF_SIZE, "rm -rf %s/*", WTPDir);
    if (ret < 0 || ret >= BUF_SIZE) {
    	return CW_FALSE;
    }

    SYSTEM_ERR(cmd_buf);

    /* Restore backup */
    ret = snprintf(cmd_buf, BUF_SIZE, "cp -ra %s/* %s/", BACKUP_DIR, WTPDir);
    if (ret < 0 || ret >= BUF_SIZE) {
    	return CW_FALSE;
    }
    SYSTEM_ERR(cmd_buf);

    return CW_TRUE;
}

void StringToLower(char *str)
{
    for(; *str != '\0'; str++)
        *str = tolower(*str);
}

CWBool ParseCUD()
{
    FILE *fp;
    char buf[BUF_SIZE];
    char *token;
    int ret = CW_TRUE;

    ret = snprintf(buf, BUF_SIZE, "%s/%s", CUP_UNPACK_DIR, CUD_FILE_NAME);
    if (ret < 0 || ret >= BUF_SIZE) {
    	return CW_FALSE;
    }

    fp = fopen(buf, "r");
    if (fp == NULL) {
        WUALog("Error while opening update descriptor.");
        return CW_FALSE;
    }

    while (fgets(buf, BUF_SIZE, fp) != NULL) {
        token = strtok(buf, " ");
	StringToLower(token);
	if (strncmp(token, "preupdate", 9) == 0) {
	    token = strtok(NULL, " ");
	    if (token == NULL) {
                WUALog("Error while parsing update descriptor.");
                ret = CW_FALSE;
		goto exit_parse;;
	    }
	    snprintf(cud.pre_script, BUF_SIZE, "%s/scripts/%s", CUP_UNPACK_DIR, token);
	} else
        if (strncmp(token, "postupdate", 10) == 0) {
	    token = strtok(NULL, " ");
	    if (token == NULL) {
                WUALog("Error while parsing update descriptor.");
                ret = CW_FALSE;
		goto exit_parse;;
	    }
	    snprintf(cud.post_script, BUF_SIZE, "%s/scripts/%s", CUP_UNPACK_DIR, token);
	} else
	if (strncmp(token, "version", 7) == 0) {
	    token = strtok(NULL, " ");
	    if (token == NULL) {
                WUALog("Error while parsing update descriptor.");
                ret = CW_FALSE;
		goto exit_parse;;
	    }
	    snprintf(cud.version, BUF_SIZE, "%s", token);
        }	
    }

    if (strlen(cud.version) == 0) {
        WUALog("Error while parsing CUD file, no version specified.");
	ret = CW_FALSE;
    }

exit_parse:
    fclose(fp);
    return ret;
}

void CleanTmpFiles(char *cupFile)
{
    int ret;
    char cmd_buf[BUF_SIZE];

    /* In this function we can afford to ignore return values */

    /* Remove CUP file */
    remove(cupFile);
    
    /* Remove Unpack and Backup Directory */
    ret = snprintf(cmd_buf, BUF_SIZE, "rm -rf %s %s", CUP_UNPACK_DIR, BACKUP_DIR);
    if (ret < 0 || ret >= BUF_SIZE) {
    	return;
    }
    
    ret = system(cmd_buf);
}

CWBool BackupCurrentWTP(char *WTPDir)
{
	
    char cmd_buf[BUF_SIZE];
    int ret;

    ret = snprintf(cmd_buf, BUF_SIZE, "cp -ar %s/* %s", WTPDir, BACKUP_DIR);
    if (ret < 0 || ret >= BUF_SIZE) {
    	return CW_FALSE;
    }
    
    SYSTEM_ERR(cmd_buf);

    return CW_TRUE;
}

CWBool MakeDir(char *dirname)
{
    int ret;

    ret = mkdir(dirname, 0700);
    if (ret != 0) {
    	WUALog("Can't create directory %s", dirname);
	return CW_FALSE;
    }

    return CW_TRUE;
}

/****************************************************************
 * Log Functions
 ****************************************************************/

CWBool WUAInitLog(char *logFile)
{
    if ((log_file = fopen(logFile, "a")) == NULL) {
        return CW_FALSE;
    }
    return CW_TRUE;
}

void WUALog(char *msg, ...)
{
    if (log_file == NULL) return;

    char date_buf[BUF_SIZE] = {'\0'};
    time_t time_s = time(NULL);
    struct tm *tm_s = localtime(&time_s);
    strftime(date_buf, BUF_SIZE, "%d/%b/%Y:%H:%M:%S %z", tm_s);
    fprintf(log_file, "%s: ", date_buf);

    va_list argp;
    va_start(argp, msg);
    vfprintf(log_file, msg, argp);
    va_end(argp);

    fprintf(log_file, "\n");

    fflush(log_file);
}

void WUALogClose()
{
    if (log_file != NULL) fclose(log_file);
}

void daemonize()
{
    pid_t pid;
    int sid;

    /* already a daemon */
    if ( getppid() == 1 ) return;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then we can exit the parent process. */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* At this point we are executing as the child process */

    /* Change the file mode mask */
    umask(0);

    /* Create a new SID for the child process */
    sid = setsid();
        if (sid < 0) {
       exit(EXIT_FAILURE);
    }

    /* Redirect standard files to /dev/null */
    freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
    freopen( "/dev/null", "w", stderr);
}

