#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <pwd.h>

static char *action = "/etc/wol.action";
static struct passwd action_passwd;
static char *user_env[32];

static void err_handler(char *err)
{
	syslog(LOG_ERR, "%s: %m", err);
	exit(errno);
}

static int name_to_passwd(char const *name)
{
	const long buflen = sysconf(_SC_GETPW_R_SIZE_MAX);
	struct passwd pwbuf, *pwbufp;
	char *buf;

	if (!name)
		return -1;

	if (buflen == -1)
		return -1;

	buf = malloc(buflen);
	if (!buf)
		return -1;

	/* On error always sets pwbufp to NULL */
	getpwnam_r(name, &pwbuf, buf, buflen, &pwbufp);
	if (!pwbufp)
		return -1;

	memcpy(&action_passwd, pwbufp, sizeof(*pwbufp));

	/* don't free the buffer as it will free data pointed by action_passwd elements */
	// free(buf);

	return 0;
}

static char *set_env_var(const char *name, const char *value)
{
	char buff[256];
	int len;

	if (!value)
		return NULL;

	len = snprintf(buff, sizeof(buff), "%s=%s", name, value);
	/* fail on error and if output was truncated */
	if ((len < 0) || len >= sizeof(buff))
		return NULL;

	return strdup(buff);
}

static void setup_user_env()
{
	int i = 0;

	/*
	 * Setup environment for action user
	 * Maximum 32 variables (if need more increase at the user_env declaration)
	 */
	user_env[i++] = set_env_var("PATH", "/bin:/usr/bin");
	user_env[i++] = set_env_var("IFS", " \t\n");
	user_env[i++] = set_env_var("USER", action_passwd.pw_name);
	user_env[i++] = set_env_var("USERNAME", action_passwd.pw_name);
	user_env[i++] = set_env_var("LOGNAME", action_passwd.pw_name);
	user_env[i++] = set_env_var("HOME", action_passwd.pw_dir);
	user_env[i++] = set_env_var("SHELL", action_passwd.pw_shell);
	/* Copy DISPLAY (if available) for GUI apps */
	if (getenv("DISPLAY"))
		user_env[i++] = set_env_var("DISPLAY", getenv("DISPLAY"));

	/* check if all variables were set correctly */
	while (i--)
		if (!user_env[i]) {
			fprintf(stderr, "Failed to generate action user environment.\n");
			exit(EXIT_FAILURE);
		}	
}

static int run_action(void)
{
	pid_t pid;
	int rv;
	int commpipe[2]; /* This holds the fd for the input & output of the pipe */

	/* Setup communication pipeline first */
	if (pipe(commpipe))
		err_handler("Pipe error!");

	/* Attempt to fork and check for errors */
	pid = fork();
	if (pid == -1)
		err_handler("Fork failed");  /* something went wrong */

	if (pid) {
		/* A positive (non-negative) PID indicates the parent process */
		dup2(commpipe[1],1);	/* Replace stdout with out side of the pipe */
		close(commpipe[0]);	/* Close unused side of pipe (in side) */
		setvbuf(stdout,(char*)NULL,_IONBF,0);	/* Set non-buffered output on stdout */
		wait(&rv);				/* Wait for child process to end */
		return rv;
	} else {
		/* A zero PID indicates that this is the child process */
		dup2(commpipe[0], 0);	/* Replace stdin with the in side of the pipe */
		close(commpipe[1]);	/* Close unused side of pipe (out side) */

		/* Run action as different user */
		if (setgid(action_passwd.pw_gid) || setuid(action_passwd.pw_uid))
			err_handler("Failed to change user");

		/* change workinf dir to user HOME */
		if (chdir(action_passwd.pw_dir))
			err_handler("Failed to change working directory");

		/* Replace the child fork with a new process */
		if (execle(action, action, NULL, user_env) == -1)
			err_handler("Failed to run action");
	}

	/* to silence compiler warning */
	return 0;
}

static void listen_wol(uint16_t port)
{
	int sfd;
	struct sockaddr_in si;
	char buf[256];
	const char sync[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sfd == -1)
		err_handler("Failed to open socket");

	memset(&si, 0, sizeof(si));
	si.sin_family = AF_INET;
	si.sin_port = htons(port);
	si.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sfd, (struct sockaddr *)&si, sizeof(si)) == -1)
		err_handler("Failed open port");

	while (1) {
		int len;

		len = recvfrom(sfd, buf, sizeof(buf), 0, NULL, NULL);
		if (len == -1)
			err_handler("Error while reading socket");

		if (len < 102) {
			syslog(LOG_WARNING, "Received packet too short: %u", len);
			continue;
		}

		if (memcmp(sync, buf, 6)) {
			syslog(LOG_WARNING, "Received non WOL packet");
			continue;
		}

		syslog(LOG_INFO, "Received WOL packet");
		syslog(LOG_INFO, "%s returned %i", action, run_action());
	}
}

static void usage()
{
	fprintf(stderr, "Usage: wold -u user [-p port] [-a action] [-f]\n");
	fprintf(stderr, "\t-u username\trun action as different user (required)\n");
	fprintf(stderr, "\t-p port\tport to listen for WOL packet (default: 9)\n");
	fprintf(stderr, "\t-f\tstay in foreground\n");
	fprintf(stderr, "\t-a action\tapplication to run on WOL (default: /etc/wol.action)\n");
}

int main(int argc, char *argv[])
{
	int opt;
	int foreground = 0;
	int user_set = 0;
	unsigned long int port = 9;

	while ((opt = getopt(argc, argv, "u:p:fa:")) != -1) {
		switch (opt) {
			case 'u':
				if (name_to_passwd(optarg)) {
					fprintf(stderr, "User %s not found\n", optarg);
					exit(EXIT_FAILURE);
				}
				user_set = 1;
				break;
			case 'p':
				port = strtoul(optarg, NULL, 0);
				if (port > USHRT_MAX) {
					perror("Invalid port");
					exit(errno);
				}
				break;
			case 'f':
				foreground = 1;
				break;
			case 'a':
				action = strdup(optarg);
				break;
			default:
				usage();
				exit(EXIT_FAILURE);
		}
	}

	if (!user_set) {
		fprintf(stderr, "User to run action not provided. It is required.\n");
		exit(EXIT_FAILURE);
	}

	setup_user_env();

	if (access(action, X_OK)) {
		fprintf(stderr, "%s: %s\n", action, strerror(errno));
		exit(errno);
	}

	if (!foreground) {
		if (daemon(0, 0) < 0) {
			perror("Failed to daemonize");
			exit(errno);
		}

		openlog("wold", LOG_PID, LOG_DAEMON);
	} else
		openlog("wold", LOG_PERROR | LOG_PID, LOG_DAEMON);

	listen_wol((uint16_t)port);

	return 0;
}
