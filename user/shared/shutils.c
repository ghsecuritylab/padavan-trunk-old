/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <error.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <net/ethernet.h>
#include <dirent.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <syslog.h>
#include <utime.h>

#include "nvram/bcmnvram.h"
#include "shutils.h"

//#define PRIVATE_HANDS_OFF_syscall_retval syscall_retval
//#define PRIVATE_HANDS_OFF_exit_status exit_status

/* Activate 64-bit file support on Linux/32bit plus others */
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 1
#define _LARGEFILE64_SOURCE 1

void
die(const char *str)
{
	printf("%s\n", str);
	exit(-1);
}

void 
bug(const char *str)
{
	printf("%s\n", str);
}

/*
 * Reads file and returns contents
 * @param	fd	file descriptor
 * @return	contents of file or NULL if an error occurred
 */
char *
fd2str(int fd)
{
	char *buf = NULL;
	size_t count = 0, n;

	do {
		buf = realloc(buf, count + 512);
		n = read(fd, buf + count, 512);
		if (n < 0) {
			free(buf);
			buf = NULL;
		}
		count += n;
	} while (n == 512);

	close(fd);
	if (buf)
		buf[count] = '\0';
	return buf;
}

/*
 * Reads file and returns contents
 * @param	path	path to file
 * @return	contents of file or NULL if an error occurred
 */
char *
file2str(const char *path)
{
	int fd;

	if ((fd = open(path, O_RDONLY)) == -1) {
		perror(path);
		return NULL;
	}

	return fd2str(fd);
}

/* 
 * Waits for a file descriptor to change status or unblocked signal
 * @param	fd	file descriptor
 * @param	timeout	seconds to wait before timing out or 0 for no timeout
 * @return	1 if descriptor changed status or 0 if timed out or -1 on error
 */
int
waitfor (int fd, int timeout)
{
	fd_set rfds;
	struct timeval tv = { timeout, 0 };

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	return select(fd + 1, &rfds, NULL, NULL, (timeout > 0) ? &tv : NULL);
}

/* 
 * Concatenates NULL-terminated list of arguments into a single
 * commmand and executes it
 * @param	argv	argument list
 * @param	path	NULL, ">output", or ">>output"
 * @param	timeout	seconds to wait before timing out or 0 for no timeout
 * @param	ppid	NULL to wait for child termination or pointer to pid
 * @return	return value of executed command or errno
 */

int
simple_eval(char *const argv[], char *path, int timeout, int *ppid)
{
	pid_t pid;
	int status;
	int fd;
	int flags;
	int sig;
	int i;

	switch (pid = fork()) {
	case -1:	// error
		perror("fork");
		return errno;
	case 0:		// child
		// Reset signal handlers set for parent process
		for (sig = 0; sig < (_NSIG-1); sig++)
			signal(sig, SIG_DFL);

		// Clean up
		//ioctl(0, TIOCNOTTY, 0);
		//close(STDIN_FILENO);
		setsid();

		for (i=3; i<256; ++i)    // close un-needed fd
			close(i);

		// Redirect stdout to <path>
		if (path) {
			flags = O_WRONLY | O_CREAT;
			if (!strncmp(path, ">>", 2)) {
				// append to <path>
				flags |= O_APPEND;
				path += 2;
			} else if (!strncmp(path, ">", 1)) {
				// overwrite <path>
				flags |= O_TRUNC;
				path += 1;
			}
			if ((fd = open(path, flags, 0644)) < 0)
				perror(path);
			else {
				dup2(fd, STDOUT_FILENO);
				dup2(fd, STDERR_FILENO);
				close(fd);
			}
		}

		// execute command 
		dprintf("%s\n", argv[0]);
		setenv("PATH", "/sbin:/bin:/usr/sbin:/usr/bin", 1);
		alarm(timeout);
		execvp(argv[0], argv);
		perror(argv[0]);
		exit(errno);
	default:	// parent
		if (ppid) {
			*ppid = pid;
			return 0;
		} else {
			if (waitpid(pid, &status, 0) == -1) {
				perror("waitpid");
				return errno;
			}
			if (WIFEXITED(status))
				return WEXITSTATUS(status);
			else
				return status;
		}
	}
}

int
s_eval2(char *const argv[], char *path, int timeout, int *ppid)
{
	pid_t pid;
	int status;
	int sig;
	int i;

	switch (pid = fork()) {
	case -1:	// error
		perror("fork");
		return errno;
	case 0:	 // child
		for (sig = 0; sig < (_NSIG-1); sig++)
			signal(sig, SIG_DFL);

		for (i=3; i<256; ++i)    // close un-needed fd
			close(i);

		setenv("PATH", "/sbin:/bin:/usr/sbin:/usr/bin", 1);
		execvp(argv[0], argv);
		perror(argv[0]);
		exit(errno);
	default:	// parent
		if (ppid) {
			*ppid = pid;
			return 0;
		} else {
			if (waitpid(pid, &status, 0) == -1) {
				perror("waitpid");
				return errno;
			}
			if (WIFEXITED(status))
				return WEXITSTATUS(status);
			else
				return status;
		}
	}
}


void time_zone_x_mapping()
{
	char tmpstr[32];
	char *ptr;

	strcpy(tmpstr, nvram_safe_get("time_zone"));

	/* replace . with : */
	if ((ptr=strchr(tmpstr, '.'))!=NULL) *ptr = ':';
	/* remove *_? */
	if ((ptr=strchr(tmpstr, '_'))!=NULL) *ptr = 0x0;

	/* special mapping */
	if (strcmp(tmpstr, "JST") == 0)
		nvram_set("time_zone_x", "UCT-9");
	else if (strcmp(tmpstr, "TST-10TDT") == 0)
		nvram_set("time_zone_x", "UCT-10");
	else if (strcmp(tmpstr, "CST-9:30CDT") == 0)
		nvram_set("time_zone_x", "UCT-9:30");
	else if (strcmp(tmpstr, "EET-2EETDST") == 0)
		nvram_set("time_zone_x", "EET-2EETDST,M3.5.0/3,M10.5.0/4");
	else if (strcmp(tmpstr, "MET-1METDST") == 0 ||
	         strcmp(tmpstr, "MEZ-1MESZ") == 0 ||
	         strcmp(tmpstr, "CET-1CEST") == 0)
		nvram_set("time_zone_x", "CET-1CEST,M3.5.0,M10.5.0/3");
	else if (strcmp(tmpstr, "GMT0BST") == 0)
		nvram_set("time_zone_x", "GMT0BST,M3.5.0/1,M10.5.0");
	else if (strcmp(tmpstr, "EUT1EUTDST") == 0)
		nvram_set("time_zone_x", "AZOT1AZOST,M3.5.0/0,M10.5.0/1");
	else if (strcmp(tmpstr, "BRT3BRST") == 0)
		nvram_set("time_zone_x", "BRT3BRST,M10.3.0/0,M2.3.0/0");
	else if (strcmp(tmpstr, "AST4ADT") == 0)
		nvram_set("time_zone_x", "AST4ADT,M3.2.0,M11.1.0");
	else if (strcmp(tmpstr, "EST5EDT") == 0)
		nvram_set("time_zone_x", "EST5EDT,M3.2.0,M11.1.0");
	else if (strcmp(tmpstr, "CST6CDT") == 0)
		nvram_set("time_zone_x", "CST6CDT,M3.2.0,M11.1.0");
	else if (strcmp(tmpstr, "MST7MDT") == 0)
		nvram_set("time_zone_x", "MST7MDT,M3.2.0,M11.1.0");
	else if (strcmp(tmpstr, "PST8PDT") == 0)
		nvram_set("time_zone_x", "PST8PDT,M3.2.0,M11.1.0");
	else if (strcmp(tmpstr, "NAST9NADT") == 0)
		nvram_set("time_zone_x", "AKST9AKDT,M3.2.0,M11.1.0");
	else
		nvram_set("time_zone_x", tmpstr);

	doSystem("echo %s > /etc/TZ", nvram_safe_get("time_zone_x"));
}

void change_passwd_unix(char *user, char *pass)
{
	FILE *fp;
	char cmdbuf[64];
	char *tmpfile = "/tmp/tmpchpw";
	
	if (!user || !*user)
		user = "admin";
	
	if (!pass)
		pass = "admin";
	
	fp=fopen(tmpfile, "w");
	if (fp)
	{
		fprintf(fp, "%s:%s\n", user, pass);
		fclose(fp);
	}
	
	sprintf(cmdbuf, "/usr/sbin/chpasswd -m < %s", tmpfile);
	system(cmdbuf);
	sprintf(cmdbuf, "rm -f %s", tmpfile);
	system(cmdbuf);
}

void recreate_passwd_unix(int force_create)
{
	FILE *fp1, *fp2;
	int i, uid, sh_num;
	char tmpusernm[32];
	char *rootnm, *usernm;
	
	rootnm = nvram_safe_get("http_username");
	if (!*rootnm) rootnm = "admin";
	
	fp1 = fopen("/etc/passwd.orig", "w");
	fp2 = fopen("/etc/group.orig", "w");
	if (fp1 && fp2)
	{
		fprintf(fp1, "%s::0:0::/home/admin:/bin/sh\n", rootnm);
		fprintf(fp1, "nobody:x:99:99::/media:/bin/false\n");
		fprintf(fp2, "%s:x:0:%s\n", rootnm, rootnm);
		fprintf(fp2, "nobody:x:99:\n");
		
		sh_num = nvram_get_int("acc_num");
		if (sh_num > 100) sh_num = 100;
		memset(tmpusernm, 0, sizeof(tmpusernm));
		uid=1000;
		for (i=0; i<sh_num; i++)
		{
			sprintf(tmpusernm, "acc_username%d", i);
			usernm = nvram_safe_get(tmpusernm);
			if (*usernm && strcmp(usernm, "root") && strcmp(usernm, "nobody") && strcmp(usernm, rootnm))
			{
				fprintf(fp1, "%s:x:%d:%d:::\n", usernm, uid, uid);
				fprintf(fp2, "%s:x:%d:\n", usernm, uid);
				uid++;
			}
		}
	}
	
	if (fp1) fclose(fp1);
	if (fp2) fclose(fp2);
	
	if (force_create || system("md5sum -cs /tmp/hashes/passwd_md5") != 0)
	{
		system("md5sum /etc/passwd.orig /etc/group.orig > /tmp/hashes/passwd_md5");
		system("cp -f /etc/passwd.orig /etc/passwd");
		system("cp -f /etc/group.orig /etc/group");
		change_passwd_unix(rootnm, nvram_safe_get("http_passwd"));
	}
	
	unlink("/etc/passwd.orig");
	unlink("/etc/group.orig");
}


int
_eval(char *const argv[], char *path, int timeout, int *ppid)
{
	sigset_t set;
	pid_t pid, ret;
	int status;
	int fd;
	int flags;
	int sig, i;

	switch (pid = fork()) {
	case -1:	/* error */
		perror("fork");
		return errno;
	case 0:	 /* child */
		/* Reset signal handlers set for parent process */
		for (sig = 0; sig < (_NSIG-1); sig++)
			signal(sig, SIG_DFL);

		/* Unblock signals if called from signal handler */
		sigemptyset(&set);
		sigprocmask(SIG_SETMASK, &set, NULL);

		/* Clean up */
		for (i=3; i<256; i++)    // close un-needed fd
			close(i);
		ioctl(0, TIOCNOTTY, 0);	// detach from current process
		setsid();
		
		/* Redirect stdout to <path> */
		if (path) {
			flags = O_WRONLY | O_CREAT;
			if (!strncmp(path, ">>", 2)) {
				/* append to <path> */
				flags |= O_APPEND;
				path += 2;
			} else if (!strncmp(path, ">", 1)) {
				/* overwrite <path> */
				flags |= O_TRUNC;
				path += 1;
			}
			if ((fd = open(path, flags, 0644)) < 0)
				perror(path);
			else {
				dup2(fd, STDOUT_FILENO);
				dup2(fd, STDERR_FILENO);
				close(fd);
			}
		}
		
		/* execute command */
		dprintf("%s\n", argv[0]);
		setenv("PATH", "/sbin:/bin:/usr/sbin:/usr/bin", 1);
		alarm(timeout);
		execvp(argv[0], argv);
		perror(argv[0]);
		exit(errno);
	default:	/* parent */
		if (ppid) {
			*ppid = pid;
			return 0;
		} else {
			do
				ret = waitpid(pid, &status, 0);
			while ((ret == -1) && (errno == EINTR));
			
			if (ret != pid) {
				perror("waitpid");
				return errno;
			}
			if (WIFEXITED(status))
				return WEXITSTATUS(status);
			else
				return status;
		}
	}
}

/* 
 * Concatenates NULL-terminated list of arguments into a single
 * commmand and executes it
 * @param	argv	argument list
 * @return	stdout of executed command or NULL if an error occurred
 */
char *
_backtick(char *const argv[])
{
	int filedes[2];
	pid_t pid;
	int status;
	char *buf = NULL;

	/* create pipe */
	if (pipe(filedes) == -1) {
		perror(argv[0]);
		return NULL;
	}

	switch (pid = fork()) {
	case -1:	/* error */
		return NULL;
	case 0:		/* child */
		close(filedes[0]);	/* close read end of pipe */
		dup2(filedes[1], 1);	/* redirect stdout to write end of pipe */
		close(filedes[1]);	/* close write end of pipe */
		execvp(argv[0], argv);
		exit(errno);
		break;
	default:	/* parent */
		close(filedes[1]);	/* close write end of pipe */
		buf = fd2str(filedes[0]);
		waitpid(pid, &status, 0);
		break;
	}
	
	return buf;
}


/*
 * fread() with automatic retry on syscall interrupt
 * @param	ptr	location to store to
 * @param	size	size of each element of data
 * @param	nmemb	number of elements
 * @param	stream	file stream
 * @return	number of items successfully read
 */
int
safe_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = 0;

	do {
		clearerr(stream);
		ret += fread((char *)ptr + (ret * size), size, nmemb - ret, stream);
	} while (ret < nmemb && ferror(stream) && errno == EINTR);

	return ret;
}

/*
 * fwrite() with automatic retry on syscall interrupt
 * @param	ptr	location to read from
 * @param	size	size of each element of data
 * @param	nmemb	number of elements
 * @param	stream	file stream
 * @return	number of items successfully written
 */
int
safe_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = 0;

	do {
		clearerr(stream);
		ret += fwrite((char *)ptr + (ret * size), size, nmemb - ret, stream);
	} while (ret < nmemb && ferror(stream) && errno == EINTR);

	return ret;
}

/*
 * Convert Ethernet address string representation to binary data
 * @param	a	string in xx:xx:xx:xx:xx:xx notation
 * @param	e	binary data
 * @return	TRUE if conversion was successful and FALSE otherwise
 */
int
ether_atoe(const char *a, unsigned char *e)
{
	char *c = (char *) a;
	int i = 0;

	memset(e, 0, ETHER_ADDR_LEN);
	for (;;) {
		e[i++] = (unsigned char) strtoul(c, &c, 16);
		if (!*c++ || i == ETHER_ADDR_LEN)
			break;
	}
	return (i == ETHER_ADDR_LEN);
}

/*
 * Convert Ethernet address binary data to string representation
 * @param	e	binary data
 * @param	a	string in xx:xx:xx:xx:xx:xx notation
 * @return	a
 */
char *
ether_etoa(const unsigned char *e, char *a)
{
	char *c = a;
	int i;

	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		if (i)
			*c++ = ':';
		c += sprintf(c, "%02X", e[i] & 0xff);
	}
	return a;
}

/*
 * Convert Ethernet address binary data to string representation
 * @param       e       binary data
 * @param       a       string in xxxxxxxxxxxx notation
 * @return      a
 */
char *
ether_etoa2(const unsigned char *e, char *a)
{
	char *c = a;
	int i;
	
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		c += sprintf(c, "%02x", e[i] & 0xff);
	}
	return a;
}

char *
ether_etoa3(const unsigned char *e, char *a)
{
	char *c = a;
	int i;
	
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		c += sprintf(c, "%02X", e[i] & 0xff);
	}
	return a;
}

/*
 *  * description: parse va and do system
 *   */
int doSystem(const char *fmt, ...)
{
	char cmd_buf[512];
	va_list pargv;

	va_start(pargv, fmt);
	vsnprintf(cmd_buf, sizeof(cmd_buf), fmt, pargv);
	va_end(pargv);

	return system(cmd_buf);
}

//int
unsigned long
swap_check()
{
	struct sysinfo info;

	sysinfo(&info);
	if (info.totalswap > 0)
	{
		return info.totalswap;
	}
	else
	{
		return 0;
	}
}

/* 
 *  * Kills process whose PID is stored in plaintext in pidfile
 *   * @param       pidfile PID file, signal
 *    * @return      0 on success and errno on failure
 *     */
int
kill_pidfile_s(char *pidfile, int sig)
{
	FILE *fp;
	pid_t pid;
	char buf[64];
	int ret = -1;

	fp = fopen(pidfile, "r");
	if (fp) {
		if (fgets(buf, sizeof(buf), fp)) {
			pid = strtoul(buf, NULL, 0);
			if (pid > 1 && pid < ULONG_MAX)
				ret = kill(pid, sig);
		}
		fclose(fp);
	}

	return ret;
}

/* 
 * Kills process whose PID is stored in plaintext in pidfile
 * @param	pidfile	PID file
 * @return	0 on success and errno on failure
 */
int
kill_pidfile(char *pidfile)
{
	return kill_pidfile_s(pidfile, SIGTERM);
}

int
get_param_int(char *line, const char *param, int base, int defval)
{
	char *ptr = strstr(line, param);
	if (!ptr)
		return defval;
	ptr += strlen(param);
	return strtol(ptr, NULL, base);
}

char *
get_param_str(char *line, const char *param, int dups)
{
	char *ptr = strstr(line, param);
	if (!ptr)
		return NULL;
	ptr += strlen(param);
	return (dups) ? strdup(ptr) : ptr;
}

int
fput_string(const char *name, const char *value)
{
	FILE *fp;

	fp = fopen(name, "w");
	if (fp) {
		fputs(value, fp);
		fclose(fp);
		return 0;
	} else {
		perror(name);
		return errno;
	}
}

int
fput_int(const char *name, int value)
{
	char svalue[32];
	sprintf(svalue, "%d", value);
	return fput_string(name, svalue);
}

int 
compare_text_files(const char* file1, const char* file2)
{
	FILE *fp1, *fp2;
	int ret = 0;
	char *v1, *v2;
	char buf1[MAX_FILE_LINE_SIZE];
	char buf2[MAX_FILE_LINE_SIZE];

	fp1 = fopen(file1, "r");
	if (!fp1)
		return -1;
	fp2 = fopen(file2, "r");
	if (!fp2) {
		fclose(fp1);
		return -1;
	}

	for (;;) {
		v1 = fgets(buf1, sizeof(buf1), fp1);
		v2 = fgets(buf2, sizeof(buf2), fp2);
		if (!v1 || !v2) {
			if (v1 != v2)
				ret = 1;
			break;
		}
		
		if (strcmp(buf1, buf2) != 0) {
			ret = 1;
			break;
		}
	}

	fclose(fp2);
	fclose(fp1);

	return ret;
}

void
logmessage(char *logheader, char *fmt, ...)
{
	va_list args;
	char buf[512];

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	openlog(logheader, 0, 0);
	syslog(0, buf);
	closelog();
	va_end(args);
}

